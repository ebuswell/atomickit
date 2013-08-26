/** @file atomic-rcp.h
 * Atomic Reference Counted Pointer
 *
 * Atomic Reference Counted Pointers (RCPs) are pointers to
 * reference-counted memory regions.  Among other things, they can be
 * used to eliminate the so-called ABA problem.
 *
 * The memory deallocation functions are all user defined.  In many
 * cases, this will simply be `afree`.
 *
 * References are *never* implicitly released.  The calling function
 * should *always* hold a reference to the item and release that
 * reference when it is no longer needed.
 */
/*
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
#ifndef ATOMICKIT_ATOMIC_RCP_H
#define ATOMICKIT_ATOMIC_RCP_H 1

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <atomickit/atomic.h>
#include <atomickit/atomic-pointer.h>

/**
 * Atomic Reference Counted Pointer
 *
 * The pointer to a reference counted memory region.
 */
typedef struct {
    atomic_ptr/*<struct arcp_item *>*/ ptr;
} arcp_t;

/**
 * \def ARCP_ALIGN The alignment of the data portion of an rcp region.
 * The maximum number of threads concurrently checking out a given
 * item from a given transaction (not the number who concurrenty have
 * the item checked out, which should be sufficient for all purposes)
 * will be equal to `ARCP_ALIGN - 1`. Check out will block for all
 * threads above this threshold.
 */
#ifdef __LP64__
# define ARCP_ALIGN 8
#else
# define ARCP_ALIGN 4
#endif

struct arcp_region;

/**
 * Atomic Reference Counted Region Header
 *
 * The metadata needed for reference counting.
 */
struct arcp_region_header {
    atomic_uint_fast32_t refcount; /** High 16 bits, number of
				    * pointers in which this is
				    * referenced; low 16 bits, the
				    * number of individual references
				    * to this region. */
    void (*destroy)(struct arcp_region *); /** Pointer to a
					    * destruction function. */
};

/**
 * Atomic Reference Counted Region
 *
 * The underlying data type which will be stored in the reference
 * counted pointer.  In most cases, this will be automatically managed
 * and you will only be dealing with a pointer to the actual data
 * region.  Good code will not depend on the content of this struct.
 *
 * A region may be stored in an arbitrary number of reference counted
 * pointers.
 */
struct arcp_region {
    struct arcp_region_header header;
    uint8_t data[1]; /** User-defined data. */
};

/**
 * The size of the reference counted region that is not the user data.
 */
#define ARCP_REGION_OVERHEAD (offsetof(struct arcp_region, data))

/* The mask for the transaction count. */
#define ARCP_COUNTMASK ((uintptr_t) (ARCP_ALIGN - 1))

/* Gets the count for a given pointer. */
#define ARCP_PTR2COUNT(ptr) (((uintptr_t) (ptr)) & ARCP_COUNTMASK)

/* Separates the pointer from the count */
#define ARCP_PTRDECOUNT(ptr) ((struct arcp_region *) (((uintptr_t) (ptr)) & ~ARCP_COUNTMASK))

/**
 * Transforms a pointer to user data to a pointer to the corresponding
 * reference counted region.
 */
#define ARCP_DATA2REGION(ptr) ((struct arcp_region *) (((intptr_t) (ptr)) - offsetof(struct arcp_region, data)))

/**
 * Transforms a pointer to a reference counted region to a pointer to
 * the corresponding user data.
 */
#define ARCP_REGION2DATA(region) ((void *) &(region)->data)

/**
 * Initialization value for `arcp_t`.
 */
#define ARCP_VAR_INIT(region) { ATOMIC_PTR_VAR_INIT(region) }

/**
 * Initialization value for `struct arcp_header`.
 */
#define ARCP_REGION_HEADER_VAR_INIT(ptrcount, refcount, destroy) { ATOMIC_VAR_INIT((ptrcount << 16) | refcount), destroy }

#define __ARCP_LITTLE_ENDIAN (((union {		\
		    uint_fast32_t v;		\
		    struct {			\
			int16_t first;		\
			int16_t second;		\
		    } s;			\
    }) { 1 }).s.first)

static inline uint_fast32_t __arcp_urefs(struct arcp_region *region, int16_t ptrcount, int16_t refcount) {
    union {
	uint_fast32_t v;
	struct {
	    int16_t first;
	    int16_t second;
	} s;
    } count;
    uint_fast32_t o_count = atomic_load_explicit(&region->header.refcount, memory_order_acquire);
    do {
	count.v = o_count;
	if(__ARCP_LITTLE_ENDIAN) {
	    count.s.first += refcount;
	    count.s.second += ptrcount;
	} else {
	    count.s.first += ptrcount;
	    count.s.second += refcount;
	}
    } while(unlikely(!atomic_compare_exchange_weak_explicit(&region->header.refcount, &o_count, count.v,
							    memory_order_acq_rel, memory_order_acquire)));
    return count.v;
}

