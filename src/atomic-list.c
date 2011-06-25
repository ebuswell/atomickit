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
#include <stdio.h>
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

char *atomic_list_to_string(atomic_list_t *list, char *(*item_to_string)(void *, int)) {
    return atomic_list_to_string_indent(list, 0, item_to_string);
}

static char *iterator_to_string(void *item, int indentlevel) {
    off_t *iterator = (off_t *) item;
    char *tmp = alloca(21);
    int r = sprintf(tmp, "%td", *iterator);
    if(r < 0) {
	return NULL;
    }
    size_t length = strlen(tmp);
    char *ret = malloc(length + 1);
    if(ret == NULL) {
	return NULL;
    }
    strcpy(ret, tmp);
    return ret;
}

char *atomic_list_to_string_indent(atomic_list_t *list, int indentlevel, char *(*item_to_string)(void *, int)) {
    int r;
    atomic_list_readlock(list);
    /* First do the actual list */
    char *list_str;
    if(list->length > 0) {
	size_t maxlen = list->length > 10 ? 10 : list->length;
	char **strings = alloca(maxlen * sizeof(char *));
	size_t length = 0;
	size_t i;
	for(i = 0; i < maxlen; i++) {
	    strings[i] = item_to_string(list->list_ptr[i], indentlevel + 8);
	    if(strings[i] == NULL) {
		size_t j;
		for(j = 0; j < i; j++) {
		    free(strings[j]);
		}
		atomic_list_readunlock(list);
		return NULL;
	    }
	    length += strlen(strings[i]);
	}
	list_str = alloca(2 /*[\n*/
			  + ((indentlevel + 8) + 2 /*,\n*/) * maxlen - (maxlen == list->length ? 1 /*,*/ : 0)
			  + (maxlen < list->length ? (indentlevel + 8) + 4 /*...\n*/ : 0)
			  + ((indentlevel + 4) + 2 /*]\0*/)
			  + length);
	char *list_str_i = list_str;
	strcpy(list_str_i, "[\n");
	list_str_i += 2;
	for(i = 0; i < maxlen; i++) {
	    memset(list_str_i, ' ', indentlevel + 8);
	    list_str_i += indentlevel + 8;
	    strcpy(list_str_i, strings[i]);
	    list_str_i += strlen(strings[i]);
	    free(strings[i]);
	    if((i < maxlen - 1) || maxlen < list->length) {
		strcpy(list_str_i, ",\n");
		list_str_i += 2;
	    } else {
		strcpy(list_str_i, "\n");
		list_str_i += 1;
	    }
	}
	if(maxlen < list->length) {
	    memset(list_str_i, ' ', indentlevel + 8);
	    list_str_i += indentlevel + 8;
	    strcpy(list_str_i, "...\n");
	    list_str_i += 4;
	}
	memset(list_str_i, ' ', indentlevel + 4);
	list_str_i += indentlevel + 4;
	strcpy(list_str_i, "]");
    } else {
	list_str = "[]";
    }

    char *capacity_str = alloca(21);
    r = sprintf(capacity_str, "%zd", list->capacity);
    if(r < 0) {
	atomic_list_readunlock(list);
	return NULL;
    }
    char *length_str = alloca(21);
    r = sprintf(length_str, "%zd", list->length);
    if(r < 0) {
	atomic_list_readunlock(list);
	return NULL;
    }
    char *lock_str = alloca(11);
    r = sprintf(lock_str, "%d", atomic_read(&list->lock));
    if(r < 0) {
	atomic_list_readunlock(list);
	return NULL;
    }
    char *iterators_str = list->iterators == NULL ? "NULL" : atomic_list_to_string_indent(list->iterators, indentlevel + 4, iterator_to_string);
    if(iterators_str == NULL) {
	atomic_list_readunlock(list);
	return NULL;
    }

    char *ret = malloc(2 /*{\n*/
		       + indentlevel + 4 + strlen("capacity = ") + strlen(capacity_str) + 2 /*,\n*/
		       + indentlevel + 4 + strlen("length = ") + strlen(length_str) + 2
		       + indentlevel + 4 + strlen("lock = ") + strlen(lock_str) + 2
		       + indentlevel + 4 + strlen("iterators = ") + strlen(iterators_str) + 2
		       + indentlevel + 4 + strlen("list_ptr = ") + strlen(list_str) + 1 /*\n*/
		       + indentlevel + 2 /*}\0*/);
    if(ret == NULL) {
	if(list->iterators != NULL) {
	    free(iterators_str);
	}
	atomic_list_readunlock(list);
	return NULL;
    }
    char *ret_i = ret;

    strcpy(ret_i, "{\n");
    ret_i += 2;

    memset(ret_i, ' ', indentlevel + 4);
    ret_i += indentlevel + 4;
    strcpy(ret_i, "capacity = ");
    ret_i += strlen("capacity = ");
    strcpy(ret_i, capacity_str);
    ret_i += strlen(capacity_str);
    strcpy(ret_i, ",\n");
    ret_i += 2;

    memset(ret_i, ' ', indentlevel + 4);
    ret_i += indentlevel + 4;
    strcpy(ret_i, "length = ");
    ret_i += strlen("length = ");
    strcpy(ret_i, length_str);
    ret_i += strlen(length_str);
    strcpy(ret_i, ",\n");
    ret_i += 2;

    memset(ret_i, ' ', indentlevel + 4);
    ret_i += indentlevel + 4;
    strcpy(ret_i, "lock = ");
    ret_i += strlen("lock = ");
    strcpy(ret_i, lock_str);
    ret_i += strlen(lock_str);
    strcpy(ret_i, ",\n");
    ret_i += 2;

    memset(ret_i, ' ', indentlevel + 4);
    ret_i += indentlevel + 4;
    strcpy(ret_i, "iterators = ");
    ret_i += strlen("iterators = ");
    strcpy(ret_i, iterators_str);
    ret_i += strlen(iterators_str);
    if(list->iterators != NULL) {
	free(iterators_str);
    }
    strcpy(ret_i, ",\n");
    ret_i += 2;

    memset(ret_i, ' ', indentlevel + 4);
    ret_i += indentlevel + 4;
    strcpy(ret_i, "list_ptr = ");
    ret_i += strlen("list_ptr = ");
    strcpy(ret_i, list_str);
    ret_i += strlen(list_str);
    strcpy(ret_i, "\n");
    ret_i += 1;

    memset(ret_i, ' ', indentlevel);
    ret_i += indentlevel;
    strcpy(ret_i, "}");
    atomic_list_readunlock(list);
    return ret;
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
	invalidate_iterators(list);
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
	if(new_list == NULL && list->length != 0) {
	    atomic_list_unlock(list);
	    return -1;
	}
	list->list_ptr = new_list;
	list->capacity = list->length;
    }
    if(list->iterators != NULL) {
	return atomic_list_compact(list->iterators);
    }
    return 0;
}

