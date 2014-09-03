/*
 * rcp.c
 *
 * Copyright 2013 Evan Buswell
 * 
 * This file is part of Atomic Kit.
 * 
 * Atomic Kit is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 2.
 * 
 * Atomic Kit is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "atomickit/atomic.h"
#include "atomickit/malloc.h"
#include "atomickit/rcp.h"

#define __ARCP_HOHDEL __ARCP_COUNTMASK
#define __ARCP_WEAKMAX (__ARCP_COUNTMASK - 1)

/* Update the references for the region, adding storedelta to storecount and
 * usedelta to usecount. Returns true when the region should be deleted. */
static inline bool __arcp_urefs(struct arcp_region *region,
                                int16_t storedelta, int16_t usedelta) {
	arcp_refcount_t count, o_count;
	bool destroy;
	/* load o_count via the pun */
	o_count.p = ak_load(&region->refcount, mo_consume);
	do {
		/* set count to o_count via the pun */
		count.p = o_count.p;
		/* alter count */
		count.v.storecount += storedelta;
		count.v.usecount += usedelta;
		if(count.v.storecount == 0
		   && count.v.usecount == 0
		   && count.v.destroy_lock == 0) {
			/* if we've reached zero, and the destroy lock isn't
 			 * set, try to set the destroy lock */
			destroy = true;
			count.v.destroy_lock = 1;
		} else {
			/* either reference count is not zero, or the destroy
 			 * lock is set */
			destroy = false;
		}
	} while(unlikely(!ak_cas(&region->refcount, &o_count.p, count.p,
			         mo_acq_rel, mo_consume)));
	return destroy;
}

/* Try to release the destroy lock for the region. Returns true if the lock
 * has been released, false if the destroy lock could not be released. */
static bool __arcp_try_release_destroy_lock(struct arcp_region *region) {
	arcp_refcount_t count, o_count;
	/* load o_count via the pun */
	o_count.p = ak_load(&region->refcount, mo_consume);
	do {
		if(o_count.v.storecount == 0
		   && o_count.v.usecount == 0) {
			/* If the storecount and usecount are zero, we would
 			 * be again responsible for destroying, so don't
 			 * release the lock. */
			return false;
		}
		/* set count via the pun */
		count.p = o_count.p;
		/* unset the destroy lock */
		count.v.destroy_lock = 0;
	} while(unlikely(!ak_cas(&region->refcount,
	                         &o_count.p, count.p,
	                         mo_acq_rel, mo_consume)));
	return true;
}

/* Assuming the reference count is (or was) 0 and we hold the destroy_lock,
 * try to destroy the region. */
static void __arcp_try_destroy(struct arcp_region *region) {
	struct arcp_weakref *weakref;
	struct arcp_region *target_o, *target;
	arcp_refcount_t count;

	/* load the weakref for the region */
	weakref =
		(struct arcp_weakref *) ak_load(&region->weakref, mo_consume);
	if(weakref == NULL) {
		/* if there's no weakref, just run the destruction function */
		if(region->destroy != NULL) {
			region->destroy(region);
		}
		return;
	}
	/* load the weakref target pointer to get any count which potentially
 	 * has not yet been migrated to the region itself */
	target_o = ak_load(&weakref->target, mo_consume);
retry:
	switch(__ARCP_PTR2COUNT(target_o)) {
	default:
		/* there is a count on weakref->target; transfer it */
		target = __ARCP_PTRDECOUNT(target_o);
		/* set target to a count-free version */
		if(likely(ak_cas_strong(&weakref->target,
		                        &target_o, target,
		                        mo_acq_rel, mo_consume))) {
			/* update our count */
			__arcp_urefs(region, 0, __ARCP_PTR2COUNT(target_o));
			/* try to release the destroy lock and return */
			if(likely(__arcp_try_release_destroy_lock(region))) {
				return;
			} /* otherwise failed to release destroy lock, fall
			   * through to loop */
		} /* otherwise target changed, loop */
		goto retry;
	case 0:
		/* there is no count on weakref->target awaiting a transfer,
 		 * we have been count-free at at least one moment */
		/* use a hand-over-hand method to properly delete
 		 * weakref->target; set count on weakref->target to
 		 * __ARCP_HOHDEL, which is a value only set by this
 		 * function */
		target = __ARCP_PTRSETCOUNT(target_o, __ARCP_HOHDEL);
		if(unlikely(!ak_cas_strong(&weakref->target,
		                           &target_o, target,
		                           mo_acq_rel, mo_consume))) {
			/* failed; loop */
			goto retry;
		}
		target_o = target;
		/* fall through */
	case __ARCP_HOHDEL:
		/* weakref->target is being prepared for deletion;
 		 * re-check refcount */
		count.p = ak_load(&region->refcount, mo_consume);
		if(unlikely(count.v.storecount != 0 || count.v.usecount != 0)) {
			/* refcount is no longer zero */
			if(likely(__arcp_try_release_destroy_lock(region))) {
				return;
			} /* otherwise, region->refcount has *become* 0, so
			   * fall through as if the above test succeeded */
		}
		/* kill the reference */
		if(unlikely(!ak_cas_strong(&weakref->target,
		                           &target_o, NULL,
		                           mo_acq_rel, mo_consume))) {
			/* failed; loop */
			goto retry;
		}
		/* finished deleting weakref target; fall through */
	}
	/* update weakref reference count and destroy if appropriate */
	if(__arcp_urefs(weakref, -1, 0)) {
		__arcp_try_destroy(weakref);
	}
	/* destroy region */
	if(region->destroy != NULL) {
		region->destroy(region);
	}
}