/**
 * Initializes a reference counted region.
 *
 * The region has a single reference after initialization, belonging
 * to the calling function.
 *
 * @param region a pointer to a reference counted region.  This is
 * typically allocated with `amalloc(ARCP_REGION_OVERHEAD + <user data
 * size>).
 * @param destroy pointer to a function that will destroy the region
 * once it is no longer in use.  If no cleanup is needed, this could
 * just be a simple wrapper around `afree`.
 */
static inline void arcp_region_init(struct arcp_region *region, void (*destroy)(struct arcp_region *)) {
    atomic_init(&region->header.refcount, 1);
    region->header.destroy = destroy;
}

/**
 * Returns the current reference count of the region.
 *
 * @param region a pointer to a reference counted region.
 *
 * @returns the reference count of the region.
 */
static inline int arcp_refcount(struct arcp_region *region) {
    union {
	uint_fast32_t v;
	struct {
	    int16_t first;
	    int16_t second;
	} s;
    } count;
    count.v = atomic_load_explicit(&region->header.refcount, memory_order_acquire);
    if(__ARCP_LITTLE_ENDIAN) {
	return count.s.first; /* low */
    } else {
	return count.s.second; /* low */
    }
}

/**
 * Returns the current pointer reference count of the region.
 *
 * @param region a pointer to a reference counted region.
 *
 * @returns the number of pointers in which the region is currently
 * stored.
 */
static inline int arcp_ptrcount(struct arcp_region *region) {
    union {
	uint_fast32_t v;
	struct {
	    int16_t first;
	    int16_t second;
	} s;
    } count;
    count.v = atomic_load_explicit(&region->header.refcount, memory_order_acquire);
    if(__ARCP_LITTLE_ENDIAN) {
	return count.s.second; /* high */
    } else {
	return count.s.first; /* high */
    }
}

/**
 * Increments the reference count of a reference counted region.
 *
 * In most cases, as references are never implicitly stolen, this need
 * not be used.  The exceptions are if a pointer to a region needs to
 * be stored somewhere where there's not contention.
 *
 * @param region the region whose reference count should be incremented.
 */
static inline struct arcp_region *arcp_incref(struct arcp_region *region) {
    if(region != NULL) {
	__arcp_urefs(region, 0, 1);
    }
    return region;
}

/**
 * Releases a reference to a reference counted region.
 *
 * @param region the region to release a reference to.
 */
static inline void arcp_release(struct arcp_region *region) {
    if(region != NULL) {
	/* if count is not transferred before this call, count will go
	 * negative instead of 0 and the transferer will be
	 * responsible for freeing the variable, if needed. */
	if(__arcp_urefs(region, 0, -1) == 0) {
	    if(region->header.destroy != NULL) {
		region->header.destroy(region);
	    }
	}
    }
}

/**
 * Initializes a reference counted pointer.
 *
 * @param rcp a pointer to the reference counted pointer being
 * initialized.
 * @param region initial contents of the pointer. This should be a
 * pointer to an initialized `arcp_region`, or `NULL`.
 */
static inline void arcp_init(arcp_t *rcp, struct arcp_region *region) {
    if(region != NULL) {
	__arcp_urefs(region, 1, 0);
    }
    atomic_ptr_store(&rcp->ptr, region); /* full barrier */
}

/**
 * Store a new region as the content of the reference counted pointer.
 *
 * The caller's references are untouched, so call
 * `arcp_region_release()` on newregion if you no longer intend to
 * reference it.
 *
 * @param rcp the pointer for which to commit the new content.
 * @param region the new content of the pointer.
 */
static inline void arcp_store(arcp_t *rcp, struct arcp_region *region) {
    if(region != NULL) {
	__arcp_urefs(region, 1, 0);
    }
    void *ptr = atomic_ptr_exchange(&rcp->ptr, region);
    struct arcp_region *oldregion = ARCP_PTRDECOUNT(ptr);
    if(oldregion != NULL) {
	if(__arcp_urefs(oldregion, -1, ARCP_PTR2COUNT(ptr)) == 0) {
	    if(oldregion->header.destroy != NULL) {
		oldregion->header.destroy(oldregion);
	    }
	}
    }
}

/**
 * Acquire a reference counted pointer's contents.
 *
 * @param rcp the pointer from which to acquire the current contents.
 *
 * @returns pointer to the region currently stored in the reference
 * counted pointer.
 */
