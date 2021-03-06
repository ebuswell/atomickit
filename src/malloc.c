/*
 * malloc.c
 *
 * Copyright 2014 Evan Buswell
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
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "atomickit/atomic.h"
#include "atomickit/malloc.h"

/* These things look like simple parameters, but if you change them beware the
 * bit twiddling... */
#define PAGE_SIZE	4096
#define PAGE_SIZE_LOG2	12
#define NSIZES		10
#define MIN_SIZE	16
#define MIN_SIZE_LOG2	4
#define OS_THRESH	8192
#define PAGE_CEIL(size)							\
	(((((size) - 1) >> PAGE_SIZE_LOG2) + 1) << PAGE_SIZE_LOG2)
/* Reference counting is stored in the extra bits of the aligned pointers. */
#define PTR_DECOUNT(ptr)						\
	((struct fstack_item *) (((uintptr_t) (ptr))			\
				 & ~((uintptr_t) (MIN_SIZE - 1))))
#define PTR_COUNT(ptr)	(((uintptr_t) (ptr)) & ((uintptr_t) (MIN_SIZE - 1)))

/* For debugging amalloc */
/* #define AMALLOC_DEBUG 1 */

/* Uses (non-atomic) vanilla malloc so that you can debug stuff using
 * valgrind */
/* #define AMALLOC_VALGRIND_DEBUG 1 */

#ifndef AMALLOC_VALGRIND_DEBUG

/* The items pushed on the free stack. Since 16 bytes is twice the pointer
 * size on 64 bit, we should always have at least this much room. */
struct fstack_item {
	struct fstack_item *next;	/* Pointer to next item. This is set
					 * in one thread, and only read in
					 * others after a load with a full
					 * memory barrier, so it doesn't need
					 * to be atomic. */
	atomic_int refcount;		/* Reference count. */
};

/* The top-level free stack pointers for each size. */
static volatile _Atomic(struct fstack_item *) glbl_fstack[NSIZES] = {
	ATOMIC_VAR_INIT(NULL) /* 16 */, ATOMIC_VAR_INIT(NULL) /* 32 */,
	ATOMIC_VAR_INIT(NULL) /* 64 */, ATOMIC_VAR_INIT(NULL) /* 128 */,
	ATOMIC_VAR_INIT(NULL) /* 256 */, ATOMIC_VAR_INIT(NULL) /* 512 */,
	ATOMIC_VAR_INIT(NULL) /* 1024 */, ATOMIC_VAR_INIT(NULL) /* 2048 */,
	ATOMIC_VAR_INIT(NULL) /* 4096 */, ATOMIC_VAR_INIT(NULL) /* 8192 */
};

/* Transforms a size into the bin number for that size. Conceptually:
 * ceil(log2(size)) - log2(MIN_SIZE). */
static inline int size2bin(size_t size) {
	int r;
	size--;
	size >>= MIN_SIZE_LOG2 - 1;
	r = 0;
	while (size >>= 1) {
		r++;
	}
	return r;
}

/* Transforms a bin number into the corresponding size. */
static inline size_t bin2size(int bin) {
	return 1 << (bin + MIN_SIZE_LOG2);
}

static void am_perror(const char *msg) {
	char buf[128];
	strcpy(buf, msg);
	strcpy(buf + strlen(buf), ": ");
	if (strerror_r(errno, buf + strlen(buf), 128 - strlen(buf)) == 0) {
		strcpy(buf + strlen(buf), "\n");
		(void) write(STDERR_FILENO, buf, strlen(buf));
	} else {
		(void) write(STDERR_FILENO, msg, strlen(msg));
	}
}

# ifndef AMALLOC_DEBUG
#  define DEBUG_PRINTF(...)	do { } while (0)
#  define STACKTRACE()		do { } while (0)
# else /* AMALLOC_DEBUG */
#  include <stdio.h>

#  include <execinfo.h>
#  include <signal.h>
#  define STACKTRACE() do {						\
		void *__btbuffer[128];					\
		backtrace_symbols_fd(__btbuffer,			\
				     backtrace(__btbuffer, 128),	\
				     STDERR_FILENO);			\
			raise(SIGSEGV);					\
	} while (0)

#  define DEBUG_PRINTF(...) do {					\
		char __buf[128];					\
		snprintf(__buf, 128, __VA_ARGS__);			\
		(void) write(STDERR_FILENO, __buf, strlen(__buf));	\
	} while (0)
