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
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include "atomickit/atomic-list.h"
#include "atomickit/atomic-ptr.h"

#define ALST_OUT_OF_BOUNDS(length, index)	\
    ((((size_t) index) >= (length)) || ((index) < 0))

#define XCHG_WHEN_UNMARKED(list, old, new)				\
    do {								\
	while(ALST_MARKED(old)) {					\
	    sched_yield(); /* don't check return value; spin on error */ \
	    old = atomic_ptr_read(&(list)->list_ptr);			\
	}								\
    } while(atomic_ptr_cmpxchg(&(list)->list_ptr, old, new) != old)

#define ALST_SIZE(n) (sizeof(struct atomic_list) + ((n) - 1) * sizeof(void *))

int atomic_list_replace(atomic_list_t *list, void **item_ary, size_t length) {
    struct atomic_list *old_list_ptr;
    struct atomic_list *new_list_ptr;
    /* Acquire write lock */
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return r;
    }
    /* Get current list_ptr */
    old_list_ptr = atomic_ptr_read(&list->list_ptr);
    /* allocate new list and copy data */
    new_list_ptr = malloc(ALST_SIZE(length));
    if(new_list_ptr == NULL) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return r;
	}
	return -1;
    }
    memcpy(new_list_ptr->list, item_ary, sizeof(void *) * length);
    new_list_ptr->length = length;
    /* Exchange new and old lists and clean up */
    XCHG_WHEN_UNMARKED(list, old_list_ptr, new_list_ptr);
    pthread_mutex_unlock(&list->mutex); /* Call already succeeded:
					 * ignore return value */
    free(old_list_ptr);

    return 0;
}

int atomic_list_set(atomic_list_t *list, off_t index, void *item) {
    struct atomic_list *old_list_ptr;
    struct atomic_list *new_list_ptr;
    /* Acquire write lock */
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return r;
    }
    /* Get current list_ptr */
    old_list_ptr = atomic_ptr_read(&list->list_ptr);
    /* Check bounds */
    if(ALST_OUT_OF_BOUNDS(ALST_UNMARK(old_list_ptr)->length, index)) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return r;
	}
	errno = EFAULT;
	return -1;
    }
    /* allocate new list and copy data */
    new_list_ptr = malloc(ALST_SIZE(ALST_UNMARK(old_list_ptr)->length));
    if(new_list_ptr == NULL) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return r;
	}
	return -1;
    }
    memcpy(new_list_ptr, ALST_UNMARK(old_list_ptr), ALST_SIZE(ALST_UNMARK(old_list_ptr)->length));
    new_list_ptr->list[index] = item;
    /* Exchange new and old lists and clean up */
    XCHG_WHEN_UNMARKED(list, old_list_ptr, new_list_ptr);
    pthread_mutex_unlock(&list->mutex); /* Call already succeeded:
					 * ignore return value */
    free(old_list_ptr);

    return 0;
}

int atomic_list_push(atomic_list_t *list, void *item) {
    struct atomic_list *old_list_ptr;
    struct atomic_list *new_list_ptr;
    /* Acquire write lock */
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return r;
    }
    /* Get current list_ptr */
    old_list_ptr = atomic_ptr_read(&list->list_ptr);
    /* allocate new list and copy data */
    new_list_ptr = malloc(ALST_SIZE(ALST_UNMARK(old_list_ptr)->length + 1));
    if(new_list_ptr == NULL) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return r;
	}
	return -1;
    }
    memcpy(new_list_ptr, ALST_UNMARK(old_list_ptr), ALST_SIZE(ALST_UNMARK(old_list_ptr)->length));
    new_list_ptr->list[ALST_UNMARK(old_list_ptr)->length] = item;
    new_list_ptr->length++;
    /* Exchange new and old lists and clean up */
    XCHG_WHEN_UNMARKED(list, old_list_ptr, new_list_ptr);
    pthread_mutex_unlock(&list->mutex); /* Call already succeeded:
					 * ignore return value */
    free(old_list_ptr);

    return 0;
}

