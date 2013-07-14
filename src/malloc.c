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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "atomickit/atomic.h"
#include "atomickit/atomic-pointer.h"
#include "atomickit/atomic-malloc.h"

/* These things look like simple parameters, but if you change them
 * beware the bit twiddling... */
#define PAGE_SIZE 4096
#define PAGE_CEIL(size) (((((size) - 1) >> 12) + 1) << 12)
#define MIN_SIZE 16
#define MIN_SIZE_LOG2 4
#define NSIZES 8
/* Reference counting is stored in the extra bits of the aligned pointers. */
#define PTR_DECOUNT(ptr) ((void *) (((uintptr_t) (ptr)) & ~((uintptr_t) (MIN_SIZE - 1))))
#define PTR_COUNT(ptr) (((uintptr_t) (ptr)) & ((uintptr_t) (MIN_SIZE - 1)))

/* The items pushed on the free stack. Since 16 bytes is twice the
 * pointer size on 64 bit, we should always have at least this much
 * room. */
struct fstack_item {
    void *next; /* Pointer to next item. This is set in one thread,
		 * and only read in others after a load with a full
		 * memory barrier. This should be fine in practice,
		 * but the semantics of C11 atomics are a bit strange;
		 * should this be an atomic_ptr? */
    volatile atomic_int refcount; /* Reference count. */
};

/* The top-level free stack pointers for each size. */
static volatile atomic_ptr glbl_fstack[NSIZES] = {
    ATOMIC_PTR_VAR_INIT(NULL) /* 16 */,  ATOMIC_PTR_VAR_INIT(NULL) /* 32 */,  ATOMIC_PTR_VAR_INIT(NULL) /* 64 */,   ATOMIC_PTR_VAR_INIT(NULL) /* 128 */,
    ATOMIC_PTR_VAR_INIT(NULL) /* 256 */, ATOMIC_PTR_VAR_INIT(NULL) /* 512 */, ATOMIC_PTR_VAR_INIT(NULL) /* 1024 */, ATOMIC_PTR_VAR_INIT(NULL) /* 2048 */
};

/* Transforms a size into the bin number for that size. Conceptually:
 * ceil(log2(size)) - log2(MIN_SIZE). */
static inline int size2bin(size_t size) {
    size--;
    size >>= MIN_SIZE_LOG2 - 1;
    int r = 0;
    while(size >>= 1) {
	r++;
    }
    return r;
}

/* Transforms a bin number into the corresponding size. */
static inline size_t bin2size(int bin) {
    return 1 << (bin + MIN_SIZE_LOG2);
}

/* Create a randomized, aligned pointer. */
static inline void *randomptr() {
    if(sizeof(void *) > 4) {
	union {
	    void *ptr;
	    struct {
		int32_t i1;
		int32_t i2;
	    } i;
	} ret;
	ret.i.i1 = mrand48();
	ret.i.i2 = mrand48();
	return PTR_DECOUNT(ret.ptr);
    } else {
	union {
	    void *ptr;
	    int32_t i;
	} ret;
	ret.i = mrand48();
	return PTR_DECOUNT(ret.ptr);
    }
}

