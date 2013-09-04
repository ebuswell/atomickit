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

struct arcp_region;

struct arcp_weak_stub;

/**
 * Atomic Reference Counted Region
 *
 * The underlying data type which will be stored in the reference
 * counted pointer.  Good code will not depend on the content of this
 * struct.
 *
 * A region may be stored in an arbitrary number of reference counted
 * pointers.
 */
struct arcp_region {
    void (*destroy)(struct arcp_region *); /** Pointer to a
					    * destruction function. */
    atomic_uint_fast32_t refcount; /** High 16 bits, number of
				    * pointers in which this is
				    * referenced; low 16 bits, the
				    * number of individual references
				    * to this region. */
    arcp_t weak_stub; /** Pointer to the weak stub for this region;
		       * initially NULL. */
} __attribute__((aligned(16)));

/**
 * Weak Stub for Atomic Reference Counted Region
 *
 * A weak stub contains a weak reference to an `arcp_region` that may
 * be turned into a strong reference.  It is itself an `arcp_region`
 * and can/should be stored in `arcp_t` containers.
 */
struct arcp_weak_stub {
    struct arcp_region;
    atomic_ptr ptr;
};

/* Used for miscellaneous alignment purposes */
struct __arcp_region_data {
    struct arcp_region;
    uint8_t data[] __attribute__((aligned(16))); /** User-defined data. */
};

/**
 * \def ARCP_ALIGN The alignment of the data portion of an rcp region.
 * The maximum number of threads concurrently checking out a given
 * item from a given transaction (not the number who concurrenty have
 * the item checked out, which should be sufficient for all purposes)
 * will be equal to `ARCP_ALIGN - 1`. Check out will block for all
 * threads above this threshold.
 */
#define ARCP_ALIGN __alignof__(struct arcp_region *)

/**
 * The size of the reference counted region that is not the user data.
 */
#define ARCP_REGION_OVERHEAD (sizeof(struct __arcp_region_data))

/* The mask for the transaction count. */
#define __ARCP_COUNTMASK ((uintptr_t) (ARCP_ALIGN - 1))

/* Gets the count for a given pointer. */
#define __ARCP_PTR2COUNT(ptr) (((uintptr_t) (ptr)) & __ARCP_COUNTMASK)

/* Separates the pointer from the count */
#define __ARCP_PTRDECOUNT(ptr) ((struct arcp_region *) (((uintptr_t) (ptr)) & ~__ARCP_COUNTMASK))

/**
 * Transforms a pointer to user data to a pointer to the corresponding
 * reference counted region.
 */
#define ARCP_DATA2REGION(ptr) ((struct arcp_region *) (((uintptr_t) (ptr)) - offsetof(struct __arcp_region_data, data)))

/**
 * Transforms a pointer to a reference counted region to a pointer to
 * the corresponding user data.
 */
#define ARCP_REGION2DATA(region) ((void *) &(((struct __arcp_region_data *) region))->data)

/**
 * Initialization value for `arcp_t`.
 */
#define ARCP_VAR_INIT(region) { ATOMIC_PTR_VAR_INIT(region) }

/**
 * Initialization value for `struct arcp_region`.
 */
#define ARCP_REGION_VAR_INIT(ptrcount, refcount, destroy) { destroy, ATOMIC_VAR_INIT((ptrcount << 16) | refcount), ARCP_VAR_INIT(NULL) }

#define __ARCP_LITTLE_ENDIAN (((union {		\
		    uint_fast32_t v;		\
		    struct {			\
			int16_t first;		\
			int16_t second;		\
		    } s;			\
    }) { 1 }).s.first)

#define __ARCP_DESTROY_LOCK_BIT ((uint_fast32_t) (1 << 31))