# endif /* AMALLOC_DEBUG */

/* Allocate pages directly from the OS. Uses mmap. */
static void *os_alloc(size_t size) {
	void *ptr;
	ptr = mmap(NULL, PAGE_CEIL(size), PROT_READ | PROT_WRITE | PROT_EXEC,
		   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED) {
		DEBUG_PRINTF("Failed allocating %zd bytes via os_alloc\n",
			     size);
		return NULL;
	} else {
		DEBUG_PRINTF("Allocated %zd bytes at %p via os_alloc\n",
			     size, ptr);
		return ptr;
	}
}

/* Free pages directly to the OS. Uses munmap. Note that currently fragmented
 * pages are never freed. */
static void os_free(void *ptr, size_t size) {
	if (munmap(ptr, PAGE_CEIL(size)) != 0) {
		am_perror("afree() failed at munmap; MEMORY MAY BE LEAKING");
		STACKTRACE();
	}
	DEBUG_PRINTF("Deallocated %zd bytes at %p via os_alloc\n", size, ptr);
}

/* Try to reallocate a memory allocation in place. Returns true if the
 * reallocation succeeded, false if the reallocation needs to move the
 * original allocation. Currently, this only succeeds if the number of pages
 * is shrinking or remaining constant, such that a simple unmap will suffice.
 * I'm not certain that the mmap function can be safely used to increase the
 * page size without more information, but even if it can that is probably too
 * difficult to be worth it, especially given that os_tryrealloc is only used
 * for reallocating pages over the OS_THRESH, likely to be an uncommon
 * occurrence. */
static bool os_tryrealloc(void *ptr __attribute__((unused)),
			  size_t oldsize, size_t newsize) {
	size_t c_oldsize, c_newsize;
	c_oldsize = PAGE_CEIL(oldsize);
	c_newsize = PAGE_CEIL(newsize);
	if (c_oldsize == c_newsize) {
		return true;
	} else if (c_oldsize > c_newsize) {
		ptr += c_newsize;
		if (munmap(ptr, c_oldsize - c_newsize) != 0) {
			DEBUG_PRINTF("Failed changing allocation from "
				     "%zd bytes at %p to %zd bytes at %p "
				     "via os_tryrealloc\n", oldsize, ptr,
				     newsize, ptr);
			am_perror("atryrealloc() failed at munmap");
			STACKTRACE();
			return false;
		}
		DEBUG_PRINTF("Changed allocation from %zd bytes at %p to "
			     "%zd bytes at %p via os_tryrealloc\n", oldsize,
			     ptr, newsize, ptr);
		return true;
	}
	return false;
}

/* Reallocate a memory region directly from the OS. */
static void *os_realloc(void *ptr, size_t oldsize, size_t newsize) {
	int r;
	void *ret;
	size_t c_oldsize, c_newsize;
	c_oldsize = PAGE_CEIL(oldsize);
	c_newsize = PAGE_CEIL(newsize);
	if (c_oldsize == c_newsize) {
		return ptr;
	} else if (c_oldsize > c_newsize) {
		r = munmap(ptr + c_newsize, c_oldsize - c_newsize);
		if (r != 0) {
			DEBUG_PRINTF("Failed changing allocation from %zd "
				     "bytes at %p to %zd bytes at %p via "
				     "os_tryrealloc\n", oldsize, ptr,
				     newsize, ptr);
			STACKTRACE();
			return NULL;
		}
		DEBUG_PRINTF("Changed allocation from %zd bytes at %p to "
			     "%zd bytes at %p via os_tryrealloc\n",
			     oldsize, ptr, newsize, ptr);
		return ptr;
	}
	/* Otherwise we're growing the region. */
	ret = mmap(NULL, c_newsize, PROT_READ | PROT_WRITE | PROT_EXEC,
		   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ret == MAP_FAILED) {
		DEBUG_PRINTF("Failed changing allocation from %zd bytes at "
			     "%p to %zd bytes at ? via os_tryrealloc\n",
			     oldsize, ptr, newsize);
		return NULL;
	}
	memcpy(ret, ptr, oldsize);
	r = munmap(ptr, c_oldsize);
	if (r != 0) {
		DEBUG_PRINTF("Failed to unmap old pointer at %p\n", ptr);
		if (munmap(ret, c_newsize) != 0) {
			am_perror("arealloc() failed in munmap error "
				  "routine; MEMORY IS LEAKING");
		}
		STACKTRACE();
		return NULL;
	}
	return ret;
}

