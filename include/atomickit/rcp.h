/** @file rcp.h
 * Atomic Reference Counted Pointer
 *
 * Atomic Reference Counted Pointers (RCPs) are pointers to reference-counted
 * memory regions.  Among other things, they can be used to eliminate the
 * so-called ABA problem.
 *
 * The memory deallocation functions are all user defined.  In many cases,
 * this will simply be `afree`.
 *
 * References are *never* implicitly released.  The calling function should
 * *always* hold a reference to the item and release that reference when it is
 * no longer needed.
 */
/*
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
 * You should have received a copy of the GNU General Public License along
 * with Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef ATOMICKIT_RCP_H
#define ATOMICKIT_RCP_H 1

#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <atomickit/atomic.h>

struct arcp_region;

/**
 * Atomic Reference Counted Pointer
 *
 * The pointer to a reference counted memory region.
 */
typedef _Atomic(struct arcp_region *) arcp_t;

typedef void (*arcp_destroy_f)(struct arcp_region *);

typedef union {
	struct {
		unsigned int destroy_lock:1; /** Lock during attempted
					      *  destruction */
		int storecount:15; /** Number of pointers referencing this */
		int16_t usecount; /** Number of checked out references */
	} v;
	uint_least32_t p;
} arcp_refcount_t;

/**
 * Atomic Reference Counted Region
 *
 * The underlying data type which will be stored in the reference counted
 * pointer.  Good code will not depend on the content of this struct.
 *
 * A region may be stored in an arbitrary number of reference counted
 * pointers.
 */
struct arcp_region {
	arcp_destroy_f destroy; /** Pointer to a destruction function. */
	atomic_uint_least32_t refcount; /** References to this region. */
	arcp_t weakref; /** Pointer to the weak reference for this
			 *  region; initially NULL. */
};

/**
 * Weak Reference for an Atomic Reference Counted Region
 *
 * A weak reference is a reference to an `arcp_region` that may be turned into
 * a strong reference.  It is itself an `arcp_region` and can/should be stored
 * in `arcp_t` containers.
 */
struct arcp_weakref {
	struct arcp_region;
	arcp_t target;
};

/**
 * \def ARCP_ALIGN The alignment of the data portion of an rcp region. The
 * maximum number of threads concurrently checking out a given item from a
 * given transaction (not the number who concurrenty have the item checked
 * out, which should be sufficient for all purposes) will be equal to
 * `ARCP_ALIGN - 1`. Check out will block for all threads above this
 * threshold.
 */
#define ARCP_ALIGN alignof(struct arcp_region *)

/* The mask for the transaction count. */
#define __ARCP_COUNTMASK ((uintptr_t) (ARCP_ALIGN - 1))

/* Gets the count for a given pointer. */
#define __ARCP_PTR2COUNT(ptr) (((uintptr_t) (ptr)) & __ARCP_COUNTMASK)

/* Sets the count for a given pointer. */
#define __ARCP_PTRSETCOUNT(ptr, count)	 				\
	((struct arcp_region *) 					\
	 ((((uintptr_t) (ptr)) & ~__ARCP_COUNTMASK) | ((uintptr_t) (count))))

/* Increments the count for a given pointer. */
#define __ARCP_PTRINC(ptr)	 					\
	((struct arcp_region *) 					\
	 (((uintptr_t) (ptr)) + 1))

/* Decrements the count for a given pointer. */
#define __ARCP_PTRDEC(ptr)	 					\
	((struct arcp_region *) 					\
	 (((uintptr_t) (ptr)) - 1))

/* Separates the pointer from the count */
#define __ARCP_PTRDECOUNT(ptr)						\
	((struct arcp_region *)						\
	 (((uintptr_t) (ptr)) & ~__ARCP_COUNTMASK))

/**
 * Initialization value for `arcp_t`.
 */
#define ARCP_VAR_INIT(region) ATOMIC_VAR_INIT(region)

#define __ARCP_REFCOUNT_INIT(scount, ucount)				\
	(((arcp_refcount_t)						\
	  { .v = { .destroy_lock = 0,					\
	           .storecount = (scount),				\
	           .usecount = (ucount) }}).p)

/**
 * Initialization value for `struct arcp_region`.
 */
#define ARCP_REGION_VAR_INIT(storecount, usecount, destroy, weakref)	\
	{ destroy, ATOMIC_VAR_INIT(__ARCP_REFCOUNT_INIT((storecount),	\
	                                                (usecount))),	\
	  ARCP_VAR_INIT(weakref) }