static inline uint_fast32_t __arcp_urefs(struct arcp_region *region, int16_t ptrcount, int16_t refcount) {
    union {
	uint_fast32_t v;
	struct {
	    int16_t first;
	    int16_t second;
	} s;
    } count;
    uint_fast32_t o_count = atomic_load_explicit(&region->refcount, memory_order_consume);
    uint_fast32_t ret;
    do {
	count.v = o_count;
	if(__ARCP_LITTLE_ENDIAN) {
	    count.s.first += refcount;
	    count.s.second += ptrcount;
	} else {
	    count.s.first += ptrcount;
	    count.s.second += refcount;
	}
	ret = count.v;
	if(count.v == 0) {
	    count.v |= __ARCP_DESTROY_LOCK_BIT;
	}
    } while(unlikely(!atomic_compare_exchange_weak_explicit(&region->refcount, &o_count, count.v,
							    memory_order_acq_rel, memory_order_consume)));
    return ret;
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
static inline void arcp_region_init(struct arcp_region *region, void (*destroy)(struct arcp_region *));

/**
 * Returns the current reference count of the region.
 *
 * @param region a pointer to a reference counted region.
 *
 * @returns the reference count of the region.
 */
static inline int arcp_refcount(struct arcp_region *region);

/**
 * Returns the current pointer reference count of the region.
 *
 * @param region a pointer to a reference counted region.
 *
 * @returns the number of pointers in which the region is currently
 * stored.
 */
static inline int arcp_ptrcount(struct arcp_region *region);

/**
 * Increments the reference count of a reference counted region,
 * returning that region.
 *
 * In most cases, as references are never implicitly stolen, this need
 * not be used.  The exceptions are if a pointer to a region needs to
 * be stored somewhere where there's not contention.
 *
 * @param region the region whose reference count should be incremented.
 */
struct arcp_region *arcp_acquire(struct arcp_region *region);

/**
 * Releases a reference to a reference counted region.
 *
 * @param region the region to release a reference to.
 */
void arcp_release(struct arcp_region *region);

/**
 * Acquires the weak stub referencing a given arcp_region, creating if
 * necessary.
 *
 * One must hold a (strong) reference to the arcp_region in order to
 * call this function.
 *
 * @param region the region for which to acquire a weak stub
 *
 * @returns the weak stub corresponding to this region, or NULL if an
 * error occurred, or if the region was originally NULL.
 */
struct arcp_weak_stub *arcp_acquire_weak_stub(struct arcp_region *region);

/**
 * Acquires a strong reference to the region weakly referenced by a
 * weak stub.
 *
 * @param stub the stub from which to load the region.
 *
 * @returns the enclosed region or NULL if the region has been destroyed.
 */
struct arcp_region *arcp_weak_acquire(struct arcp_weak_stub *stub);

/**
 * Acquires a strong reference to a region whose weak stub is
 * currently stored in a reference counted pointer.
 *
 * @param rcp the reference counted pointer in which the weak reference is stored.
 *
 * @returns the referenced region or NULL if the region has been destroyed.
 */
struct arcp_region *arcp_weak_load(arcp_t *rcp);

/**
 * Initializes a reference counted pointer.
 *
 * @param rcp a pointer to the reference counted pointer being
 * initialized.
 * @param region initial contents of the pointer. This should be a
 * pointer to an initialized `arcp_region`, or `NULL`.
 */
static inline void arcp_init(arcp_t *rcp, struct arcp_region *region);

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
void arcp_store(arcp_t *rcp, struct arcp_region *region);

/**
 * Acquire a reference counted pointer's contents.
 *
 * @param rcp the pointer from which to acquire the current contents.
 *
 * @returns pointer to the region currently stored in the reference
 * counted pointer.
 */
struct arcp_region *arcp_load(arcp_t *rcp);

/**
 * Load a reference counted pointer's contents without affecting the
 * reference count.
 *
 * @param rcp the pointer for which to look at the current contents.
 *
 * @returns the contents of the pointer.
 */
static inline struct arcp_region *arcp_load_phantom(arcp_t *rcp);

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
struct arcp_region *arcp_exchange(arcp_t *rcp, struct arcp_region *region);

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
bool arcp_compare_store(arcp_t *rcp, struct arcp_region *oldregion, struct arcp_region *newregion);

bool arcp_compare_store_release(arcp_t *rcp, struct arcp_region *oldregion, struct arcp_region *newregion);

static inline void arcp_region_init(struct arcp_region *region, void (*destroy)(struct arcp_region *)) {
    atomic_init(&region->refcount, 1);
    region->destroy = destroy;
    arcp_init(&region->weak_stub, NULL);
}

static inline int arcp_refcount(struct arcp_region *region) {
    union {
	uint_fast32_t v;
	struct {
	    int16_t first;
	    int16_t second;
	} s;
    } count;
    count.v = atomic_load_explicit(&region->refcount, memory_order_consume);
    if(__ARCP_LITTLE_ENDIAN) {
	return count.s.first; /* low */
    } else {
	return count.s.second; /* low */
    }
}

static inline int arcp_ptrcount(struct arcp_region *region) {
    union {
	uint_fast32_t v;
	struct {
	    int16_t first;
	    int16_t second;
	} s;
    } count;
    count.v = atomic_load_explicit(&region->refcount, memory_order_consume) & (~__ARCP_DESTROY_LOCK_BIT);
    if(__ARCP_LITTLE_ENDIAN) {
	return count.s.second; /* high */
    } else {
	return count.s.first; /* high */
    }
}

static inline void arcp_init(arcp_t *rcp, struct arcp_region *region) {
    if(region != NULL) {
	__arcp_urefs(region, 1, 0);
    }
    atomic_ptr_store_explicit(&rcp->ptr, region, memory_order_release);
}

static inline struct arcp_region *arcp_load_phantom(arcp_t *rcp) {
    return __ARCP_PTRDECOUNT(atomic_ptr_load_explicit(&rcp->ptr, memory_order_acquire));
}

#endif /* ! ATOMICKIT_ATOMIC_RCP_H */