# ifndef AMALLOC_DEBUG
#  define CHECK_FREE(ptr)		do { } while (0)
#  define MAYBE_CHECK_FREE(ptr, size)	do { } while (0)
#  define CHECK_ALLOC(ptr)		do { } while (0)
#  define MAYBE_CHECK_ALLOC(ptr, size)	do { } while (0)
# else /* AMALLOC_DEBUG */
#  include "atomickit/rcp.h"

/* Simple array list; we keep it sorted by ptr */
struct debug_alloc_list {
	struct arcp_region;
	size_t len;
	uintptr_t ptrs[];
};

/* The variable in which the RCU array list is stored */
static arcp_t debug_alloc = ARCP_VAR_INIT(NULL);

/* Destruction: free directly to the OS to avoid variables */
static void __destroy_debug_alloc_list(struct debug_alloc_list *list) {
	os_free(list,
		sizeof(struct debug_alloc_list)
		+ sizeof(uintptr_t) * list->len);
};

/* Check if the provided pointer is in the list of allocated pointers, and if
 * so remove it. */
static void check_free(void *ptr) {
	struct debug_alloc_list *list;
	struct debug_alloc_list *newlist;
	size_t l, u, i;
	/* wrap the whole thing in an RCU loop */
	do {
		/* load the current list */
		list = (struct debug_alloc_list *) arcp_load(&debug_alloc);
		if (list == NULL) {
			/* the list is null, so this free cannot possibly be
			 * right */
			DEBUG_PRINTF("Trying to free %p but nothing has "
				     "been allocated\n", ptr);
			STACKTRACE();
		}
		/* binary search for ptr */
		l = 0;
		u = list->len;
		i = 0;
		while (l < u) {
			i = (l + u) / 2;
			if (((uintptr_t) ptr) < list->ptrs[i]) {
				u = i;
			} else if (((uintptr_t) ptr) > list->ptrs[i]) {
				l = ++i;
			} else {
				/* found it; all is well */
				/* allocate new list */
				newlist = os_malloc(
					sizeof(struct debug_alloc_list)
					+ sizeof(void *) * (list->len - 1));
				if (newlist == NULL) {
					DEBUG_PRINTF("os_malloc failed\n");
					STACKTRACE();
				}
				arcp_region_init(
					newlist,
					(arcp_destroy_f)
					__destroy_debug_alloc_list);
				/* copy everything over, leaving out the value
				 * at i */
				newlist->len = list->len - 1;
				memcpy(newlist->ptrs, list->ptrs,
				       sizeof(void *) * i);
				memcpy(newlist->ptrs + i, list->ptrs + i + 1,
				       sizeof(void *) * (list->len
							 - (i + 1)));
				goto try_replace;
			}
		}
		/* failed to find the ptr */
		DEBUG_PRINTF("Trying to free %p but it has not been "
			     "allocated or has been deallocated\n", ptr);
		STACKTRACE();
	try_replace: ;
	} while (!arcp_compare_store_release(&debug_alloc, list, newlist));
}

/* Check that the newly allocated ptr is not already in the list of allocated
 * ptrs, and add it to the list. */
static void check_alloc(void *ptr) {
	struct debug_alloc_list *list;
	struct debug_alloc_list *newlist;
	size_t l, u, i;
	/* wrap the whole thing in an RCU loop */
	do {
		/* load the list */
		list = (struct debug_alloc_list *) arcp_load(&debug_alloc);
		if (list == NULL) {
			/* nothing has been allocated yet; create a
			 * single-valued list */
			newlist = os_malloc(
					sizeof(struct debug_alloc_list)
					+ sizeof(void *) * 1);
			if (newlist == NULL) {
				DEBUG_PRINTF("os_malloc failed\n");
				STACKTRACE();
			}
			arcp_region_init(
				newlist,
				(arcp_destroy_f) __destroy_debug_alloc_list);
			newlist->ptrs[0] = (uintptr_t) ptr;
			newlist->len = 1;
		} else {
			/* binary search for ptr */
			l = 0;
			u = list->len;
			i = 0;
			while (l < u) {
				i = (l + u) / 2;
				if (((uintptr_t) ptr) < list->ptrs[i]) {
					u = i;
				} else if (((uintptr_t) ptr)
					   > list->ptrs[i]) {
					l = ++i;
				} else {
					/* ptr exists; it was already
					 * allocated */
					DEBUG_PRINTF("Trying to allocate "
						     "already-allocated %p\n",
						     ptr);
					STACKTRACE();
				}
			}
			/* not found, add it to the list */
			/* allocate new list */
			newlist = os_malloc(
					sizeof(struct debug_alloc_list)
					+ sizeof(void *) * (list->len + 1));
			if (newlist == NULL) {
				DEBUG_PRINTF("os_malloc failed\n");
				STACKTRACE();
			}
			arcp_region_init(
					newlist,
					(arcp_destroy_f)
					__destroy_debug_alloc_list);
			newlist->len = list->len + 1;
			/* copy over values, adding ptr at i */
			memcpy(newlist->ptrs, list->ptrs, sizeof(void *) * i);
			newlist->ptrs[i] = (uintptr_t) ptr;
			memcpy(newlist->ptrs + i + 1, list->ptrs + i,
			       sizeof(void *) * (list->len - i));
		}
	} while (!arcp_compare_store_release(&debug_alloc, list, newlist));
}