int atomic_list_compact(atomic_list_t *list) {
    int r;
    atomic_list_lock(list);
    r = atomic_list_compact_internal(list);
    atomic_list_unlock(list);
    return r;
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
    if(new_capacity < 2) {
	new_capacity = 2;
    }
    while(capacity > new_capacity) {
	/* Increase by half of current capacity */
	new_capacity += (new_capacity >> 1);
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
	atomic_list_unlock(list);
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
    increment_iterators(list, 0);
    list->list_ptr[0] = item;
    atomic_list_unlock(list);
    return 0;
}

void *atomic_list_shift(atomic_list_t *list) {
    void *ret;
    atomic_list_lock(list);
    if(list->length == 0) {
	atomic_list_unlock(list);
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
    if(ALST_OUT_OF_BOUNDS(list->length, index)) {
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
    for(i = 0; i < list->length;) {
	if(list->list_ptr[i] == value) {
	    memmove(list->list_ptr + i, list->list_ptr + i + 1, (--list->length - i) * sizeof(void *));
	    decrement_iterators(list, i);
	} else {
	    i++;
	}
    }
    atomic_list_unlock(list);
}

void atomic_list_remove_by_exec(atomic_list_t *list, int(*grep)(void *)) {
    size_t i;
    atomic_list_lock(list);
    i = 0;
    while(i < list->length) {
	if(grep(list->list_ptr[i])) {
	    memmove(list->list_ptr + i, list->list_ptr + i + 1, (--list->length - i) * sizeof(void *));
	    decrement_iterators(list, i);
	} else {
	    i++;
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
	r = compar(list->list_ptr[i], item);
	if(r > 0) {
	    max = i;
	} else if(r < 0) {
	    min = ++i;
	} else {
	    break;
	}
    }
    memmove(list->list_ptr + i + 1, list->list_ptr + i, (list->length++ - i) * sizeof(void *));
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