/**
 * Initialization value for `struct arcp_weakref`.
 */
#define ARCP_WEAKREF_VAR_INIT(storecount, usecount, destroy, ptr)	\
	{ ARCP_REGION_VAR_INIT(storecount, usecount, destroy, NULL),	\
	  ARCP_VAR_INIT(ptr) }

/**
 * Initializes a reference counted region.
 *
 * The region has a single reference after initialization, belonging to the
 * calling function.
 *
 * @param region a pointer to a reference counted region.  This is typically
 * allocated with `amalloc(ARCP_REGION_OVERHEAD + <user data size>).
 * @param destroy pointer to a function that will destroy the region once it
 * is no longer in use.  If no cleanup is needed, this could just be a simple
 * wrapper around `afree`.
 */
void arcp_region_init(struct arcp_region *region, arcp_destroy_f destroy);

/**
 * Initializes the weak reference for a reference counted region.
 *
 * @param region a pointer to a reference counted region.
 *
 * @returns zero on success, nonzero on failure.
 */
int arcp_region_init_weakref(struct arcp_region *region);

/**
 * Destroys the weak reference for a reference counted region.
 *
 * This is automatically called when applicable after the user destruction
 * function, but is still provided for when deleting a region by a custom
 * mechanism.
 *
 * @param region a pointer to a reference counted region.
 */
void arcp_region_destroy_weakref(struct arcp_region *region);

/**
 * Returns the current checked out reference count of the region.
 *
 * @param region a pointer to a reference counted region.
 *
 * @returns the reference count of the region.
 */
static inline int arcp_usecount(struct arcp_region *region);

/**
 * Returns the current stored reference count of the region.
 *
 * @param region a pointer to a reference counted region.
 *
 * @returns the number of pointers in which the region is currently stored.
 */
static inline int arcp_storecount(struct arcp_region *region);

/**
 * Increments the reference count of a reference counted region, returning
 * that region.
 *
 * In most cases, as references are never implicitly stolen, this need not be
 * used.  The exceptions are if a pointer to a region needs to be stored
 * somewhere where there's not contention.
 *
 * @param region the region whose reference count should be incremented.
 */
#define arcp_acquire(region) ((typeof(region)) __arcp_acquire(region))
struct arcp_region *__arcp_acquire(struct arcp_region *region);

/**
 * Returns a weak reference to this region.
 *
 * You must call `arcp_region_init_weakref` before this.
 *
 * @param region the region to which to return a weak reference.
 *
 * @returns a weak reference to this region.
 */
struct arcp_weakref *arcp_weakref(struct arcp_region *region);

/**
 * Returns a weak reference to this region.
 *
 * You must call `arcp_region_init_weakref` before this.
 *
 * @param region the region to which to return a weak reference.
 *
 * @returns a weak reference to this region.
 */
static inline struct arcp_weakref *
arcp_weakref_phantom(struct arcp_region *region);

/**
 * Releases a reference to a reference counted region.
 *
 * @param region the region to release a reference to.
 */
void arcp_release(struct arcp_region *region);

/**
 * Loads the item to which the weak reference refers.
 *
 * @param weakref the weak reference for which to load the item.
 *
 * @returns the item to which the weak reference refers, or NULL if the item
 * has been destroyed.
 */
struct arcp_region *arcp_weakref_load(struct arcp_weakref *weakref);

struct arcp_region *arcp_weakref_load_release(struct arcp_weakref *weakref);

/**
 * Initializes a reference counted pointer.
 *
 * @param rcp a pointer to the reference counted pointer being initialized.
 * @param region initial contents of the pointer. This should be a pointer to
 * an initialized `arcp_region`, or `NULL`.
 */
void arcp_init(arcp_t *rcp, struct arcp_region *region);

/**
 * Store a new region as the content of the reference counted pointer.
 *
 * The caller's references are untouched, so call `arcp_region_release()` on
 * newregion if you no longer intend to reference it.
 *
 * @param rcp the pointer for which to commit the new content.
 * @param region the new content of the pointer.
 *
 * @returns zero on success, nonzero on failure.
 */
void arcp_store(arcp_t *rcp, struct arcp_region *region);

