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
#ifndef _ATOMICKIT_ATOMIC_LIST_H
#define _ATOMICKIT_ATOMIC_LIST_H

#include <atomickit/spinlock.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct atomic_list_struct {
    void **list_ptr;
    struct atomic_list_struct *iterators;
    size_t capacity;
    size_t length;
    spinlock_t lock;
} atomic_list_t;

#define ALST_ERROR ((void *) (~ ((intptr_t) 0x5555)))
#define ALST_EMPTY ((void *) (~ ((intptr_t) 0xAAA9)))

#define ALST_OUT_OF_BOUNDS(length, index)	\
    ((((size_t) index) >= (length)) || ((index) < 0))

/* nonatomic read functions -- should be *fast* */

static inline void *nonatomic_list_get(atomic_list_t *list, off_t index) {
    if(ALST_OUT_OF_BOUNDS(list->length, index)) {
	errno = EFAULT;
	return ALST_ERROR;
    }
    return list->list_ptr[index];
}

static inline void *nonatomic_list_first(atomic_list_t *list) {
    if(list->length == 0) {
	return ALST_EMPTY;
    } else {
	return list->list_ptr[0];
    }
}

static inline void *nonatomic_list_last(atomic_list_t *list) {
    if(list->length == 0) {
	return ALST_EMPTY;
    } else {
	return list->list_ptr[list->length - 1];
    }
}

static inline void **nonatomic_list_ary(atomic_list_t *list) {
    return list->list_ptr;
}

static inline size_t nonatomic_list_length(atomic_list_t *list) {
    return list->length;
}

static inline void atomic_list_readlock(atomic_list_t *list) {
    spinlock_multilock(&list->lock);
}

static inline void atomic_list_readunlock(atomic_list_t *list) {
    spinlock_unlock(&list->lock);
}

static inline void *atomic_list_get(atomic_list_t *list, off_t index) {
    void *ret;
    atomic_list_readlock(list);
    ret = nonatomic_list_get(list, index);
    atomic_list_readunlock(list);
    return ret;
}

static inline void *atomic_list_first(atomic_list_t *list) {
    void *ret;
    atomic_list_readlock(list);
    ret = nonatomic_list_first(list);
    atomic_list_readunlock(list);
    return ret;
}

static inline void *atomic_list_last(atomic_list_t *list) {
    void *ret;
    atomic_list_readlock(list);
    ret = nonatomic_list_last(list);
    atomic_list_readunlock(list);
    return ret;
}

static inline size_t atomic_list_length(atomic_list_t *list) {
    size_t ret;
    atomic_list_readlock(list);
    ret = nonatomic_list_length(list);
    atomic_list_readunlock(list);
    return ret;
}

typedef off_t atomic_iterator_t;

int atomic_iterator_init(atomic_list_t *list, atomic_iterator_t *iterator);

void atomic_iterator_destroy(atomic_list_t *list, atomic_iterator_t *iterator);

static inline void *atomic_iterator_next(atomic_list_t *list, atomic_iterator_t *iterator) {
    void *ret;
    atomic_list_readlock(list);
    ret = nonatomic_list_get(list, (*iterator)++);
    atomic_list_readunlock(list);
    return ret == ALST_ERROR ? ALST_EMPTY : ret;
}

int atomic_list_init(atomic_list_t *list);
int atomic_list_init_with_capacity(atomic_list_t *list, size_t initial_capacity);
void atomic_list_destroy(atomic_list_t *list);
int atomic_list_compact(atomic_list_t *list);
int atomic_list_prealloc(atomic_list_t *list, size_t size);

/* Write functions */
void **atomic_list_checkout(atomic_list_t *list);
void atomic_list_checkin(atomic_list_t *list, void **ary, size_t length);

int atomic_list_set(atomic_list_t *list, off_t index, void *item);
int atomic_list_push(atomic_list_t *list, void *item);
void *atomic_list_pop(atomic_list_t *list);
int atomic_list_unshift(atomic_list_t *list, void *item);
void *atomic_list_shift(atomic_list_t *list);
int atomic_list_insert(atomic_list_t *list, off_t index, void *item);
void *atomic_list_remove(atomic_list_t *list, off_t index);
void atomic_list_remove_by_value(atomic_list_t *list, void *item);
void atomic_list_reverse(atomic_list_t *list);
void atomic_list_remove_by_exec(atomic_list_t *list, int(*grep)(void *));
void atomic_list_sort(atomic_list_t *list, int(*compar)(void *, void *));
int atomic_list_insert_sorted(atomic_list_t *list, int(*compar)(void *, void *), void *item);
void atomic_list_clear(atomic_list_t *list);

#endif /* #ifndef _ATOMICKIT_ATOMIC_LIST_H */