#  define CHECK_FREE(ptr)	check_free(ptr)
#  define MAYBE_CHECK_FREE(ptr, size) do {				\
		if (size != 0) {						\
			check_free(ptr);				\
		}							\
	} while (0)
#  define CHECK_ALLOC(ptr)	check_alloc(ptr)
#  define MAYBE_CHECK_ALLOC(ptr, size) do {				\
		if (size != 0) {						\
			check_alloc(ptr);				\
		}							\
	} while (0)

# endif /* AMALLOC_DEBUG */

/* void *amalloc(size_t size) { */
/* 	if (size == 0) { */
/* 		return NULL; */
/* 	} */
/* 	return os_alloc(size); */
/* } */

/* void afree(void *ptr, size_t size) { */
/* 	if (size == 0) { */
/* 		return; */
/* 	} */
/* 	os_free(ptr, size); */
/* } */

/* void *arealloc(void *ptr, size_t oldsize, size_t newsize) { */
/* 	if (oldsize == 0) { */
/* 		return amalloc(newsize); */
/* 	} else if (newsize == 0) { */
/* 		os_free(ptr, oldsize); */
/* 		return ptr; */
/* 	} */
/* 	return os_realloc(ptr, oldsize, newsize); */
/* } */

/* bool atryrealloc(void *ptr, size_t oldsize, size_t newsize) { */
/*	 if (oldsize == 0) { */
/*		 return newsize == 0; */
/*	 } else if (newsize == 0) { */
/*		 os_free(ptr, oldsize); */
/*		 return true; */
/*	 } */
/*	 return os_tryrealloc(ptr, oldsize, newsize); */
/* } */

/* Atomically pop a memory region of the specified size off the stack. Returns
 * NULL if the stack for that memory region is empty. */
