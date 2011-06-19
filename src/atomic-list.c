/*
 * atomic-list.c
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
#define _GNU_SOURCE
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "atomickit/spinlock.h"
#include "atomickit/atomic-list.h"

#define atomic_list_lock(list) spinlock_lock(&((list)->lock))
#define atomic_list_unlock(list) atomic_list_readunlock(list)
#define ALST_DEFAULT_CAPACITY 10

static inline void invalidate_iterators(atomic_list_t *list) {
    if(list->iterators != NULL) {
	size_t **ary = (size_t **) nonatomic_list_ary(list->iterators);
	size_t i;
	for(i = 0; i < nonatomic_list_length(list->iterators); i++) {
	    *ary[i] = SIZE_MAX;
	}
    }
}

static inline void decrement_iterators(atomic_list_t *list, off_t index) {
    if(list->iterators != NULL) {
	size_t **ary = (size_t **) nonatomic_list_ary(list->iterators);
	size_t i;
	for(i = 0; i < nonatomic_list_length(list->iterators); i++) {
	    if(*ary[i] > (size_t) index) {
		(*ary[i])--;
	    }
	}
    }
}

static inline void increment_iterators(atomic_list_t *list, off_t index) {
    if(list->iterators != NULL) {
	size_t **ary = (size_t **) nonatomic_list_ary(list->iterators);
	size_t i;
	for(i = 0; i < nonatomic_list_length(list->iterators); i++) {
	    if(*ary[i] != SIZE_MAX && *ary[i] > (size_t) index) {
		(*ary[i])++;
	    }
	}
    }
}

static inline int atomic_list_init_internal(atomic_list_t *list, size_t initial_capacity, bool init_iterators) {
    list->list_ptr = malloc(sizeof(void *) * initial_capacity);
    if(list->list_ptr == NULL) {
	return -1;
    }
    if(init_iterators) {
	list->iterators = malloc(sizeof(atomic_list_t));
	if(list->iterators == NULL) {
	    free(list->list_ptr);
	    return -1;
	}
	int r = atomic_list_init_internal(list->iterators, ALST_DEFAULT_CAPACITY, false);
	if(r != 0) {
	    free(list->iterators);
	    free(list->list_ptr);
	    return r;
	}
    } else {
	list->iterators = NULL;
    }

    list->capacity = initial_capacity;
    list->length = 0;
    spinlock_init(&list->lock);
    return 0;
}


int atomic_list_init_with_capacity(atomic_list_t *list, size_t initial_capacity) {
    return atomic_list_init_internal(list, initial_capacity, true);
}

int atomic_list_init(atomic_list_t *list) {
    return atomic_list_init_internal(list, ALST_DEFAULT_CAPACITY, true);
}

void atomic_list_destroy(atomic_list_t *list) {
    atomic_list_lock(list);
    if(list->iterators != NULL) {
	atomic_list_destroy(list->iterators);
	free(list->iterators);
	list->iterators = NULL;
    }
    if(list->list_ptr != NULL) {
	free(list->list_ptr);
	list->list_ptr = NULL;
    }
    list->capacity = 0;
    list->length = 0;
    atomic_list_unlock(list);
}

static inline int atomic_list_compact_internal(atomic_list_t *list) {
    if(list->length != list->capacity) {
	void **new_list = realloc(list->list_ptr, sizeof(void *) * list->length);
	if(new_list == NULL) {
	    atomic_list_unlock(list);
	    return -1;
	}
	list->list_ptr = new_list;
	list->capacity = list->length;
    }
    if(list->iterators != NULL) {
	int r = atomic_list_compact(list->iterators);
	atomic_list_unlock(list);
	return r;
    }
    return 0;
}

int atomic_list_compact(atomic_list_t *list) {
    atomic_list_lock(list);
    atomic_list_compact_internal(list);
    atomic_list_unlock(list);
    return 0;
}

int atomic_list_prealloc(atomic_list_t *list, size_t capacity) {
    atomic_list_lock(list);
    if(list->capacity < capacity) {
	void **new_list = realloc(list->list_ptr, sizeof(void *) * capacity);
	if(new_list == NULL) {
	    atomic_list_unlock(list);
	    return -1;
	}
	list->list_ptr = new_list;
	list->capacity = capacity;
    }
    atomic_list_unlock(list);
    return 0;
}

/* Assume we're already locked */
static int ensure_capacity(atomic_list_t *list, size_t capacity) {
    size_t new_capacity = list->capacity;
    while(capacity > new_capacity) {
	/* Increase by half of current capacity */
	new_capacity = new_capacity + (new_capacity >> 1);
    }
    void **new_list = realloc(list->list_ptr, sizeof(void *) * new_capacity);
    if(new_list == NULL) {
	return -1;
    }
    list->list_ptr = new_list;
    list->capacity = new_capacity;
    return 0;
}

