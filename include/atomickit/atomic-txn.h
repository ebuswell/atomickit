/** @file atomic-txn.h
 * Atomic Transactions
 *
 * Atomic transactions are pointers with simple transactional
 * semantics for reference-counted memory regions.  Among other
 * things, they can be used to eliminate the so-called ABA problem.
 *
 * The memory allocation and deallocation functions are all user
 * defined.  In most cases, these will simply be `malloc` and `free`,
 * but note that these functions are themselves not, in most
 * implementations, lock free.  Thus, for more critically parallel
 * areas where the turnover time is fast, the programmer will want to
 * design her own version of these functions.  A simple implementation
 * is simply to use a null function for deallocation and a ring buffer
 * for allocation, provided one can be sure that the earliest
 * allocated item will always be deallocated before the buffer loops.
 * This is actually pretty typically the case.  Alternately, lock free
 * versions of malloc and free do exist, but they are complex and
 * often not suitable for general use at this time.
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
#ifndef ATOMICKIT_ATOMIC_TXN_H
#define ATOMICKIT_ATOMIC_TXN_H 1

#include <atomickit/atomic-pointer.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Atomic Transaction
 *
 * The container for the transactional item.
 */
typedef struct {
    volatile atomic_ptr/*<struct atxn_item>*/ ptr;
} atxn_t;

/**
 * \def ATXN_ALIGN The alignment of the data portion of a transaction
 * item.  The maximum number of threads concurrently checking out a
 * given item from a given transaction will be equal to `ATXN_ALIGN -
 * 1`.
 */
#ifdef __LP64__
# define ATXN_ALIGN 16
#else
# define ATXN_ALIGN 8
#endif

/**
 * Atomic Transaction Item
 *
 * The underlying data type which will be stored in the atomic
 * transaction.  In most cases, this will be automatically managed and
 * you will only be dealing with a pointer to the actual data region.
 * Good code will not depend on the content of this struct.
 *
 * An item may be stored in an arbitrary number of transactions.
 */
struct atxn_item {
    void (*destroy)(struct atxn_item *); /** Pointer to a destruction function */
    volatile atomic_int_fast8_t refcount; /** The current reference
					   * count, not including
					   * references from inside
					   * transactions */
    volatile atomic_int_fast8_t txn_refcount; /** The number of
					       * transactions this
					       * item currently
					       * resides in. */
    uint8_t data[1] __attribute__((aligned(ATXN_ALIGN))); /** User-defined data. */
};

/**
 * The size of the atxn_item that is not the user data.
 */
#define ATXN_ITEM_OVERHEAD (sizeof(struct atxn_item) - 1)

/* The mask for the transaction count. */
#define ATXN_COUNTMASK ((intptr_t) (ATXN_ALIGN - 1))

#define ATXN_PTRDECOUNT(ptr) ((void *) (((intptr_t) (ptr)) & ~ATXN_COUNTMASK))

/**
 * Transforms a pointer to a memory region to a pointer to the
 * corresponding transaction item.
 */
#define ATXN_PTR2ITEM(ptr) ((struct atxn_item *) (((intptr_t) (ptr)) - offsetof(struct atxn_item, data)))
/* Gets the count for a given pointer. */
#define ATXN_PTR2COUNT(ptr) ((intptr_t) (ptr) & ATXN_COUNTMASK)

/**
 * Transforms a pointer to a transaction item to a pointer to the
 * corresponding memory region.
 */
#define ATXN_ITEM2PTR(item) ((void *) &item->data)

/* #define ATXN_INIT(item) { ATOMIC_PTR_VAR_INIT(ATXN_ITEM2PTR(item)) } */

/**
 * Initializes a transaction item and returns a pointer to its
 * coresponding memory region.
 *
 * The item has a single reference at this point, belonging to the
 * calling function.
 *
 * @param item a pointer to a transaction item.  This is typically
 * allocated with `malloc(ATXN_ITEM_OVERHEAD + <user data size>).
 * @param destroy pointer to a function that will destroy the
 * allocated item once it is no longer in use.  If no cleanup is
 * needed, this could just be `free`.
 *
 * @returns a pointer to the memory region that corresponds with the
 * item.
 */
static inline void *atxn_item_init(struct atxn_item *item, void (*destroy)(struct atxn_item *)) {
    atomic_store(&item->refcount, 1);
    atomic_store(&item->txn_refcount, 0);
    item->destroy = destroy;
    return ATXN_ITEM2PTR(item);
}

/**
 * Initializes a transaction.
 *
 * @param txn a pointer to the transaction being initialized.
 * @param ptr initial contents of the transaction. This should be a
 * pointer to the memory region corresponding to an initialized
 * `atxn_item`, or `NULL`.
 */
static inline void atxn_init(atxn_t *txn, void *ptr) {
    if(ptr != NULL) {
	atomic_fetch_add(&ATXN_PTR2ITEM(ptr)->txn_refcount, 1);
    }
    atomic_ptr_init(&txn->ptr, ptr);
}

/**
 * Destroys a transaction.
 *
 * @param txn the transaction to destroy.
 */