void *atomic_list_pop(atomic_list_t *list) {
    struct atomic_list *old_list_ptr;
    struct atomic_list *new_list_ptr;
    /* Acquire write lock */
    void *ret;
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return ALST_ERROR;
    }
    /* Get current list_ptr */
    old_list_ptr = atomic_ptr_read(&list->list_ptr);
    if(ALST_UNMARK(old_list_ptr)->length == 0) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return ALST_ERROR;
	}
	return ALST_EMPTY;
    }
    /* allocate new list and copy data */
    new_list_ptr = malloc(ALST_SIZE(ALST_UNMARK(old_list_ptr)->length - 1));
    if(new_list_ptr == NULL) {
	pthread_mutex_unlock(&list->mutex);
	return ALST_ERROR;
    }
    memcpy(new_list_ptr, ALST_UNMARK(old_list_ptr), ALST_SIZE(ALST_UNMARK(old_list_ptr)->length - 1));
    ret = ALST_UNMARK(old_list_ptr)->list[ALST_UNMARK(old_list_ptr)->length - 1];
    new_list_ptr->length--;
    /* Exchange new and old lists and clean up */
    XCHG_WHEN_UNMARKED(list, old_list_ptr, new_list_ptr);
    pthread_mutex_unlock(&list->mutex); /* Call already succeeded:
					 * ignore return value */
    free(old_list_ptr);

    return ret;
}

int atomic_list_unshift(atomic_list_t *list, void *item) {
    struct atomic_list *old_list_ptr;
    struct atomic_list *new_list_ptr;
    /* Acquire write lock */
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return r;
    }
    /* Get current list_ptr */
    old_list_ptr = atomic_ptr_read(&list->list_ptr);
    /* allocate new list and copy data */
    new_list_ptr = malloc(ALST_SIZE(ALST_UNMARK(old_list_ptr)->length + 1));
    if(new_list_ptr == NULL) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return r;
	}
	return -1;
    }
    memcpy(new_list_ptr->list + 1, ALST_UNMARK(old_list_ptr)->list, sizeof(void *) * ALST_UNMARK(old_list_ptr)->length);
    new_list_ptr->list[0] = item;
    new_list_ptr->length = ALST_UNMARK(old_list_ptr)->length + 1;
    /* Exchange new and old lists and clean up */
    XCHG_WHEN_UNMARKED(list, old_list_ptr, new_list_ptr);
    pthread_mutex_unlock(&list->mutex); /* Call already succeeded:
					 * ignore return value */
    free(old_list_ptr);

    return 0;
}

void *atomic_list_shift(atomic_list_t *list) {
    struct atomic_list *old_list_ptr;
    struct atomic_list *new_list_ptr;
    /* Acquire write lock */
    void *ret;
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return ALST_ERROR;
    }
    /* Get current list_ptr */
    old_list_ptr = atomic_ptr_read(&list->list_ptr);
    if(ALST_UNMARK(old_list_ptr)->length == 0) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return ALST_ERROR;
	}
	return ALST_EMPTY;
    }
    /* allocate new list and copy data */
    new_list_ptr = malloc(ALST_SIZE(ALST_UNMARK(old_list_ptr)->length - 1));
    if(new_list_ptr == NULL) {
	r = pthread_mutex_unlock(&list->mutex);
	return ALST_ERROR;
    }
    memcpy(new_list_ptr->list, ALST_UNMARK(old_list_ptr)->list + 1, sizeof(void *) * (ALST_UNMARK(old_list_ptr)->length - 1));
    ret = ALST_UNMARK(old_list_ptr)->list[0];
    new_list_ptr->length = ALST_UNMARK(old_list_ptr)->length - 1;
    /* Exchange new and old lists and clean up */
    XCHG_WHEN_UNMARKED(list, old_list_ptr, new_list_ptr);
    pthread_mutex_unlock(&list->mutex); /* Call already succeeded:
					 * ignore return value */
    free(old_list_ptr);

    return ret;
}