/**
 * Store a weak reference to a region as the content of the regerence counted
 * pointer.
 *
 * The caller's references are untouched, so call `arcp_region_release()` on
 * newregion if you no longer intend to reference it.
 *
 * @param rcp the pointer for which to commit the new content.
 * @param region the region to store the weak reference to.
 */
void arcp_store_weak(arcp_t *rcp, struct arcp_region *region);

/**
 * Acquire a reference counted pointer's contents.
 *
 * @param rcp the pointer from which to acquire the current contents.
 *
 * @returns pointer to the region currently stored in the reference counted
 * pointer.
 */
struct arcp_region *arcp_load(arcp_t *rcp);

/**
 * Load a reference counted pointer's contents without affecting the reference
 * count.
 *
 * @param rcp the pointer for which to look at the current contents.
 *
 * @returns the contents of the pointer.
 */
static inline struct arcp_region *arcp_load_phantom(arcp_t *rcp);

/**
 * Acquires a strong reference to a region whose weak stub is currently stored
 * in a reference counted pointer.
 *
 * @param rcp the reference counted pointer in which the weak reference is
 * stored.
 *
 * @returns the referenced region or NULL if the region has been destroyed.
 */
struct arcp_region *arcp_load_weak(arcp_t *rcp);

/**
 * Exchange a new region with the content of the reference counted pointer.
 *
 * This is unconditional and should only be used in special circumstances. The
 * caller's references are untouched, so call `arcp_region_release()` on
 * newregion if you no longer intend to reference it.
 *
 * @param rcp the pointer for which to commit the new content.
 * @param region the new content of the transaction.
 *
 * @returns the old contents of rcp.
 */
struct arcp_region *arcp_swap(arcp_t *rcp, struct arcp_region *region);

/**
 * Atomically store a weak reference for a pointer while acquiring a strong
 * reference to the previous contents.
 *
 * This is unconditional and should only be used in special circumstances. The
 * caller's references are untouched, so call `arcp_region_release()` on
 * newregion if you no longer intend to reference it.
 *
 * @param rcp the pointer for which to commit the new content.
 * @param region the new content of the transaction.
 *
 * @returns a strong reference to the old contents of rcp, or NULL on error.
 */
struct arcp_region *arcp_exchange_weak(arcp_t *rcp,
                                       struct arcp_region *region);

/**
 * Compare the content of the reference counted pointer with `oldregion`, and
 * if equal set its content to `newregion`.
 *
 * Only succeeds if `oldregion` is still the content of the pointer. The
 * caller's references are untouched, so call `arcp_region_release()` on
 * oldregion and/or newregion if you no longer intend to reference them. Note
 * that the semantics of updating oldregion that might be expected in analogy
 * to `ak_cas` do not hold, as updating oldregion cannot be implemented as a
 * trivial side-effect of this operation.
 *
 * @param rcp the pointer for which to commit the new content.
 * @param oldregion the previous content of the transaction.
 * @param newregion the new content of the transaction.
 *
 * @returns true if set to newregion, false otherwise.
 */
bool arcp_cas(arcp_t *rcp, struct arcp_region *oldregion,
              struct arcp_region *newregion);

bool arcp_cas_weak(arcp_t *rcp, struct arcp_region *oldregion,
                   struct arcp_region *newregion);

bool arcp_cas_release(arcp_t *rcp, struct arcp_region *oldregion,
                      struct arcp_region *newregion);

bool arcp_cas_release_weak(arcp_t *rcp, struct arcp_region *oldregion,
                           struct arcp_region *newregion);

static inline int arcp_usecount(struct arcp_region *region) {
	arcp_refcount_t refcount;
	refcount.p = ak_load(&region->refcount, mo_consume);
	return refcount.v.usecount;
}

static inline int arcp_storecount(struct arcp_region *region) {
	arcp_refcount_t refcount;
	refcount.p = ak_load(&region->refcount, mo_consume);
	return refcount.v.storecount;
}

static inline struct arcp_weakref *
arcp_weakref_phantom(struct arcp_region *region) {
	return region == NULL ? NULL
		: (struct arcp_weakref *) __ARCP_PTRDECOUNT(
				ak_load(&region->weakref, mo_acquire));
}

static inline struct arcp_region *arcp_load_phantom(arcp_t *rcp) {
	return __ARCP_PTRDECOUNT(ak_load(rcp, mo_acquire));
}

#endif /* ! ATOMICKIT_RCP_H */
