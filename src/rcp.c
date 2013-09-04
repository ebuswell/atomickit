/*
 * rcp.c
 *
 * Copyright 2013 Evan Buswell
 * 
 * This file is part of Atomic Kit.
 * 
 * Atomic Kit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 2.
 * 
 * Atomic Kit is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "atomickit/atomic.h"
#include "atomickit/atomic-pointer.h"
#include "atomickit/atomic-malloc.h"
#include "atomickit/atomic-rcp.h"

static inline bool __arcp_try_release_destroy_lock(struct arcp_region *region) {
    uint_fast32_t count = atomic_load_explicit(&region->refcount, memory_order_consume);
    do {
	if(count == __ARCP_DESTROY_LOCK_BIT) {
	    return false;
	}
    } while(unlikely(!atomic_compare_exchange_weak_explicit(&region->refcount, &count, count ^ __ARCP_DESTROY_LOCK_BIT,
							    memory_order_acq_rel, memory_order_consume)));
    return true;
}

static void __arcp_try_destroy(struct arcp_region *region) {
    struct arcp_weak_stub *stub = (struct arcp_weak_stub *) arcp_load(&region->weak_stub);
    if(stub == NULL) {
	if(region->destroy != NULL) {
	    region->destroy(region);
	}
	return;
    }
    void *ptr = atomic_ptr_load_explicit(&stub->ptr, memory_order_consume);
    int16_t count;
retry:
    switch(count = __ARCP_PTR2COUNT(ptr)) {
    default:
	/* Transfer the count */
	if(likely(atomic_ptr_compare_exchange_strong_explicit(&stub->ptr, &ptr, __ARCP_PTRDECOUNT(ptr),
							      memory_order_acq_rel, memory_order_consume))) {
	    __arcp_urefs(region, 0, count);
	    if(likely(__arcp_try_release_destroy_lock(region))) {
		goto abort;
	    }
	    ptr = __ARCP_PTRDECOUNT(ptr);
	}
	goto retry;
    case 0:
	/* set to __ARCP_COUNTMASK */
	if(unlikely(!atomic_ptr_compare_exchange_strong_explicit(&stub->ptr, &ptr, (void *) (((uintptr_t) ptr) | __ARCP_COUNTMASK),
								 memory_order_acq_rel, memory_order_consume))) {
	    goto retry;
	}
	/* fall through */
    case __ARCP_COUNTMASK:
	/* re-check refcount */
	if(unlikely(atomic_load_explicit(&region->refcount, memory_order_consume) != __ARCP_DESTROY_LOCK_BIT)) {
	    if(likely(__arcp_try_release_destroy_lock(region))) {
		goto abort;
	    }
	    /* Otherwise, region->refcount has *become* equal to __ARCP_DESTROY_LOCK_BIT */
	}
	/* kill the reference */
	if(unlikely(!atomic_ptr_compare_exchange_strong_explicit(&stub->ptr, &ptr, NULL,
								 memory_order_acq_rel, memory_order_consume))) {
	    goto retry;
	}
    }
    arcp_store(&region->weak_stub, NULL);
    if(region->destroy != NULL) {
	region->destroy(region);
    }
abort:
    arcp_release(stub);
}

static void __arcp_destroy_weak_stub(struct arcp_weak_stub *stub) {
    afree(stub, sizeof(struct arcp_weak_stub));
}

struct arcp_region *arcp_acquire(struct arcp_region *region) {
    if(region != NULL) {
	__arcp_urefs(region, 0, 1);
    }
    return region;
}

void arcp_release(struct arcp_region *region) {
    if(region != NULL) {
	if(__arcp_urefs(region, 0, -1) == 0) {
	    __arcp_try_destroy(region);
	}
    }
}

struct arcp_weak_stub *arcp_acquire_weak_stub(struct arcp_region *region) {
    if(region == NULL) {
	return NULL;
    }
    struct arcp_weak_stub *stub = (struct arcp_weak_stub *) arcp_load(&region->weak_stub);
    if(stub != NULL) {
	return stub;
    }
    stub = amalloc(sizeof(struct arcp_weak_stub));
    if(stub == NULL) {
	return NULL;
    }
    atomic_ptr_init(&stub->ptr, region);
    arcp_region_init(stub, (void (*)(struct arcp_region *)) __arcp_destroy_weak_stub);
    if(likely(arcp_compare_store(&region->weak_stub, NULL, stub))) {
	return stub;
    } else {
	afree(stub, sizeof(struct arcp_weak_stub));
	return (struct arcp_weak_stub *) arcp_load(&region->weak_stub);
    }
}

