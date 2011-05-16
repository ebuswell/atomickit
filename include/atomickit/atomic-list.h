/*
 * atomic-list.h
 * 
 * Copyright 2011 Evan Buswell
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
/*
 * This is a one-sided wait-free algorithm for a modifiable list.  The
 * design requirements were as follows:
 *
 * Reader:
 *  * Should be as fast as possible.
 *  * Must be wait-free.
 *  * Locking is not recursive -- only one thread may lock or another
 *    mechanism must be used to manage this.
 *
 * Writer:
 *  * Writing is assumed to be rare and uncontentious.  Blocking and
 *    system calls are allowed.
 *  * No particular speed requirements.
 *  * Cannot lock structure.
 *
 */
#ifndef _ATOMICKIT_ATOMIC_LIST_H
#define _ATOMICKIT_ATOMIC_LIST_H

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <atomickit/atomic-ptr.h>

typedef struct {
    pthread_mutex_t mutex;
    atomic_ptr_t list_ptr;
} atomic_list_t;

struct atomic_list {
    size_t length;
    void *list[1];
};

#define ALST_MARKED(p)					\
    ((int) (((intptr_t) (p)) & ((intptr_t) 0x1)))
#define ALST_MARK(p)						\
    ((__typeof__(p)) (((intptr_t) (p)) | ((intptr_t) 0x1)))
#define ALST_UNMARK(p)						\
    ((__typeof__(p)) (((intptr_t) (p)) & (~ ((intptr_t) 0x1))))

#define ALST_ERROR ((void *) (~ ((intptr_t) 0x5555)))
#define ALST_EMPTY ((void *) (~ ((intptr_t) 0xAAA9)))

/* nonatomic read functions -- should be *fast* */
/* *must* acquire the read lock before using */

static inline void *nonatomic_list_get(atomic_list_t *list, off_t index) {
    struct atomic_list *list_ptr;
    list_ptr = ALST_UNMARK((struct atomic_list *) atomic_ptr_read(&list->list_ptr));
    return list_ptr->list[index];
}

static inline void *nonatomic_list_first(atomic_list_t *list) {
    struct atomic_list *list_ptr;
    list_ptr = ALST_UNMARK((struct atomic_list *) atomic_ptr_read(&list->list_ptr));
    return list_ptr->list[0];
}

static inline void *nonatomic_list_last(atomic_list_t *list) {
    struct atomic_list *list_ptr;
    list_ptr = ALST_UNMARK((struct atomic_list *) atomic_ptr_read(&list->list_ptr));
    return list_ptr->list[list_ptr->length - 1];
}

static inline void **nonatomic_list_ary(atomic_list_t *list) {
    struct atomic_list *list_ptr;
    list_ptr = ALST_UNMARK((struct atomic_list *) atomic_ptr_read(&list->list_ptr));
    return list_ptr->list;
}

static inline size_t nonatomic_list_length(atomic_list_t *list) {
    struct atomic_list *list_ptr;
    list_ptr = ALST_UNMARK((struct atomic_list *) atomic_ptr_read(&list->list_ptr));
    return list_ptr->length;
}

/* practically wait-free locking functions */
static inline void atomic_list_readlock(atomic_list_t *list) {
    struct atomic_list *list_ptr;
    for(;;) {
	list_ptr = (struct atomic_list *) atomic_ptr_read(&list->list_ptr);
	if(ALST_MARKED(list_ptr)) {
	    break;
	}
	if(likely(atomic_ptr_cmpxchg(&list->list_ptr, list_ptr, ALST_MARK(list_ptr))
		  == list_ptr)) {
	    break;
	}
    }
}

static inline void atomic_list_readunlock(atomic_list_t *list) {
    /* a simple set is safe as it is assumed that the read lock is held. */
    atomic_ptr_set(&list->list_ptr,
		   ALST_UNMARK(atomic_ptr_read(&list->list_ptr)));
}

/* Write functions: no particular speed requirements */

int atomic_list_replace(atomic_list_t *list, void **item_ary, size_t length);
int atomic_list_set(atomic_list_t *list, off_t index, void *item);
int atomic_list_push(atomic_list_t *list, void *item);
void *atomic_list_pop(atomic_list_t *list);
int atomic_list_unshift(atomic_list_t *list, void *item);
void *atomic_list_shift(atomic_list_t *list);
int atomic_list_insert(atomic_list_t *list, off_t index, void *item);
void *atomic_list_remove(atomic_list_t *list, off_t index);
int atomic_list_remove_by_value(atomic_list_t *list, void *item);
int atomic_list_clear(atomic_list_t *list);
int atomic_list_init(atomic_list_t *list);
int atomic_list_destroy(atomic_list_t *list);

void *atomic_list_get(atomic_list_t *list, off_t index);
void *atomic_list_first(atomic_list_t *list);
void *atomic_list_last(atomic_list_t *list);
size_t atomic_list_length(atomic_list_t *list);


#endif /* #ifndef _ATOMICKIT_ATOMIC_LIST_H */
