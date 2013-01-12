/*
 * atomic-txn.h
 * 
 * Copyright 2012 Evan Buswell
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

typedef struct {
    atomic_ptr/*<struct atxn_item>*/ ptr;
} atxn_t;

#ifdef __LP64__
# define ATXN_ALIGN 16
#else
# define ATXN_ALIGN 8
#endif

struct atxn_item {
    void (*destroy)(struct atxn_item *);
    atomic_int_fast8_t refcount;
    uint8_t data[1] __attribute__((aligned(ATXN_ALIGN)));
};

#define ATXN_COUNTMASK ((intptr_t) (ATXN_ALIGN - 1))

#define ATXN_PTR2ITEM(ptr) ((struct atxn_item *) ((ptr) - (sizeof(struct atxn_item) - 1)))
#define ATXN_ITEM2PTR(item) ((void *) &item->data)

#define ATXN_INIT(item) { ATOMIC_PTR_VAR_INIT(ATXN_ITEM2PTR(item)) }

/* Not atomic */
static inline void atxn_init(atxn_t *txn, struct atxn_item *item) {
    atomic_ptr_init(&txn->ptr, ATXN_ITEM2PTR(item));
}

/* Not atomic */
static inline void atxn_destroy(atxn_t *txn) {
    void *ptr;
    if((ptr = atomic_ptr_load_explicit(&txn->ptr, memory_order_relaxed)) != NULL) {
	struct atxn_item *item = ATXN_ITEM(ptr & ~ATXN_COUNTMASK);
	item->destroy(item);
	atomic_ptr_store_explicit(&txn->ptr, NULL, memory_order_relaxed);
    }
}

static inline void *atxn_acquire(volatile atxn_t *txn) {
    void *ptr = atomic_ptr_fetch_add(&txn->ptr, 1);
    return ptr & ~ATXN_COUNTMASK;
}

static inline void atxn_release(volatile atxn_t *txn, void *ptr) {
    /* Is the ptr still valid? then just decrement it. */
    void *localptr = atomic_ptr_load_explicit(&txn->ptr, memory_order_relaxed);
    while((localptr & ~ATXN_COUNTMASK) == ptr) {
	if(likely(atomic_ptr_compare_exchange_weak_explicit(&txn->ptr, &localptr, localptr - 1),
		  memory_order_seq_cst, memory_order_relaxed)) {
	    /* Success! */
	    return;
	}
    }
    /* Nope.  Try next method. */
    if(ptr != NULL) {
	/* if count is not transferred before this call, count will go
	 * negative instead of 0 and the transferer will be
	 * responsible for freeing the variable. */
	struct atxn_item *item = ATXN_PTR2ITEM(ptr);
	if(atomic_fetch_sub(&item->refcount, 1) - 1 == 0) {
	    item->destroy(item);
	}
    }
}

/* On success, discards reference to oldptr and implicit reference to
 * newitem.  On failure, references are unchanged. */
static inline bool atxn_commit(volatile atxn_t *txn, void *oldptr, struct atxn_item *newitem) {
    void *newptr = ATXN_ITEM2PTR(newitem);
    void *localptr = atomic_ptr_load_explicit(&txn->ptr, memory_order_relaxed);
    do {
	if((localptr & ~ATXN_COUNTMASK) != oldptr) {
	    /* fail */
	    return false;
	}
    } while(unlikely(!atomic_ptr_compare_exchange_weak_explicit(&txn->ptr, &localptr, newptr,
								memory_order_seq_cst, memory_order_relaxed)));
    /* success! */
    if((localptr & ~ATXN_COUNTMASK) != NULL) {
	/* Transfer count, minus the implicitly present reference to oldptr. */
	int_fast8_t count = (localptr & ATXN_COUNTMASK) - 1;
	struct atxn_item *item = ATXN_PTR2ITEM(localptr & ~ATXN_COUNTMASK);
	if(count == 0) {
	    item->destroy(item);
	} else if(atomic_fetch_add(&item->refcount, count) + count == 0) {
	    item->destroy(item);
	}
    }
    return true;
}

#endif /* ! ATOMICKIT_ATOMIC_TXN_H */