static void __arcp_destroy_weakref(struct arcp_weakref *stub) {
	afree(stub, sizeof(struct arcp_weakref));
}

void arcp_region_init(struct arcp_region *region, void (*destroy)(struct arcp_region *)) {
	/* initialize to storecount 0, usecount 1 (the caller) */
	ak_init(&region->refcount, __ARCP_REFCOUNT_INIT(0, 1));
	region->destroy = destroy;
	ak_init(&region->weakref, NULL);
}

int arcp_region_init_weakref(struct arcp_region *region) {
	struct arcp_weakref *stub;
	struct arcp_weakref *nostub;

	if(region == NULL) {
		return 0;
	}
	/* load current weakref */
	nostub =
		(struct arcp_weakref *) ak_load(&region->weakref, mo_consume);
	if(nostub != NULL) {
		/* weakref has already been initialized */
		return 0;
	}
	/* allocate a new weakref */
	stub = amalloc(sizeof(struct arcp_weakref));
	if(stub == NULL) {
		return -1;
	}
	/* initialize the weakref */
	ak_init(&stub->target, region);
	/* storecount 1 (the region), usecount 0 */
	ak_init(&stub->refcount, __ARCP_REFCOUNT_INIT(1, 0));
	stub->destroy = (arcp_destroy_f) __arcp_destroy_weakref;
	ak_init(&stub->weakref, NULL);
	if(unlikely(!ak_cas(&region->weakref, &nostub, stub,
	                    mo_acq_rel, mo_relaxed))) {
		/* someone else set the weakref */
		afree(stub, sizeof(struct arcp_weakref));
	}
	return 0;
}

void arcp_region_destroy_weakref(struct arcp_region *region) {
	arcp_store(&region->weakref, NULL);
}

struct arcp_region *__arcp_acquire(struct arcp_region *region) {
	if(region != NULL) {
		__arcp_urefs(region, 0, 1);
	}
	return region;
}

struct arcp_weakref *arcp_weakref(struct arcp_region *region) {
	return (struct arcp_weakref *) arcp_load(&region->weakref);
}

void arcp_release(struct arcp_region *region) {
	if(region != NULL) {
		if(__arcp_urefs(region, 0, -1)) {
			__arcp_try_destroy(region);
		}
	}
}

struct arcp_region *arcp_weakref_load(struct arcp_weakref *weakref) {
	struct arcp_region *ptr;
	struct arcp_region *desired;
	struct arcp_region *ret;

	if(weakref == NULL) {
		return NULL;
	}
	/* load the target */
	ptr = ak_load(&weakref->target, mo_consume);
	do {
	retry:
		switch(__ARCP_PTR2COUNT(ptr)) {
		case __ARCP_WEAKMAX:
			/* spinlock if too many threads are accessing this at once. */
			cpu_yield();
			ptr = ak_load(&weakref->target, mo_consume);
			goto retry;
		case __ARCP_HOHDEL:
			/* attempted deletion; treat it as 0 */
			desired = __ARCP_PTRSETCOUNT(ptr, 1);
			break;
		default:
			desired = __ARCP_PTRINC(ptr);
		}
	} while(unlikely(!ak_cas(&weakref->target, &ptr, desired,
	                         mo_acq_rel, mo_consume)));
	ptr = desired;
	/* We have one reference */
	ret = __ARCP_PTRDECOUNT(ptr);
	if(ret != NULL) {
		__arcp_urefs(ret, 0, 1);
	}
	/* We have two references, try and remove the one stored on the pointer. */
	while(unlikely(!ak_cas(&weakref->target, &ptr, __ARCP_PTRDEC(ptr),
	                       mo_acq_rel, mo_consume))) {
		/* Is the pointer still valid? */
		if((__ARCP_PTRDECOUNT(ptr) != ret)
		   || (__ARCP_PTR2COUNT(ptr) == 0)
		   || (__ARCP_PTR2COUNT(ptr) == __ARCP_HOHDEL)) {
			/* Somebody else has transferred / will transfer the
			 * count. */
			if(ret != NULL) {
				__arcp_urefs(ret, 0, -1);
			}
			break;
		}
	}
	return ret;
}

struct arcp_region *arcp_weakref_load_release(struct arcp_weakref *weakref) {
	struct arcp_region *ret;
	ret = arcp_weakref_load(weakref);
	arcp_release(weakref);
	return ret;
}

void arcp_init(arcp_t *rcp, struct arcp_region *region) {
	if(region != NULL) {
		__arcp_urefs(region, 1, 0);
	}
	ak_store(rcp, region, mo_release);
}