/* Allocate pages directly from the OS. Uses mmap. */
static inline void *os_alloc(size_t size) {
    return mmap(NULL, PAGE_CEIL(size), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

/* Free pages directly to the OS. Uses munmap. Note that currently
 * fragmented pages are never freed. */
static inline int os_free(void *ptr, size_t size) {
    return munmap(ptr, PAGE_CEIL(size));
}

/* Reallocate a memory region directly with the OS. */
static inline void *os_realloc(void *ptr, size_t oldsize, size_t newsize) {
    size_t c_oldsize = PAGE_CEIL(oldsize);
    size_t c_newsize = PAGE_CEIL(newsize);
    if(c_oldsize == c_newsize) {
	return ptr;
    }
    void *ret = mmap(NULL, c_newsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);;
    if(ret == NULL) {
	return NULL;
    }
    memcpy(ret, ptr, oldsize > newsize ? newsize : oldsize);
    int r = munmap(ptr, c_oldsize);
    if(r != 0) {
	munmap(ret, c_newsize);
	return NULL;
    }
    return ret;
}

/* Atomically pop a memory region of the specified size off the
 * stack. Returns NULL if the stack for that memory region is empty. */
static inline void *fstack_pop(int bin) {
    for(;;) {
	/* Acquire the top of the stack and update its reference
	 * count. */
	void *next = atomic_ptr_fetch_add(&glbl_fstack[bin], 1) + 1;
	if(PTR_DECOUNT(next) == NULL) {
	    /* The stack is currently empty; get rid of the reference
	     * count we just added to the NULL pointer. */
	    do {
		if(likely(atomic_ptr_compare_exchange_weak_explicit(
			      &glbl_fstack[bin], &next, NULL,
			      memory_order_seq_cst, memory_order_relaxed)
			  || next == NULL)) {
		    /* Empty stack. */
		    return NULL;
		}
	    } while(PTR_DECOUNT(next) == NULL);
	    /* Something was added while we were trying to update; try
	     * again. */
	    continue;
	}
	struct fstack_item *item = (struct fstack_item *) PTR_DECOUNT(next);
	/* Try to pop the item off the top of the stack. */
	do {
	    if(likely(atomic_ptr_compare_exchange_weak_explicit(
			  &glbl_fstack[bin], &next, item->next,
			  memory_order_seq_cst, memory_order_relaxed))) {
		/* Transfer count. */
		atomic_fetch_add(&item->refcount, PTR_COUNT(next));
		break;
	    }
	} while(PTR_DECOUNT(next) == item);
	/* The item is no longer the top of the stack, because someone
	 * successfully popped it. Release reference. If the refcount
	 * hasn't been transferred yet, then refcount will go negative
	 * instead of zero. */
	if(likely((atomic_fetch_sub(&item->refcount, 1) - 1) == 0)) {
	    /* We are the last to hold a reference to this item. As no
	     * one else can claim it; return it. */
	    return (void *) item;
	}
	/* References to the item remain. Someone else will return it,
	 * or push will add it back to the stack. Loop and try
	 * again. */
    }
}

static void fstack_push(int bin, void *ptr) {
    struct fstack_item *new_item = (struct fstack_item *) ptr;
    /* Set the initial refcount to zero. */
    atomic_init(&new_item->refcount, 0);
    for(;;) {
	/* Get the top of the stack; we have to update reference count
	 * since we act like pop if there's more than our own
	 * reference count. */
	void *next = atomic_ptr_fetch_add(&glbl_fstack[bin], 1) + 1;
	if(PTR_DECOUNT(next) == NULL) {
	    /* This is the only item that will be on the stack. */
	    new_item->next = NULL;
	    do {
		if(likely(atomic_ptr_compare_exchange_weak_explicit(
			      &glbl_fstack[bin], &next, ptr,
			      memory_order_seq_cst, memory_order_relaxed))) {
		    /* Success! */
		    return;
		}
	    } while(PTR_DECOUNT(next) == NULL);
	    /* Something else was added before we could add this; try again. */
	    continue;
	}
	struct fstack_item *item = (struct fstack_item *) PTR_DECOUNT(next);
	if(likely(PTR_COUNT(next) == 1)) {
	    new_item->next = PTR_DECOUNT(next);
	    /* Try to push the item on to the top of the stack. */
	    if(likely(atomic_ptr_compare_exchange_weak_explicit(
			  &glbl_fstack[bin], &next, new_item,
			  memory_order_seq_cst, memory_order_relaxed))) {
		/* Success! */
		return;
	    }
	}
	/* Someone's trying to pop this item, or someone else is
	 * trying to push to it, too. Because we can't distinguish
	 * this situation, just help pop the item. */
	while(PTR_DECOUNT(next) == item) {
	    if(atomic_ptr_compare_exchange_weak_explicit(
		   &glbl_fstack[bin], &next, item->next,
		   memory_order_seq_cst, memory_order_relaxed)) {
		/* Transfer count. */
		atomic_fetch_add(&item->refcount, PTR_COUNT(next));
		break;
	    }
	}
	/* The item is no longer the top of the stack, because someone
	 * successfully popped it. Release reference. If the refcount
	 * hasn't been transferred yet, then refcount will go negative
	 * instead of zero. */
	if((atomic_fetch_sub(&item->refcount, 1) - 1) == 0) {
	    /* We are the last to hold a reference to this item. But
	     * we don't want it, so push this one, too. This is one of
	     * the more inelegant things about this algorithm... */
	    fstack_push(bin, item);
	}
	/* Loop and try again. */
    }
}

void *amalloc(size_t size) {
    if(size == 0) {
	return randomptr();
    }
    if(size > 2048) {
	return os_alloc(size);
    }
    void *ret;
    int bin = size2bin(size);
    int i;
    for(i = bin; i < NSIZES; i++) {
	if((ret = fstack_pop(i)) != NULL) {
	    goto breakdown_chunk;
	}
    }
    /* allocate a new chunk */
    ret = os_alloc(PAGE_SIZE);
    if(ret == NULL) {
	return NULL;
    }
    /* i = NSIZES; -- already nsizes from for loop above */
breakdown_chunk:
    while(i > bin) {
	i--;
	fstack_push(i, ret + bin2size(i));
    }
    return ret;
}

int afree(void *ptr, size_t size) {
    if(size == 0) {
	return 0;
    } else if(size > 2048) {
	return os_free(ptr, size);
    } else {
	fstack_push(size2bin(size), ptr);
	return 0;
    }
}

void *arealloc(void *ptr, size_t oldsize, size_t newsize) {
    if(oldsize == 0) {
	if(newsize == 0) {
	    return ptr;
	}
	return amalloc(newsize);
    } else if(newsize == 0) {
	int r = afree(ptr, oldsize);
	if(r != 0) {
	    return NULL;
	}
	return randomptr();
    }
    if(oldsize > 2048 && newsize > 2048) {
	return os_realloc(ptr, oldsize, newsize);
    } else if(size2bin(oldsize) == size2bin(newsize)) {
	return ptr;
    }
    void *ret = amalloc(newsize);
    if(ret == NULL) {
	return NULL;
    }
    memcpy(ret, ptr, oldsize > newsize ? newsize : oldsize);
    int r = afree(ptr, oldsize);
    if(r != 0) {
	afree(ret, newsize);
	return NULL;
    }
    return ret;
}
