#include <sys/mman.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include "atomickit/atomic.h"
#include "atomickit/atomic-pointer.h"
#include "atomickit/atomic-malloc.h"

#define PAGE_SIZE 4096
#define PAGE_CEIL(size) ((size) + (size) % PAGE_SIZE)
#define MIN_SIZE 16
#define MIN_SIZE_LOG2 4
#define NSIZES 8
#define PTR_DECOUNT(ptr) ((void *) (((uintptr_t) (ptr)) & ~((uintptr_t) (MIN_SIZE - 1))))
#define PTR_COUNT(ptr) (((uintptr_t) (ptr)) & ((uintptr_t) (MIN_SIZE - 1)))

static inline int size2bin(size_t size) {
    size--;
    size >>= MIN_SIZE_LOG2 - 1;
    int r = 0;
    while(size >>= 1) {
	r++;
    }
    return r;
}

static inline size_t bin2size(int bin) {
    return 1 << (bin + MIN_SIZE_LOG2);
}

struct fstack_item {
    atomic_ptr next;
    atomic_int refcount;
};

static volatile atomic_ptr glbl_fstack[NSIZES] = {
    ATOMIC_PTR_INIT(NULL), ATOMIC_PTR_INIT(NULL), ATOMIC_PTR_INIT(NULL), ATOMIC_PTR_INIT(NULL),
    ATOMIC_PTR_INIT(NULL), ATOMIC_PTR_INIT(NULL), ATOMIC_PTR_INIT(NULL), ATOMIC_PTR_INIT(NULL)
};

static inline void *os_alloc(size_t size) {
    return mmap(NULL, PAGE_CEIL(size), PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

static inline int os_free(void *ptr, size_t size) {
    return munmap(ptr, PAGE_CEIL(size));
}

static inline void *os_realloc(void *ptr, size_t oldsize, size_t newsize) {
    oldsize = PAGE_CEIL(oldsize);
    newsize = PAGE_CEIL(newsize);
    if(oldsize == newsize) {
	return ptr;
    }
    return mremap(ptr, oldsize, newsize, MREMAP_MAYMOVE);
}

static inline void *fstack_pop(int bin) {
    for(;;) {
	void *next = atomic_fetch_add(&glbl_fstack[bin], 1) + 1;
	if(PTR_DECOUNT(next) == NULL) {
	    do {
		if(likely(atomic_compare_exchange_weak_explicit(
			      &glbl_fstack[bin], &next, NULL,
			      memory_order_seq_cst, memory_order_relaxed))
		   || next == NULL) {
		    /* Empty stack */
		    return NULL;
		}
	    } while(PTR_DECOUNT(next) == NULL);
	    /* something was added; try again */
	    continue;
	}
	struct fstack_item *item = (struct fstack_item *) PTR_DECOUNT(next);
	do {
	    if(atomic_compare_exchange_weak_explicit(
		   &glbl_fstack[bin], &next, atomic_ptr_load(&item->next),
		   memory_order_seq_cst, memory_order_relaxed)) {
		/* set next pointer null to show that we've popped,
		   rather than been pushed under */
		atomic_ptr_store(&item->next, NULL);
		/* transfer count */
		atomic_add(&item->refcount, PTR_COUNT(next));
		break;
	    }
	} while(PTR_DECOUNT(next) == item);
	int refcount = atomic_fetch_sub(&item->refcount, 1) - 1;
	if(refcount == 0 && atomic_ptr_load(&item->next) == NULL) {
	    /* safe to return */
	    return (void *) item;
	}
	/* someone else will return it, or push will add it back to the stack, loop and try again */
    }
}

static inline void fstack_push(int bin, void *ptr) {
    struct fstack_item *new_item = (struct fstack_item *) ptr;
    atomic_init(&new_item->refcount, 0);
    for(;;) {
	void *next = atomic_fetch_add(&glbl_fstack[bin], 1);
	if(PTR_DECOUNT(next) == NULL) {
	    new_item->next = NULL;
	    do {
		if(likely(atomic_compare_exchange_weak_explicit(
			      &glbl_fstack[bin], &next, ptr,
			      memory_order_seq_cst, memory_order_relaxed))) {
		    return;
		}
	    } while(PTR_DECOUNT(next) == NULL);
	    /* something else was added; try again */
	    continue;
	}
	new_item->next = PTR_DECOUNT(next);

	struct fstack_item *item = (struct fstack_item *) new_item->next;
	do {
	    if(atomic_compare_exchange_weak_explicit(
		   &glbl_fstack[bin], &next, ptr,
		   memory_order_seq_cst, memory_order_relaxed)) {
		if(PTR_COUNT(next) > 1) {
		    /* transfer count, minus our own, so that it properly returns to zero */
		    atomic_add(&item->refcount, PTR_COUNT(next) - 1);
		}
		return;
	    }
	} while(PTR_DECOUNT(next) == item);
	/* next was taken out from under us; clean up refcount and try again */
	int refcount = atomic_fetch_sub(&item->refcount, 1) - 1;
	if(refcount == 0) {
	    /* we are now the proud owner of an item we didn't want :-/ put it back! */
	    fstack_push(bin, (void *) item);
	}
    }
}

void *amalloc(size_t size) {
    if(size == 0) {
	return NULL;
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
	return amalloc(newsize);
    } else if(newsize == 0) {
	int r = afree(ptr, oldsize);
	if(r != 0) {
	    /* do what? */
	}
	return NULL;
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