struct arcp_region *arcp_load(arcp_t *rcp) {
	/* YURI
 	 * We saw a sample of your manhood on the way. A place called Mink.
 	 *
 	 * PASHA (A shadow on his face)
 	 * They'd been selling horses to the whites.
 	 *
 	 * YURI
 	 * No. It seems you burned the wrong village.
 	 *
 	 * PASHA
 	 * They always say that. And what does it matter? A village betrays
 	 * us, a village is burnt. The point's made.
 	 *
 	 * ---Dr. Zhivago, 1964
 	 * (i.e. don't sweat whose reference is whose)
 	 */
	struct arcp_region *ptr;
	struct arcp_region *ret;

	ptr = ak_load(rcp, mo_acquire);
	do {
		while(unlikely(__ARCP_PTR2COUNT(ptr) == __ARCP_COUNTMASK)) {
			/* Spinlock if too many threads are accessing this
 			 * at once. */
			cpu_yield();
			ptr = ak_load(rcp, mo_consume);
		}
	} while(unlikely(!ak_cas(rcp, &ptr, __ARCP_PTRINC(ptr),
	                         mo_acq_rel, mo_consume)));
	ptr = __ARCP_PTRINC(ptr);
	/* We have one reference */
	ret = __ARCP_PTRDECOUNT(ptr);
	if(ret != NULL) {
		__arcp_urefs(ret, 0, 1);
	}
	/* We have two references, try and remove the one stored on the
 	 * pointer. */
	while(unlikely(!ak_cas(rcp, &ptr, __ARCP_PTRDEC(ptr),
	                       mo_acq_rel, mo_acquire))) {
		/* Is the pointer still valid? */
		if((__ARCP_PTRDECOUNT(ptr) != ret)
		   /* The second test will prevent a/b/a errors */
		   || (__ARCP_PTR2COUNT(ptr) == 0)) {
			/* Somebody else has transferred / will transfer the
 			 * count. */
			if(ret != NULL) {
				__arcp_urefs(ret, 0, -1);
			}
			break;
		}
	}
	return ret;
}

void arcp_store(arcp_t *rcp, struct arcp_region *region) {
	struct arcp_region *ptr;
	struct arcp_region *oldregion;

	if(region != NULL) {
		__arcp_urefs(region, 1, 0);
	}
	ptr = ak_swap(rcp, region, mo_seq_cst);
	oldregion = __ARCP_PTRDECOUNT(ptr);
	if(oldregion != NULL) {
		if(__arcp_urefs(oldregion, -1, __ARCP_PTR2COUNT(ptr))) {
			__arcp_try_destroy(oldregion);
		}
	}
}

struct arcp_region *arcp_swap(arcp_t *rcp, struct arcp_region *region) {
	struct arcp_region *ptr;
	struct arcp_region *oldregion;

	if(region != NULL) {
		__arcp_urefs(region, 1, 0);
	}
	ptr = ak_swap(rcp, region, mo_acq_rel);
	oldregion = __ARCP_PTRDECOUNT(ptr);
	if(oldregion != NULL) {
		__arcp_urefs(oldregion, -1, __ARCP_PTR2COUNT(ptr) + 1);
	}
	return oldregion;
}

bool arcp_cas(arcp_t *rcp, struct arcp_region *oldregion,
              struct arcp_region *newregion) {
	struct arcp_region *ptr;

	if(newregion != NULL) {
		__arcp_urefs(newregion, 1, 0);
	}
	ptr = ak_load(rcp, mo_acquire);
	do {
		if(__ARCP_PTRDECOUNT(ptr) != oldregion) {
			/* fail */
			if(newregion != NULL) {
				__arcp_urefs(newregion, -1, 0);
			}
			return false;
		}
	} while(unlikely(!ak_cas(rcp, &ptr, newregion,
	                         mo_acq_rel, mo_acquire)));
	/* success! */
	if(oldregion != NULL) {
		/* Transfer count */
		__arcp_urefs(oldregion, -1, __ARCP_PTR2COUNT(ptr));
		/* refcount can't reach 0 here as the caller should still own
 		 * a reference to oldptr */
	}
	return true;
}

bool arcp_cas_release(arcp_t *rcp, struct arcp_region *oldregion,
                      struct arcp_region *newregion) {
	struct arcp_region *ptr;

	if(newregion != NULL) {
		__arcp_urefs(newregion, 1, -1);
	}
	ptr = ak_load(rcp, mo_acquire);
	do {
		if(__ARCP_PTRDECOUNT(ptr) != oldregion) {
			/* fail */
			if(newregion != NULL) {
				if(__arcp_urefs(newregion, -1, 0)) {
					__arcp_try_destroy(newregion);
				}
			}
			arcp_release(oldregion);
			return false;
		}
	} while(unlikely(!ak_cas(rcp, &ptr, newregion,
	                         mo_acq_rel, mo_acquire)));
	/* success! */
	if(oldregion != NULL) {
		/* Transfer count */
		if(__arcp_urefs(oldregion, -1, __ARCP_PTR2COUNT(ptr) - 1)) {
			__arcp_try_destroy(oldregion);
		}
	}
	return true;
}