/* drops its reference to ptr */
static inline void atxn_destroy(atxn_t *txn) {
    void *ptr;
    if(ATXN_PTRDECOUNT(ptr = atomic_ptr_exchange(&txn->ptr, NULL)) != NULL) {
	int_fast8_t count = ATXN_PTR2COUNT(ptr);
	struct atxn_item *item = ATXN_PTR2ITEM(ATXN_PTRDECOUNT(ptr));
	count = atomic_fetch_add(&item->refcount, count) + count;
	if(atomic_fetch_sub(&item->txn_refcount, 1) - 1 == 0) {
	    if(count == 0) {
		item->destroy(item);
	    } else if(unlikely(atomic_load(&item->refcount) == 0)
		      && likely(atomic_fetch_sub(&item->refcount, 1) == 0)) {
		item->destroy(item);
	    }
	}
    }
}

/**
 * Acquire a transaction's contents.
 *
 * @param txn the transaction from which to acquire the current
 * contents.
 *
 * @returns pointer to the memory region for the particular
 * transaction item.
 */
static inline void *atxn_acquire(volatile atxn_t *txn) {
    return ATXN_PTRDECOUNT(atomic_ptr_fetch_add(&txn->ptr, 1));
}

/**
 * Checks to see if the content of the transaction is equal to the
 * provided pointer during some moment of this call.
 *
 * @param txn the transaction to check.
 * @param ptr pointer to the memory region for a particular
 * transaction item.
 *
 * @returns true if `ptr` is equal to the content, false otherwise.
 */
static inline bool atxn_check(volatile atxn_t *txn, void *ptr) {
    return ATXN_PTRDECOUNT(atomic_ptr_load(&txn->ptr)) == ptr;
}

/**
 * Releases a reference to a `struct atxn_item` pointer.
 *
 * `atxn_release` is preferred if there is a corresponding
 * transaction, although both functions will always work correctly.
 *
 * @param ptr the pointer to release a reference to.
 */
static inline void atxn_item_release(void *ptr) {
    if(ptr != NULL) {
	/* if count is not transferred before this call, count will go
	 * negative instead of 0 and the transferer will be
	 * responsible for freeing the variable, if needed. */
	struct atxn_item *item = ATXN_PTR2ITEM(ptr);
	int_fast8_t count = atomic_fetch_sub(&item->refcount, 1) - 1;
	if(count == 0 && atomic_load(&item->txn_refcount) == 0) {
	    if(likely(atomic_fetch_sub(&item->refcount, 1) == 0)) {
		item->destroy(item);
	    }
	}
    }
}

/**
 * Releases a reference to a `struct atxn_item` pointer.
 *
 * `atxn_release` is preferred if there is a corresponding
 * transaction, although both functions will always work correctly.
 *
 * @param txn the transaction from which a reference was acquired.
 * @param ptr the pointer to release a reference to.
 */
static inline void atxn_release(volatile atxn_t *txn, void *ptr) {
    /* Is the ptr still valid? then just decrement it, provided it's
     * count is at least 1. */
    void *localptr = atomic_ptr_load_explicit(&txn->ptr, memory_order_relaxed);
    while(ATXN_PTRDECOUNT(localptr) == ptr && ATXN_PTR2COUNT(localptr) != 0) {
	if(likely(atomic_ptr_compare_exchange_weak_explicit(&txn->ptr, &localptr, localptr - 1,
							    memory_order_seq_cst, memory_order_relaxed))) {
	    /* Success! */
	    return;
	}
    }
    /* Nope.  Try next method. */
    atxn_item_release(ptr);
}

/**
 * Commit a new item as the content of the transaction.
 *
 * Only suceeds if `oldptr` is still the content of the
 * transaction. The caller's references are untouched, so call
 * `atxn_item_release()` on oldptr and/or newptr if you no longer
 * intend to reference them.
 *
 * @param txn the transaction for which to commit the new content.
 * @param oldptr the previous content of the transaction.
 * @param newptr the new content of the transaction.
 *
 * @returns true if the commit was successful, false otherwise.
 */
static inline bool atxn_commit(volatile atxn_t *txn, void *oldptr, void *newptr) {
    void *localptr = atomic_ptr_load_explicit(&txn->ptr, memory_order_relaxed);
    if(newptr != NULL) {
	atomic_fetch_add(&ATXN_PTR2ITEM(newptr)->txn_refcount, 1);
    }
    do {
	if(ATXN_PTRDECOUNT(localptr) != oldptr) {
	    /* fail */
	    if(newptr != NULL) {
		atomic_fetch_sub(&ATXN_PTR2ITEM(newptr)->txn_refcount, 1);
	    }
	    return false;
	}
    } while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&txn->ptr, &localptr, newptr,
								memory_order_seq_cst, memory_order_relaxed)));
    /* success! */
    if(ATXN_PTRDECOUNT(localptr) != NULL) {
	/* Transfer count */
	int_fast8_t count = ATXN_PTR2COUNT(localptr);
	struct atxn_item *item = ATXN_PTR2ITEM(ATXN_PTRDECOUNT(localptr));
	atomic_fetch_add(&item->refcount, count);
	atomic_fetch_sub(&item->txn_refcount, 1);
    }
    return true;
}

#endif /* ! ATOMICKIT_ATOMIC_TXN_H */