void **atomic_list_checkout(atomic_list_t *list) {
    atomic_list_lock(list);
    int r = atomic_list_compact_internal(list);
    if(r != 0) {
	atomic_list_unlock(list);
	return NULL;
    }
    return list->list_ptr;
}

void atomic_list_checkin(atomic_list_t *list, void **ary, size_t length) {
    list->list_ptr = ary;
    list->capacity = length;
    list->length = length;
    invalidate_iterators(list);
    atomic_list_unlock(list);
}

int atomic_list_set(atomic_list_t *list, off_t index, void *item) {
    atomic_list_lock(list);
    if(ALST_OUT_OF_BOUNDS(list->length, index)) {
	atomic_list_unlock(list);
	errno = EFAULT;
	return -1;
    }
    list->list_ptr[index] = item;
    atomic_list_unlock(list);
    return 0;
}

int atomic_list_push(atomic_list_t *list, void *item) {
    int r;
    atomic_list_lock(list);
    r = ensure_capacity(list, list->length + 1);
    if(r != 0) {
	atomic_list_unlock(list);
	return r;
    }
    list->list_ptr[list->length++] = item;
    atomic_list_unlock(list);
    return 0;
}

void *atomic_list_pop(atomic_list_t *list) {
    void *ret;
    atomic_list_lock(list);
    if(list->length == 0) {
	return ALST_EMPTY;
    }
    ret = list->list_ptr[--list->length];
    atomic_list_unlock(list);
    return ret;
}

int atomic_list_unshift(atomic_list_t *list, void *item) {
    int r;
    atomic_list_lock(list);
    r = ensure_capacity(list, list->length + 1);
    if(r != 0) {
	atomic_list_unlock(list);
	return r;
    }
    memmove(list->list_ptr + 1, list->list_ptr, list->length++ * sizeof(void *));
    list->list_ptr[0] = item;
    increment_iterators(list, 0);
    atomic_list_unlock(list);
    return 0;
}

void *atomic_list_shift(atomic_list_t *list) {
    void *ret;
    atomic_list_lock(list);
    if(list->length == 0) {
	return ALST_EMPTY;
    }
    ret = list->list_ptr[0];
    memmove(list->list_ptr, list->list_ptr + 1, --list->length * sizeof(void *));
    decrement_iterators(list, 0);
    atomic_list_unlock(list);
    return ret;
}

int atomic_list_insert(atomic_list_t *list, off_t index, void *item) {
    int r;
    atomic_list_lock(list);
    if(ALST_OUT_OF_BOUNDS(list->length + 1, index)) {
	atomic_list_unlock(list);
	errno = EFAULT;
	return -1;
    }
    r = ensure_capacity(list, list->length + 1);
    if(r != 0) {
	atomic_list_unlock(list);
	return r;
    }
    memmove(list->list_ptr + index + 1, list->list_ptr + index, (list->length++ - index) * sizeof(void *));
    list->list_ptr[index] = item;
    increment_iterators(list, index);
    atomic_list_unlock(list);
    return 0;
}