int atomic_list_insert(atomic_list_t *list, off_t index, void *item) {
    struct atomic_list *old_list_ptr;
    struct atomic_list *new_list_ptr;
    /* Acquire write lock */
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return r;
    }
    /* Get current list_ptr */
    old_list_ptr = atomic_ptr_read(&list->list_ptr);
    /* Check bounds */
    if(ALST_OUT_OF_BOUNDS(ALST_UNMARK(old_list_ptr)->length + 1, index)) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return r;
	}
	errno = EFAULT;
	return -1;
    }
    /* allocate new list and copy data */
    new_list_ptr = malloc(ALST_SIZE(ALST_UNMARK(old_list_ptr)->length + 1));
    if(new_list_ptr == NULL) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return r;
	}
	return -1;
    }
    memcpy(new_list_ptr, ALST_UNMARK(old_list_ptr), ALST_SIZE(index));
    memcpy(new_list_ptr->list + index + 1, ALST_UNMARK(old_list_ptr)->list + index, sizeof(void *) * (ALST_UNMARK(old_list_ptr)->length - index));
    new_list_ptr->list[index] = item;
    new_list_ptr->length++;
    /* Exchange new and old lists and clean up */
    XCHG_WHEN_UNMARKED(list, old_list_ptr, new_list_ptr);
    pthread_mutex_unlock(&list->mutex); /* Call already succeeded:
					 * ignore return value */
    free(old_list_ptr);

    return 0;
}

void *atomic_list_remove(atomic_list_t *list, off_t index) {
    struct atomic_list *old_list_ptr;
    struct atomic_list *new_list_ptr;
    /* Acquire write lock */
    void *ret;
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return ALST_ERROR;
    }
    /* Get current list_ptr */
    old_list_ptr = atomic_ptr_read(&list->list_ptr);
    /* Check bounds */
    if(ALST_OUT_OF_BOUNDS(ALST_UNMARK(old_list_ptr)->length, index)) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return ALST_ERROR;
	}
	errno = EFAULT;
	return ALST_ERROR;
    }
    /* allocate new list and copy data */
    new_list_ptr = malloc(ALST_SIZE(ALST_UNMARK(old_list_ptr)->length - 1));
    if(new_list_ptr == NULL) {
	pthread_mutex_unlock(&list->mutex);
	return NULL;
    }
    memcpy(new_list_ptr, ALST_UNMARK(old_list_ptr), ALST_SIZE(index));
    memcpy(new_list_ptr->list + index, ALST_UNMARK(old_list_ptr)->list + index + 1, sizeof(void *) * (ALST_UNMARK(old_list_ptr)->length - index));
    ret = ALST_UNMARK(old_list_ptr)->list[index];
    new_list_ptr->length--;
    /* Exchange new and old lists and clean up */
    XCHG_WHEN_UNMARKED(list, old_list_ptr, new_list_ptr);
    pthread_mutex_unlock(&list->mutex); /* Call already succeeded:
					 * ignore return value */
    free(old_list_ptr);

    return ret;
}

int atomic_list_remove_by_value(atomic_list_t *list, void *value) {
    struct atomic_list *old_list_ptr;
    struct atomic_list *new_list_ptr;
    /* Acquire write lock */
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return -1;
    }
    /* Get current list_ptr */
    old_list_ptr = atomic_ptr_read(&list->list_ptr);
    if(ALST_UNMARK(old_list_ptr)->length == 0) {
	return 0;
    }
    /* count the occurrences of value in list */
    size_t count = 0;
    void **ary = ALST_UNMARK(old_list_ptr)->list;
    size_t length = ALST_UNMARK(old_list_ptr)->length;
    size_t i;
    for(i = 0; i < length; i++) {
	if(ary[i] == value) {
	    count++;
	}
    }
    if(count == 0) {
	return 0;
    }
    /* allocate new list and copy data */
    new_list_ptr = malloc(ALST_SIZE(ALST_UNMARK(old_list_ptr)->length - count));
    if(new_list_ptr == NULL) {
	pthread_mutex_unlock(&list->mutex);
	return -1;
    }
    size_t j = 0;
    void **newary = new_list_ptr->list;
    for(i = 0; i < length; i++) {
	if(ary[i] != value) {
	    newary[j++] = ary[i];
	}
    }
    new_list_ptr->length = length - count;
    /* Exchange new and old lists and clean up */
    XCHG_WHEN_UNMARKED(list, old_list_ptr, new_list_ptr);
    pthread_mutex_unlock(&list->mutex); /* Call already succeeded:
					 * ignore return value */
    free(old_list_ptr);

    return 0;
}