static inline struct arcp_region *arcp_load(arcp_t *rcp) {
    void *ptr = atomic_ptr_load_explicit(&rcp->ptr, memory_order_acquire);
    do {
	while(unlikely(ARCP_PTR2COUNT(ptr) == ARCP_ALIGN - 1)) {
	    /* Spinlock if too many threads are accessing this at once. */
	    cpu_relax();
	    ptr = atomic_ptr_load_explicit(&rcp->ptr, memory_order_acquire);
	}
    } while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&rcp->ptr, &ptr, ptr + 1,
								memory_order_acq_rel, memory_order_acquire)));
    ptr += 1;
    /* We have one reference */
    struct arcp_region *ret = ARCP_PTRDECOUNT(ptr);
    if(ret != NULL) {
	__arcp_urefs(ret, 0, 1);
    }
    /* We have two references, try and remove the one stored on the pointer. */
    while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&rcp->ptr, &ptr, ptr - 1,
							      memory_order_acq_rel, memory_order_acquire))) {
	/* Is the pointer still valid? */
	if((ARCP_PTRDECOUNT(ptr) != ret)
	   || (ARCP_PTR2COUNT(ptr) == 0)) { /* The second test will prevent a/b/a errors */
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

/**
 * Load a reference counted pointer's contents without affecting the
 * reference count.
 *
 * @param rcp the pointer for which to look at the current contents.
 *
 * @returns the contents of the pointer.
 */
static inline struct arcp_region *arcp_load_weak(arcp_t *rcp) {
    return ARCP_PTRDECOUNT(atomic_ptr_load_explicit(&rcp->ptr, memory_order_acquire));
}

/**
 * Exchange a new region with the content of the reference counted
 * pointer.
 *
 * This is unconditional and should only be used in special
 * circumstances. The caller's references are untouched, so call
 * `arcp_region_release()` on newregion if you no longer intend to
 * reference it.
 *
 * @param rcp the pointer for which to commit the new content.
 * @param region the new content of the transaction.
 *
 * @returns the old contents of rcp.
 */
static inline struct arcp_region *arcp_exchange(arcp_t *rcp, struct arcp_region *region) {
    if(region != NULL) {
	__arcp_urefs(region, 1, 0);
    }
    void *ptr = atomic_ptr_exchange(&rcp->ptr, region); /* full barrier */
    struct arcp_region *oldregion = ARCP_PTRDECOUNT(ptr);
    if(oldregion != NULL) {
	__arcp_urefs(oldregion, -1, ARCP_PTR2COUNT(ptr) + 1);
    }
    return oldregion;
}

/**
 * Compare the content of the reference counted pointer with
 * `oldregion`, and if equal set its content to `newregion`.
 *
 * Only succeeds if `oldregion` is still the content of the
 * transaction. The caller's references are untouched, so call
 * `arcp_region_release()` on oldregion and/or newregion if you no
 * longer intend to reference them. Note that the semantics of
 * updating oldregion that might be expected in analogy to
 * `atomic_compare_exchange_*` do not hold, as updating oldregion
 * cannot be implemented as a trivial side-effect of this operation.
 *
 * @param rcp the pointer for which to commit the new content.
 * @param oldregion the previous content of the transaction.
 * @param newregion the new content of the transaction.
 *
 * @returns true if set to newregion, false otherwise.
 */
static inline bool arcp_compare_store(arcp_t *rcp, struct arcp_region *oldregion, struct arcp_region *newregion) {
    if(newregion != NULL) {
	__arcp_urefs(newregion, 1, 0);
    }
    void *ptr = atomic_ptr_load_explicit(&rcp->ptr, memory_order_acquire);
    do {
	if(ARCP_PTRDECOUNT(ptr) != oldregion) {
	    /* fail */
	    if(newregion != NULL) {
		__arcp_urefs(newregion, -1, 0);
	    }
	    return false;
	}
    } while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&rcp->ptr, &ptr, newregion,
								memory_order_seq_cst, memory_order_acquire))); /* full barrier */
    /* success! */
    if(oldregion != NULL) {
	/* Transfer count */
	__arcp_urefs(oldregion, -1, ARCP_PTR2COUNT(ptr));
	/* refcount can't reach 0 here as the caller should still own a reference to oldptr */
    }
    return true;
}

static inline bool arcp_compare_store_release(arcp_t *rcp, struct arcp_region *oldregion, struct arcp_region *newregion) {
    if(newregion != NULL) {
	__arcp_urefs(newregion, 1, -1);
    }
    void *ptr = atomic_ptr_load_explicit(&rcp->ptr, memory_order_acquire);
    do {
	if(ARCP_PTRDECOUNT(ptr) != oldregion) {
	    /* fail */
	    if(newregion != NULL) {
		if(__arcp_urefs(newregion, -1, 0) == 0) {
		    if(newregion->header.destroy != NULL) {
			newregion->header.destroy(newregion);
		    }
		}
	    }
	    arcp_release(oldregion);
	    return false;
	}
    } while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&rcp->ptr, &ptr, newregion,
								memory_order_seq_cst, memory_order_acquire))); /* full barrier */
    /* success! */
    if(oldregion != NULL) {
	/* Transfer count */
	if(__arcp_urefs(oldregion, -1, ARCP_PTR2COUNT(ptr) - 1) == 0) {
	    if(oldregion->header.destroy != NULL) {
		oldregion->header.destroy(oldregion);
	    }
	}
    }
    return true;
}

#endif /* ! ATOMICKIT_ATOMIC_RCP_H */