void *atomic_list_remove(atomic_list_t *list, off_t index) {
    void *ret;
    atomic_list_lock(list);
    if(ALST_OUT_OF_BOUNDS(list->length + 1, index)) {
	atomic_list_unlock(list);
	errno = EFAULT;
	return ALST_ERROR;
    }
    ret = list->list_ptr[index];
    memmove(list->list_ptr + index, list->list_ptr + index + 1, (--list->length - index) * sizeof(void *));
    decrement_iterators(list, index);
    atomic_list_unlock(list);
    return ret;
}

void atomic_list_remove_by_value(atomic_list_t *list, void *value) {
    size_t i;
    atomic_list_lock(list);
    for(i = 0; i < list->length; i++) {
	if(list->list_ptr[i] == value) {
	    memmove(list->list_ptr + i, list->list_ptr + i + 1, (--list->length - i) * sizeof(void *));
	    decrement_iterators(list, i);
	}
    }
    atomic_list_unlock(list);
}

void atomic_list_remove_by_exec(atomic_list_t *list, int(*grep)(void *)) {
    size_t i;
    atomic_list_lock(list);
    for(i = 0; i < list->length; i++) {
	if(grep(list->list_ptr[i])) {
	    memmove(list->list_ptr + i, list->list_ptr + i + 1, (--list->length - i) * sizeof(void *));
	    decrement_iterators(list, i);
	}
    }
    atomic_list_unlock(list);
}

void atomic_list_reverse(atomic_list_t *list) {
    atomic_list_lock(list);
    size_t i;
    for(i = 0; i < (list->length >> 1); i++) {
	register void *tmp;
	tmp = list->list_ptr[i];
	list->list_ptr[i] = list->list_ptr[list->length - 1 - i];
	list->list_ptr[list->length - 1 - i] = tmp;
    }
    invalidate_iterators(list);
    atomic_list_unlock(list);
}

static int compar_internal(const void *a, const void *b, void *arg) {
    int(*compar)(void *, void *) = (int(*)(void *, void *)) arg;
    void **_a = (void **) a;
    void **_b = (void **) b;
    return compar(_a, _b);
}

void atomic_list_sort(atomic_list_t *list, int(*compar)(void *, void *)) {
    atomic_list_lock(list);
    qsort_r(list->list_ptr, list->length, sizeof(void *), compar_internal, (void *) compar);
    invalidate_iterators(list);
    atomic_list_unlock(list);
}

int atomic_list_insert_sorted(atomic_list_t *list, int(*compar)(void *, void *), void *item) {
    int r;
    atomic_list_lock(list);
    r = ensure_capacity(list, list->length + 1);
    if(r != 0) {
	atomic_list_unlock(list);
	return r;
    }
    size_t max = list->length;
    size_t min = 0;
    size_t i = 0;
    while(min < max) {
	i = (max + min) >> 1;
	r = compar(item, list->list_ptr[i]);
	if(r < 0) {
	    max = i;
	} else if(r > 0) {
	    min = i + 1;
	} else {
	    break;
	}
    }
    memmove(list->list_ptr + i, list->list_ptr + i + 1, (list->length++ - i) * sizeof(void *));
    list->list_ptr[i] = item;
    increment_iterators(list, i);
    atomic_list_unlock(list);
    return 0;
}

void atomic_list_clear(atomic_list_t *list) {
    atomic_list_lock(list);
    list->length = 0;
    atomic_list_unlock(list);
}

int atomic_iterator_init(atomic_list_t *list, atomic_iterator_t *iterator) {
    int r;
    atomic_list_readlock(list);
    *iterator = 0;
    r = atomic_list_push(list->iterators, iterator);
    atomic_list_readunlock(list);
    return r;
}

void atomic_iterator_destroy(atomic_list_t *list, atomic_iterator_t *iterator) {
    atomic_list_readlock(list);
    *iterator = SIZE_MAX;
    atomic_list_remove_by_value(list->iterators, iterator);
    atomic_list_readunlock(list);
}