static inline void *fstack_pop(int bin) {
	struct fstack_item *item;
	for (;;) {
		void *next;
		/* Acquire the top of the stack and update its reference
		 * count. */
		next = ak_load(&glbl_fstack[bin], mo_consume);
		do {
			while (PTR_COUNT(next) == MIN_SIZE - 1) {
				/* Spinlock if too many threads are accessing
				 * this at once. */
				cpu_yield();
				next = ak_load(&glbl_fstack[bin], mo_acquire);
			}
		} while (unlikely(!ak_cas(&glbl_fstack[bin], &next, next + 1,
					  mo_acq_rel, mo_consume)));
		next += 1;
		if (PTR_DECOUNT(next) == NULL) {
			/* The stack is currently empty; get rid of the
			 * reference count we just added to the NULL pointer.
			 */
			do {
				if (likely(ak_cas(&glbl_fstack[bin],
						  &next, NULL,
						  mo_acq_rel, mo_relaxed)
					   || next == NULL)) {
					/* Empty stack. */
					return NULL;
				}
			} while (PTR_DECOUNT(next) == NULL);
			/* Something was added while we were trying to update;
			 * try again. */
			continue;
		}
		item = (struct fstack_item *) PTR_DECOUNT(next);
# ifdef AMALLOC_DEBUG
		/* double-check stack alignment */
		if (bin2size(bin) >= PAGE_SIZE) {
			if ((((uintptr_t) item)
			     & (((uintptr_t) PAGE_SIZE) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned top of stack at %p "
					     "for %zd byte stack\n", item,
					     bin2size(bin));
				STACKTRACE();
			}
			if ((((uintptr_t) item->next)
			     & (((uintptr_t) PAGE_SIZE) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned next top of stack "
					     "at %p for %zd byte stack\n",
					     item->next, bin2size(bin));
				DEBUG_PRINTF("(Top of stack at %p)\n", item);
				STACKTRACE();
			}
		} else {
			if ((((uintptr_t) item)
			     & (((uintptr_t) bin2size(bin)) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned top of stack at %p "
					     "for %zd byte stack\n", item,
					     bin2size(bin));
				STACKTRACE();
			}
			if ((((uintptr_t) item->next)
			     & (((uintptr_t) bin2size(bin)) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned next top of stack "
					     "at %p for %zd byte stack\n",
					     item->next, bin2size(bin));
				DEBUG_PRINTF("(Top of stack at %p)\n", item);
				STACKTRACE();
			}
		}
# endif /* AMALLOC_DEBUG */
		/* Try to pop the item off the top of the stack. */
		do {
			if (likely(ak_cas(&glbl_fstack[bin],
					  &next, item->next,
					  mo_acq_rel, mo_relaxed))) {
				/* Transfer count. */
				ak_ldadd(&item->refcount, PTR_COUNT(next),
					 mo_acq_rel);
				break;
			}
		} while (PTR_DECOUNT(next) == item);
		/* The item is no longer the top of the stack, because someone
		 * successfully popped it. Release reference. If the refcount
		 * hasn't been transferred yet, then refcount will go negative
		 * instead of zero. */
		if (likely((ak_ldsub(&item->refcount, 1, mo_seq_cst) - 1)
			   == 0)) {
			/* We are the last to hold a reference to this item.
			 * As no one else can claim it; return it. */
			return (void *) item;
		}
		/* References to the item remain. Someone else will return it,
		 * or push will add it back to the stack. Loop and try again.
		 */
	}
}

/* Atomically push a memory region of the specified size on to the stack. */
static void fstack_push(int bin, void *ptr) {
	struct fstack_item *new_item;
	struct fstack_item *item;
	new_item = (struct fstack_item *) ptr;
	/* Set the initial refcount to zero. */
	ak_init(&new_item->refcount, 0);
	for (;;) {
		void *next;
		/* Get the top of the stack; we have to update reference count
		 * since we act like pop if there's more than our own
		 * reference count. */
		next = ak_load(&glbl_fstack[bin], mo_acquire);
		do {
			while (PTR_COUNT(next) == MIN_SIZE - 1) {
				/* Spinlock if too many threads are accessing
				 * this at once. */
				cpu_yield();
				next = ak_load(&glbl_fstack[bin], mo_acquire);
			}
		} while (unlikely(!ak_cas(&glbl_fstack[bin], &next, next + 1,
					  mo_acq_rel, mo_acquire)));
		next += 1;
		if (PTR_DECOUNT(next) == NULL) {
			/* This is the only item that will be on the stack. */
			new_item->next = NULL;
			do {
				if (likely(ak_cas(&glbl_fstack[bin],
						  &next, ptr,
						  mo_seq_cst, mo_acquire))) {
					/* Success! */
					return;
				}
			} while (PTR_DECOUNT(next) == NULL);  
			/* Something else was added before we could add this;
			 * try again. */
			continue;
		}
		item = (struct fstack_item *) PTR_DECOUNT(next);
# ifdef AMALLOC_DEBUG
		/* double-check stack alignment */
		if (bin2size(bin) >= PAGE_SIZE) {
			if ((((uintptr_t) item)
			     & (((uintptr_t) PAGE_SIZE) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned top of stack at %p "
					     "for %zd byte stack\n", item,
					     bin2size(bin));
				STACKTRACE();
			}
			if ((((uintptr_t) item->next)
			     & (((uintptr_t) PAGE_SIZE) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned next top of stack "
					     "at %p for %zd byte stack\n",
					     item->next, bin2size(bin));
				DEBUG_PRINTF("(Top of stack at %p)\n", item);
				STACKTRACE();
			}
		} else {
			if ((((uintptr_t) item)
			     & (((uintptr_t) bin2size(bin)) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned top of stack at %p "
					     "for %zd byte stack\n", item,
					     bin2size(bin));
				STACKTRACE();
			}
			if ((((uintptr_t) item->next)
			     & (((uintptr_t) bin2size(bin)) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned next top of stack "
					     "at %p for %zd byte stack\n",
					     item->next, bin2size(bin));
				DEBUG_PRINTF("(Top of stack at %p)\n", item);
				STACKTRACE();
			}
		}
# endif /* AMALLOC_DEBUG */
		if (likely(PTR_COUNT(next) == 1)) {
			new_item->next = PTR_DECOUNT(next);
			/* Try to push the item on to the top of the stack. */
			if (likely(ak_cas(&glbl_fstack[bin], &next, new_item,
					  mo_seq_cst, mo_acquire))) {
				/* Success! */
				return;
			}
		}
		/* Someone's trying to pop this item, or someone else is
		 * trying to push to it, too. Because we can't distinguish
		 * this situation, just help pop the item. */
		while (PTR_DECOUNT(next) == item) {
			if (likely(ak_cas(&glbl_fstack[bin],
					  &next, item->next,
					  mo_acq_rel, mo_acquire))) {
				/* Transfer count. */
				ak_ldadd(&item->refcount, PTR_COUNT(next),
					 mo_acq_rel);
				break;
			}
		}
		/* The item is no longer the top of the stack, because someone
		 * successfully popped it. Release reference. If the refcount
		 * hasn't been transferred yet, then refcount will go negative
		 * instead of zero. */
		if ((ak_ldsub(&item->refcount, 1, mo_seq_cst) - 1) == 0) {
			/* We are the last to hold a reference to this item.
			 * But we don't want it, so push this one, too. This
			 * is one of the more inelegant things about this
			 * algorithm... */
			fstack_push(bin, item);
		}
		/* Loop and try again. */
	}
}

void *amalloc(size_t size) {
	void *ret;
	int bin;
	int i;
	if (size == 0) {
		return NULL;
	}
	if (size > OS_THRESH) {
		/* allocate directly from the os */
		ret = os_alloc(size);
		CHECK_ALLOC(ret);
		return ret;
	}
	/* find the bin number for the requested size */
	bin = size2bin(size);
	/* try to pop increasingly larger chunks */
	for (i = bin; i < NSIZES; i++) {
		if ((ret = fstack_pop(i)) != NULL) {
			/* we popped a larger chunk than necessary, so go
			 * break it down */
			goto breakdown_chunk;
		}
	}
	/* allocate a new chunk */
	ret = os_alloc(OS_THRESH);
	if (ret == NULL) {
		return NULL;
	}
	/* i = NSIZES; -- already nsizes from for loop above */
	i -= 1;
breakdown_chunk:
	/* subdivide it, keeping the later half from the os, until the
	 * chunk we're left with is appropriate to the requested size. */
	while (i-- > bin) {
		fstack_push(i, ret + bin2size(i));
	}
# ifdef AMALLOC_DEBUG
	/* double-check alignment */
	if (size >= PAGE_SIZE) {
		if ((((uintptr_t) ret) & (((uintptr_t) PAGE_SIZE) - 1))
		    != 0) {
			DEBUG_PRINTF("Misaligned allocation of %zd bytes at "
				     "%p\n", size, ret);
			STACKTRACE();
		}
	} else {
		if ((((uintptr_t) ret)
		     & (((uintptr_t) bin2size(size2bin(size))) - 1))
		    != 0) {
			DEBUG_PRINTF("Misaligned allocation of %zd bytes at "
				     "%p\n", size, ret);
			STACKTRACE();
		}
	}
# endif /* AMALLOC_DEBUG */
	CHECK_ALLOC(ret);
	return ret;
}

void afree(void *ptr, size_t size) {
	MAYBE_CHECK_FREE(ptr, size);
	if (size == 0) {
		return;
	} else if (size > OS_THRESH) {
		/* free directly to the os */
		os_free(ptr, size);
	} else {
# ifdef AMALLOC_DEBUG
		/* double-check alignment */
		if (size >= PAGE_SIZE) {
			if ((((uintptr_t) ptr)
			     & (((uintptr_t) PAGE_SIZE) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned deallocation of "
					     "%zd bytes at %p\n", size, ptr);
				STACKTRACE();
			}
		} else {
			if ((((uintptr_t) ptr)
			    & (((uintptr_t) bin2size(size2bin(size))) - 1))
			   != 0) {
				DEBUG_PRINTF("Misaligned deallocation of "
					     "%zd bytes at %p\n", size, ptr);
				STACKTRACE();
			}
		}
# endif /* AMALLOC_DEBUG */
		/* push the chunk back on to the free list */
		fstack_push(size2bin(size), ptr);
	}
}

bool atryrealloc(void *ptr, size_t oldsize, size_t newsize) {
	MAYBE_CHECK_FREE(ptr, oldsize);
	MAYBE_CHECK_ALLOC(ptr, oldsize);
	if (oldsize == 0 && newsize == 0) {
		return true;
	} else if (oldsize > OS_THRESH && newsize > OS_THRESH) {
		/* try to reallocate directly from the OS */
		return os_tryrealloc(ptr, oldsize, newsize);
	} else {
# ifdef AMALLOC_DEBUG
		/* double-check alignment */
		if (oldsize >= PAGE_SIZE) {
			if ((((uintptr_t) ptr)
			     & (((uintptr_t) PAGE_SIZE) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned reallocation of "
					     "%zd bytes at %p\n", oldsize,
					     ptr);
				STACKTRACE();
			}
		} else {
			if ((((uintptr_t) ptr)
			     & (((uintptr_t) bin2size(size2bin(oldsize))) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned reallocation of "
					     "%zd bytes at %p\n", oldsize,
					     ptr);
				STACKTRACE();
			}
		}
# endif /* AMALLOC_DEBUG */
		/* if the chunk size is already large enough to accomodate the
		 * new allocation, then we succeed; otherwise, we fail */
		if (size2bin(oldsize) == size2bin(newsize)) {
			return true;
		} else {
			return false;
		}
	}
}

void *arealloc(void *ptr, size_t oldsize, size_t newsize) {
	void *ret;
	MAYBE_CHECK_FREE(ptr, oldsize);
	MAYBE_CHECK_ALLOC(ptr, oldsize);
	if (oldsize == 0) {
		if (newsize == 0) {
			return ptr;
		} else {
			/* simple allocation */
			MAYBE_CHECK_FREE(ptr, oldsize);
			return amalloc(newsize);
		}
	} else if (newsize == 0) {
		/* simple free */
		afree(ptr, oldsize);
		MAYBE_CHECK_ALLOC(ptr, newsize);
		return ptr;
	}
	if (oldsize > OS_THRESH && newsize > OS_THRESH) {
		/* reallocate directly from the OS */
		ret = os_realloc(ptr, oldsize, newsize);
		if (ret != NULL) {
			CHECK_FREE(ptr);
			CHECK_ALLOC(ret);
		}
		return ret;
	} else {
# ifdef AMALLOC_DEBUG
		/* double-check alignment */
		if (oldsize >= PAGE_SIZE) {
			if ((((uintptr_t) ptr)
			     & (((uintptr_t) PAGE_SIZE) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned reallocation of "
					     "%zd bytes at %p\n", oldsize,
					     ptr);
				STACKTRACE();
			}
		} else {
			if ((((uintptr_t) ptr)
			     & (((uintptr_t) bin2size(size2bin(oldsize))) - 1))
			    != 0) {
				DEBUG_PRINTF("Misaligned reallocation of "
					     "%zd bytes at %p\n", oldsize,
					     ptr);
				STACKTRACE();
			}
		}
# endif /* AMALLOC_DEBUG */
		/* if the chunk size is the same, do nothing */
		if (size2bin(oldsize) == size2bin(newsize)) {
			return ptr;
		}
	}
	/* allocate a new chunk */
	ret = amalloc(newsize);
	if (ret == NULL) {
		return NULL;
	}
	/* copy over the contents */
	memcpy(ret, ptr, oldsize > newsize ? newsize : oldsize);
	/* free the old chunk */
	afree(ptr, oldsize);
	return ret;
}

#else /* AMALLOC_VALGRIND_DEBUG */

# include <malloc.h>

void *amalloc(size_t size) {
	return malloc(size);
}

void afree(void *ptr, size_t size __attribute__((unused))) {
	free(ptr);
}

bool atryrealloc(void *ptr __attribute__((unused)),
		 size_t oldsize __attribute__((unused)),
		 size_t newsize __attribute__((unused))) {
	return false;
}

void *arealloc(void *ptr,
	       size_t oldsize __attribute__((unused)), size_t newsize) {
	return realloc(ptr, newsize);
}

#endif /* AMALLOC_VALGRIND_DEBUG */
