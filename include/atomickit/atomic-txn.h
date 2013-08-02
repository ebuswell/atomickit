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
# define ATXN_ALIGN 8
#else
# define ATXN_ALIGN 4
#endif

struct atxn_item;

/**
 * Atomic Transaction Item Header
 *
 * The metadata needed for reference counting.
 */
struct atxn_item_header {
    void (*destroy)(struct atxn_item *); /** Pointer to a destruction function */
    volatile atomic_uint_fast32_t refcount; /** High 16 bits, number
					     * of transactions in
					     * which this is
					     * referenced; low 16
					     * bits, the number of
					     * individual references
					     * to this item. */
};

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
    struct atxn_item_header header;
    uint8_t data[1] __attribute__((aligned(ATXN_ALIGN))); /** User-defined data. */
};

/**
 * The size of the atxn_item that is not the user data.
 */
#define ATXN_ITEM_OVERHEAD (offsetof(struct atxn_item, data))

/* The mask for the transaction count. */
#define ATXN_COUNTMASK ((intptr_t) (ATXN_ALIGN - 1))

#define ATXN_PTRDECOUNT(ptr) ((void *) (((uintptr_t) (ptr)) & ~ATXN_COUNTMASK))

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
#define ATXN_ITEM2PTR(item) (((void *) item) + offsetof(struct atxn_item, data))

#define ATXN_VAR_INIT(item) { ATOMIC_PTR_VAR_INIT(ATXN_ITEM2PTR(item)) }

#define ATXN_VAR_NULL_INIT { ATOMIC_PTR_VAR_INIT(NULL) }

#define ATXN_ITEM_HEADER_VAR_INIT(destroy, txncount, refcount) { destroy, ATOMIC_VAR_INIT((txncount << 16) | refcount) }

/* #define ATXN_INIT(item) { ATOMIC_PTR_VAR_INIT(ATXN_ITEM2PTR(item)) } */
static inline uint_fast32_t __atxn_urefs(struct atxn_item *item, int16_t txncount, int16_t refcount) {
    uint_fast32_t o_count;
    uint_fast32_t count;
    union {
	volatile int16_t s;
	volatile uint16_t u;
    } otxncount;
    union {
	volatile int16_t s;
	volatile uint16_t u;
    } orefcount;
    o_count = atomic_load_explicit(&item->header.refcount, memory_order_acquire);
    do {
	orefcount.u = o_count & 0xFFFF;
	otxncount.u = o_count >> 16;
	orefcount.s += refcount;
	otxncount.s += txncount;
	count = (otxncount.u << 16) | orefcount.u;
    } while(unlikely(!atomic_compare_exchange_weak_explicit(&item->header.refcount, &o_count, count,
							    memory_order_acq_rel, memory_order_acquire)));
    return count;
}

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
    atomic_init(&item->header.refcount, 1);
    item->header.destroy = destroy;
    return ATXN_ITEM2PTR(item);
}

static inline int atxn_refcount(void *ptr) {
    union {
	volatile int16_t s;
	volatile uint16_t u;
    } refcount;
    refcount.u = atomic_load_explicit(&ATXN_PTR2ITEM(ptr)->header.refcount, memory_order_acquire) & 0xFFFF;
    return refcount.s;
}

static inline int atxn_txncount(void *ptr) {
    union {
	volatile int16_t s;
	volatile uint16_t u;
    } txncount;
    txncount.u = atomic_load_explicit(&ATXN_PTR2ITEM(ptr)->header.refcount, memory_order_acquire) >> 16;
    return txncount.s;
}

/**
 * Increments the reference count of a `struct atxn_item` pointer.
 *
 * This is intended for situations in which a single thread is passing
 * a reference among different memory regions. Contention should not
 * be an issue in this transfer.
 *
 * @param ptr the pointer whose reference count should be incremented.
 */
static inline void atxn_incref(void *ptr) {
    if(ptr != NULL) {
	__atxn_urefs(ATXN_PTR2ITEM(ptr), 0, 1);
    }
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
	__atxn_urefs(ATXN_PTR2ITEM(ptr), 1, 0);
	atomic_thread_fence(memory_order_seq_cst);
    }
    atomic_ptr_store_explicit(&txn->ptr, ptr, memory_order_release);
}

/**
 * Destroys a transaction.
 *
 * @param txn the transaction to destroy.
 */