int atomic_list_clear(atomic_list_t *list) {
    struct atomic_list *old_list_ptr;
    struct atomic_list *new_list_ptr;
    /* Acquire write lock */
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return r;
    }
    /* Get current list_ptr */
    old_list_ptr = atomic_ptr_read(&list->list_ptr);
    /* allocate new list and copy data */
    new_list_ptr = malloc(ALST_SIZE(0));
    if(new_list_ptr == NULL) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return r;
	}
	return -1;
    }
    new_list_ptr->length = 0;
    /* Exchange new and old lists and clean up */
    XCHG_WHEN_UNMARKED(list, old_list_ptr, new_list_ptr);
    pthread_mutex_unlock(&list->mutex); /* Call already succeeded:
					 * ignore return value */
    free(old_list_ptr);

    return 0;
}

int atomic_list_init(atomic_list_t *list) {
    struct atomic_list *list_ptr;
    int r;
    list_ptr = malloc(ALST_SIZE(0));
    if(list_ptr == NULL) {
	return -1;
    }
    atomic_ptr_set(&list->list_ptr, list_ptr);

    r = pthread_mutex_init(&list->mutex, NULL);
    if(r != 0) {
	return r;
    }

    return 0;
}

int atomic_list_destroy(atomic_list_t *list) {
    struct atomic_list *list_ptr;
    int r;
    r = pthread_mutex_destroy(&list->mutex);
    if(r != 0) {
	return r;
    }

    list_ptr = ALST_UNMARK(atomic_ptr_xchg(&list->list_ptr, NULL));
    if(list_ptr != NULL) {
	free(list_ptr);
    }
    
    return 0;
}

void *atomic_list_get(atomic_list_t *list, off_t index) {
    struct atomic_list *list_ptr;
    void *ret;
    /* Acquire write lock to prevent writing */
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return ALST_ERROR;
    }

    list_ptr = ALST_UNMARK((struct atomic_list *) atomic_ptr_read(&list->list_ptr));
    if(ALST_OUT_OF_BOUNDS(list_ptr->length, index)) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return ALST_ERROR;
	}
	errno = EFAULT;
	return ALST_ERROR;
    }

    ret = list_ptr->list[index];

    r = pthread_mutex_unlock(&list->mutex);
    if(r != 0) {
	return ALST_ERROR;
    }

    return ret;
}

void *atomic_list_first(atomic_list_t *list) {
    struct atomic_list *list_ptr;
    void *ret;
    /* Acquire write lock to prevent writing */
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return ALST_ERROR;
    }

    list_ptr = ALST_UNMARK((struct atomic_list *) atomic_ptr_read(&list->list_ptr));
    if(list_ptr->length == 0) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return ALST_ERROR;
	}
	return ALST_EMPTY;
    } else {
	ret = list_ptr->list[0];
    }

    r = pthread_mutex_unlock(&list->mutex);
    if(r != 0) {
	return ALST_ERROR;
    }

    return ret;
}

void *atomic_list_last(atomic_list_t *list) {
    struct atomic_list *list_ptr;
    void *ret;
    /* Acquire write lock to prevent writing */
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return ALST_ERROR;
    }

    list_ptr = ALST_UNMARK((struct atomic_list *) atomic_ptr_read(&list->list_ptr));
    if(list_ptr->length == 0) {
	r = pthread_mutex_unlock(&list->mutex);
	if(r != 0) {
	    return ALST_ERROR;
	}
	return ALST_EMPTY;
    } else {
	ret = list_ptr->list[list_ptr->length - 1];
    }

    r = pthread_mutex_unlock(&list->mutex);
    if(r != 0) {
	return ALST_ERROR;
    }

    return ret;
}

void **atomic_list_ary(atomic_list_t *list) {
    struct atomic_list *list_ptr;
    void *ret;
    /* Acquire write lock to prevent writing */
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return ALST_ERROR;
    }

    list_ptr = ALST_UNMARK((struct atomic_list *) atomic_ptr_read(&list->list_ptr));
    ret = list_ptr->list;

    r = pthread_mutex_unlock(&list->mutex);
    if(r != 0) {
	return ALST_ERROR;
    }

    return ret;
}

size_t atomic_list_length(atomic_list_t *list) {
    struct atomic_list *list_ptr;
    size_t ret;
    /* Acquire write lock to prevent writing */
    int r;
    r = pthread_mutex_lock(&list->mutex);
    if(r != 0) {
	return -1;
    }

    list_ptr = ALST_UNMARK((struct atomic_list *) atomic_ptr_read(&list->list_ptr));
    ret = list_ptr->length;

    r = pthread_mutex_unlock(&list->mutex);
    if(r != 0) {
	return -1;
    }

    return ret;
}