struct arcp_region *arcp_weak_acquire(struct arcp_weak_stub *stub) {
    void *ptr = atomic_ptr_load_explicit(&stub->ptr, memory_order_consume);
    void *desired;
    do {
    retry:
	switch(__ARCP_PTR2COUNT(ptr)) {
	case __ARCP_COUNTMASK - 1:
	    /* Spinlock if too many threads are accessing this at once. */
	    cpu_relax();
	    ptr = atomic_ptr_load_explicit(&stub->ptr, memory_order_consume);
	    goto retry;
	case __ARCP_COUNTMASK:
	    desired = __ARCP_PTRDECOUNT(ptr) + 1;
	    break;
	default:
	    desired = ptr + 1;
	}
    } while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&stub->ptr, &ptr, desired,
								memory_order_acq_rel, memory_order_consume)));
    ptr = desired;
    /* We have one reference */
    struct arcp_region *ret = __ARCP_PTRDECOUNT(ptr);
    if(ret != NULL) {
	__arcp_urefs(ret, 0, 1);
    }
    /* We have two references, try and remove the one stored on the pointer. */
    while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&stub->ptr, &ptr, ptr - 1,
							      memory_order_acq_rel, memory_order_consume))) {
	/* Is the pointer still valid? */
	if((__ARCP_PTRDECOUNT(ptr) != ret)
	   || (__ARCP_PTR2COUNT(ptr) == 0)
	   || (__ARCP_PTR2COUNT(ptr) == __ARCP_COUNTMASK)) {
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

struct arcp_region *arcp_weak_load(arcp_t *rcp) {
    struct arcp_weak_stub *stub = (struct arcp_weak_stub *) arcp_load(rcp);
    if(stub == NULL) {
	return NULL;
    }
    struct arcp_region *ret = arcp_weak_acquire(stub);
    arcp_release(stub);
    return ret;
}

struct arcp_region *arcp_load(arcp_t *rcp) {
    void *ptr = atomic_ptr_load_explicit(&rcp->ptr, memory_order_acquire);
    do {
	while(unlikely(__ARCP_PTR2COUNT(ptr) == __ARCP_COUNTMASK)) {
	    /* Spinlock if too many threads are accessing this at once. */
	    cpu_relax();
	    ptr = atomic_ptr_load_explicit(&rcp->ptr, memory_order_consume);
	}
    } while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&rcp->ptr, &ptr, ptr + 1,
								memory_order_acq_rel, memory_order_consume)));
    ptr += 1;
    /* We have one reference */
    struct arcp_region *ret = __ARCP_PTRDECOUNT(ptr);
    if(ret != NULL) {
	__arcp_urefs(ret, 0, 1);
    }
    /* We have two references, try and remove the one stored on the pointer. */
    while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&rcp->ptr, &ptr, ptr -1,
							      memory_order_acq_rel, memory_order_acquire))) {
	/* Is the pointer still valid? */
	if((__ARCP_PTRDECOUNT(ptr) != ret)
	   || (__ARCP_PTR2COUNT(ptr) == 0)) { /* The second test will prevent a/b/a errors */
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
    if(region != NULL) {
	__arcp_urefs(region, 1, 0);
    }
    void *ptr = atomic_ptr_exchange(&rcp->ptr, region);
    struct arcp_region *oldregion = __ARCP_PTRDECOUNT(ptr);
    if(oldregion != NULL) {
	if(__arcp_urefs(oldregion, -1, __ARCP_PTR2COUNT(ptr)) == 0) {
	    __arcp_try_destroy(oldregion);
	}
    }
}

struct arcp_region *arcp_exchange(arcp_t *rcp, struct arcp_region *region) {
    if(region != NULL) {
	__arcp_urefs(region, 1, 0);
    }
    void *ptr = atomic_ptr_exchange_explicit(&rcp->ptr, region, memory_order_acq_rel);
    struct arcp_region *oldregion = __ARCP_PTRDECOUNT(ptr);
    if(oldregion != NULL) {
	__arcp_urefs(oldregion, -1, __ARCP_PTR2COUNT(ptr) + 1);
    }
    return oldregion;
}

bool arcp_compare_store(arcp_t *rcp, struct arcp_region *oldregion, struct arcp_region *newregion) {
    if(newregion != NULL) {
	__arcp_urefs(newregion, 1, 0);
    }
    void *ptr = atomic_ptr_load_explicit(&rcp->ptr, memory_order_acquire);
    do {
	if(__ARCP_PTRDECOUNT(ptr) != oldregion) {
	    /* fail */
	    if(newregion != NULL) {
		__arcp_urefs(newregion, -1, 0);
	    }
	    return false;
	}
    } while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&rcp->ptr, &ptr, newregion,
								memory_order_acq_rel, memory_order_acquire)));
    /* success! */
    if(oldregion != NULL) {
	/* Transfer count */
	__arcp_urefs(oldregion, -1, __ARCP_PTR2COUNT(ptr));
	/* refcount can't reach 0 here as the caller should still own a reference to oldptr */
    }
    return true;
}

bool arcp_compare_store_release(arcp_t *rcp, struct arcp_region *oldregion, struct arcp_region *newregion) {
    if(newregion != NULL) {
	__arcp_urefs(newregion, 1, -1);
    }
    void *ptr = atomic_ptr_load_explicit(&rcp->ptr, memory_order_acquire);
    do {
	if(__ARCP_PTRDECOUNT(ptr) != oldregion) {
	    /* fail */
	    if(newregion != NULL) {
		if(__arcp_urefs(newregion, -1, 0) == 0) {
		    __arcp_try_destroy(newregion);
		}
	    }
	    arcp_release(oldregion);
	    return false;
	}
    } while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&rcp->ptr, &ptr, newregion,
								memory_order_acq_rel, memory_order_acquire)));
    /* success! */
    if(oldregion != NULL) {
	/* Transfer count */
	if(__arcp_urefs(oldregion, -1, __ARCP_PTR2COUNT(ptr) - 1) == 0) {
	    __arcp_try_destroy(oldregion);
	}
    }
    return true;
}