/* The transaction drops its reference to ptr */
static inline void atxn_destroy(atxn_t *txn) {
    void *ptr;
    if(ATXN_PTRDECOUNT(ptr = atomic_ptr_exchange_explicit(&txn->ptr, NULL, memory_order_acq_rel)) != NULL) {
	struct atxn_item *item = ATXN_PTR2ITEM(ATXN_PTRDECOUNT(ptr));
	if(__atxn_urefs(item, -1, ATXN_PTR2COUNT(ptr)) == 0) {
	    if(item->header.destroy != NULL) {
		item->header.destroy(item);
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
static inline void *atxn_acquire(atxn_t *txn) {
    void *ptr = atomic_ptr_load_explicit(&txn->ptr, memory_order_acquire);
    do {
	while(ATXN_PTR2COUNT(ptr) == ATXN_ALIGN - 1) {
	    /* Spinlock if too many threads are accessing this at once. */
	    ptr = atomic_ptr_load_explicit(&txn->ptr, memory_order_acquire);
	}
    } while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&txn->ptr, &ptr, ptr + 1,
								memory_order_acq_rel, memory_order_acquire)));
    ptr += 1;
    /* We have one reference */
    void *ret = ATXN_PTRDECOUNT(ptr);
    if(ret != NULL) {
	__atxn_urefs(ATXN_PTR2ITEM(ret), 0, 1);
    }
    /* We have two references, try and remove the one from the pointer. */
    while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&txn->ptr, &ptr, ptr - 1,
							      memory_order_acq_rel, memory_order_acquire))) {
	/* Is the pointer still valid? */
	if(ATXN_PTRDECOUNT(ptr) != ret) {
	    /* Somebody else has transferred / will transfer the
	     * count. */
	    if(ret != NULL) {
		__atxn_urefs(ATXN_PTR2ITEM(ret), 0, -1);
	    }
	    break;
	}
    }
    return ret;
}

/**
 * Peek at a transaction's contents without affecting the reference
 * count.
 *
 * @param txn the transaction from which to look at the current
 * contents.
 *
 * @returns pointer to the memory region for the particular
 * transaction item.
 */
static inline void *atxn_peek(atxn_t *txn) {
    return ATXN_PTRDECOUNT(atomic_ptr_load_explicit(&txn->ptr, memory_order_acquire));
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
static inline bool atxn_check(atxn_t *txn, void *ptr) {
    return ATXN_PTRDECOUNT(atomic_ptr_load_explicit(&txn->ptr, memory_order_acquire)) == ptr;
}

/**
 * Releases a reference to a `struct atxn_item` pointer.
 *
 * @param ptr the pointer to release a reference to.
 */
static inline void atxn_release(void *ptr) {
    if(ptr != NULL) {
	/* if count is not transferred before this call, count will go
	 * negative instead of 0 and the transferer will be
	 * responsible for freeing the variable, if needed. */
	struct atxn_item *item = ATXN_PTR2ITEM(ptr);
	if(__atxn_urefs(item, 0, -1) == 0) {
	    if(item->header.destroy != NULL) {
		item->header.destroy(item);
	    }
	}
    }
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
static inline bool atxn_commit(atxn_t *txn, void *oldptr, void *newptr) {
    void *localptr = atomic_ptr_load_explicit(&txn->ptr, memory_order_acquire);
    if(newptr != NULL) {
	__atxn_urefs(ATXN_PTR2ITEM(newptr), 1, 0);
    }
    do {
	if(ATXN_PTRDECOUNT(localptr) != oldptr) {
	    /* fail */
	    if(newptr != NULL) {
		__atxn_urefs(ATXN_PTR2ITEM(newptr), -1, 0);
	    }
	    return false;
	}
    } while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&txn->ptr, &localptr, newptr,
								memory_order_seq_cst, memory_order_relaxed)));
    /* success! */
    if(oldptr != NULL) {
	/* Transfer count */
	__atxn_urefs(ATXN_PTR2ITEM(oldptr), -1, ATXN_PTR2COUNT(localptr));
	/* refcount can't reach 0 here as the caller should still own a reference to oldptr */
    }
    return true;
}

/**
 * Store a new item as the content of the transaction.
 *
 * This is unconditional and should only be used in special
 * circumstances. The caller's references are untouched, so call
 * `atxn_item_release()` on newptr if you no longer intend to
 * reference it.
 *
 * @param txn the transaction for which to commit the new content.
 * @param newptr the new content of the transaction.
 *
 * @returns true if the commit was successful, false otherwise.
 */
static inline void atxn_store(atxn_t *txn, void *newptr) {
    if(newptr != NULL) {
	__atxn_urefs(ATXN_PTR2ITEM(newptr), 1, 0);
    }
    void *ptr;
    if(ATXN_PTRDECOUNT(ptr = atomic_ptr_exchange_explicit(&txn->ptr, newptr, memory_order_seq_cst)) != NULL) {
	struct atxn_item *item = ATXN_PTR2ITEM(ATXN_PTRDECOUNT(ptr));
	if(__atxn_urefs(item, -1, ATXN_PTR2COUNT(ptr)) == 0) {
	    if(item->header.destroy != NULL) {
		item->header.destroy(item);
	    }
	}
    }
}

#endif /* ! ATOMICKIT_ATOMIC_TXN_H */
