/*
 * array.c
 *
 * Copyright 2013 Evan Buswell
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
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "atomickit/rcp.h"
#include "atomickit/array.h"
#include "atomickit/malloc.h"

static void aary_destroy(struct aary *array) {
	size_t i;
	for(i = 0; i < array->len; i++) {
		arcp_release(array->items[i]);
	}
	afree(array, AARY_SIZE(array->len));
}

struct aary *aary_dup(struct aary *array) {
	struct aary *ret;
	size_t i;
	ret = amalloc(AARY_SIZE(array->len));
	if(ret == NULL) {
		return NULL;
	}
	for(i = 0; i < array->len; i++) {
		ret->items[i] = arcp_acquire(array->items[i]);
	}
	ret->len = array->len;
	arcp_region_init(ret, (arcp_destroy_f) aary_destroy);
	return ret;
}

struct aary *aary_create(size_t len) {
	struct aary *ret = amalloc(AARY_SIZE(len));
	if(ret == NULL) {
		return NULL;
	}
	memset(ret->items, 0, sizeof(struct arcp_region *) * len);
	ret->len = len;
	arcp_region_init(ret, (arcp_destroy_f) aary_destroy);
	return ret;
}

#define IF_BSEARCH(list, nitems, value, retidx) do {	\
	size_t __l;					\
	size_t __u;					\
	(retidx) = __l = (0);				\
	__u = (nitems);					\
	while(__l < __u) {				\
		struct arcp_region *__v;		\
		(retidx) = (__l + __u) / 2;		\
		__v = (list)[retidx];			\
		if((value) < __v) {			\
			__u = (retidx);			\
		} else if((value) > __v) {		\
			__l = ++(retidx);		\
		} else

#define ENDIF_BSEARCH	\
	}		\
} while(0)

struct aary *aary_insert(struct aary *array, size_t i,
                         struct arcp_region *region) {
	size_t len = array->len;
	if(atryrealloc(array, AARY_SIZE(len), AARY_SIZE(len + 1))) {
		memmove(&array->items[i + 1], &array->items[i],
		        sizeof(struct arcp_region *) * (len - i));
	} else {
		struct aary *new_array = amalloc(AARY_SIZE(len + 1));
		if(new_array == NULL) {
			return NULL;
		}
		memcpy(new_array, array, AARY_SIZE(i));
		memcpy(&new_array->items[i + 1], &array->items[i],
		       sizeof(struct arcp_region *) * (len - i));
		afree(array, AARY_SIZE(len));
		array = new_array;
	}
	array->len = len + 1;
	array->items[i] = arcp_acquire(region);
	return array;
}

struct aary *aary_dup_insert(struct aary *array, size_t i,
                             struct arcp_region *region) {
	size_t len;
	struct aary *new_array;
	size_t j;
	len = array->len;
	new_array = amalloc(AARY_SIZE(len + 1));
	if(new_array == NULL) {
		return NULL;
	}
	for(j = 0; j < i; j++) {
		new_array->items[j] = arcp_acquire(array->items[j]);
	}
	for(; j < len; j++) {
		new_array->items[j + 1] = arcp_acquire(array->items[j]);
	}
	new_array->len = len + 1;
	new_array->items[i] = arcp_acquire(region);
	arcp_region_init(new_array, (arcp_destroy_f) aary_destroy);
	return new_array;
}

struct aary *aary_remove(struct aary *array, size_t i) {
	size_t len;
	struct arcp_region *deleted;
	struct arcp_region *last;
	len = array->len;
	deleted = array->items[i];
	last = array->items[len - 1];
	if(atryrealloc(array, AARY_SIZE(len), AARY_SIZE(len - 1))) {
		if(deleted != last) {
			memmove(&array->items[i], &array->items[i + 1],
		                sizeof(struct arcp_region *) * ((len - 1)
			                                        - (i + 1)));
			array->items[len - 2] = last;
		}
	} else {
		struct aary *new_array;
		new_array = amalloc(AARY_SIZE(len - 1));
		if(new_array == NULL) {
			return NULL;
		}
		memcpy(new_array, array, AARY_SIZE(i));
		memcpy(&new_array->items[i], &array->items[i + 1],
		       sizeof(struct arcp_region *) * (len - (i + 1)));
		afree(array, AARY_SIZE(len));
		array = new_array;
	}
	array->len = len - 1;
	arcp_release(deleted);
	return array;
}

struct aary *aary_dup_remove(struct aary *array, size_t i) {
	size_t len;
	size_t j;
	struct aary *new_array;
	len = array->len;
	new_array = amalloc(AARY_SIZE(len - 1));
	if(new_array == NULL) {
		return NULL;
	}
	for(j = 0; j < i; j++) {
		new_array->items[j] = arcp_acquire(array->items[j]);
	}
	for(; j + 1 < len; j++) {
		new_array->items[j] = arcp_acquire(array->items[j + 1]);
	}
	new_array->len = len - 1;
	arcp_region_init(new_array, (arcp_destroy_f) aary_destroy);
	return new_array;
}

struct aary *aary_append(struct aary *array, struct arcp_region *region) {
	size_t len;
	len = array->len;
	array = arealloc(array, AARY_SIZE(len), AARY_SIZE(len + 1));
	if(array == NULL) {
		return NULL;
	}
	array->len = len + 1;
	array->items[len] = arcp_acquire(region);
	return array;
}

struct aary *aary_dup_append(struct aary *array, struct arcp_region *region) {
	size_t len;
	struct aary *new_array;
	size_t j;
	len = array->len;
	new_array = amalloc(AARY_SIZE(len + 1));
	if(new_array == NULL) {
		return NULL;
	}
	for(j = 0; j < len; j++) {
		new_array->items[j] = arcp_acquire(array->items[j]);
	}
	new_array->len = len + 1;
	new_array->items[len] = arcp_acquire(region);
	arcp_region_init(new_array, (arcp_destroy_f) aary_destroy);
	return new_array;
}

struct aary *aary_pop(struct aary *array) {
	size_t len;
	struct arcp_region *region;
	len = array->len;
	region = array->items[len - 1];
	array = arealloc(array, AARY_SIZE(len), AARY_SIZE(len - 1));
	if(array == NULL) {
		return NULL;
	}
	array->len = len - 1;
	arcp_release(region);
	return array;
}

struct aary *aary_dup_pop(struct aary *array) {
	size_t len;
	struct aary *new_array;
	size_t j;
	len = array->len;
	new_array = amalloc(AARY_SIZE(len - 1));
	if(new_array == NULL) {
		return NULL;
	}
	for(j = 0; j < len - 1; j++) {
		new_array->items[j] = arcp_acquire(array->items[j]);
	}
	new_array->len = len - 1;
	arcp_region_init(new_array, (arcp_destroy_f) aary_destroy);
	return new_array;
}

struct aary *aary_prepend(struct aary *array, struct arcp_region *region) {
	size_t len;
	len = array->len;
	if(atryrealloc(array, AARY_SIZE(len), AARY_SIZE(len + 1))) {
		memmove(&array->items[1], &array->items[0],
		        sizeof(struct arcp_region *) * len);
	} else {
		struct aary *new_array;
		new_array = amalloc(AARY_SIZE(len + 1));
		if(new_array == NULL) {
			return NULL;
		}
		memcpy(new_array, array, AARY_OVERHEAD);
		memcpy(&new_array->items[1], &array->items[0],
		       sizeof(struct arcp_region *) * len);
		afree(array, AARY_SIZE(len));
		array = new_array;
	}
	array->len = len + 1;
	array->items[0] = arcp_acquire(region);
	return array;
}

struct aary *aary_dup_prepend(struct aary *array,
                              struct arcp_region *region) {
	size_t len;
	struct aary *new_array;
	size_t j;
	len = array->len;
	new_array = amalloc(AARY_SIZE(len + 1));
	if(new_array == NULL) {
		return NULL;
	}
	for(j = 0; j < len; j++) {
		new_array->items[j + 1] = arcp_acquire(array->items[j]);
	}
	new_array->len = len + 1;
	new_array->items[0] = arcp_acquire(region);
	arcp_region_init(new_array, (arcp_destroy_f) aary_destroy);
	return new_array;
}

struct aary *aary_shift(struct aary *array) {
	size_t len;
	struct arcp_region *region;
	struct arcp_region *last;
	len = array->len;
	region = array->items[0];
	last = array->items[len - 1];
	if(atryrealloc(array, AARY_SIZE(len), AARY_SIZE(len - 1))) {
		memmove(&array->items[0], &array->items[1],
		        sizeof(struct arcp_region *) * ((len - 1) - 1));
		array->items[len - 2] = last;
	} else {
		struct aary *new_array;
		new_array = amalloc(AARY_SIZE(len - 1));
		if(new_array == NULL) {
			return NULL;
		}
		memcpy(new_array, array, AARY_OVERHEAD);
		memcpy(&new_array->items[0], &array->items[1],
		       sizeof(struct arcp_region *) * (len - 1));
		afree(array, AARY_SIZE(len));
		array = new_array;
	}
	array->len = len - 1;
	arcp_release(region);
	return array;
}

struct aary *aary_dup_shift(struct aary *array) {
	size_t len;
	struct aary *new_array;
	size_t j;
	len = array->len;
	new_array = amalloc(AARY_SIZE(len - 1));
	if(new_array == NULL) {
		return NULL;
	}
	for(j = 1; j < len; j++) {
		new_array->items[j - 1] = arcp_acquire(array->items[j]);
	}
	new_array->len = len - 1;
	arcp_region_init(new_array, (arcp_destroy_f) aary_destroy);
	return new_array;
}

void aary_sortx(struct aary *array) {
	int qsort_compar(const struct arcp_region **region1,
	                 const struct arcp_region **region2) {
		if(*region1 < *region2) {
			return -1;
		} else if(*region1 == *region2) {
			return 0;
		} else {
			return 1;
		}
	}
	qsort(array->items, array->len, sizeof(struct arcp_region *),
	      (int (*)(const void *, const void *)) qsort_compar);
}

void aary_sort(struct aary *array, int (*compar)(const struct arcp_region *,
               const struct arcp_region *)) {
	int qsort_compar(const struct arcp_region **region1,
	                 const struct arcp_region **region2) {
		return compar(*region1, *region2);
	}

	qsort(array->items, array->len, sizeof(struct arcp_region *),
	      (int (*)(const void *, const void *)) qsort_compar);
}

void aary_sort_r(struct aary *array, int (*compar)(const struct arcp_region *,
                 const struct arcp_region *, void *arg), void *arg) {
	int qsort_compar(const struct arcp_region **region1,
	                 const struct arcp_region **region2) {
		return compar(*region1, *region2, arg);
	}

	qsort(array->items, array->len, sizeof(struct arcp_region *),
	      (int (*)(const void *, const void *)) qsort_compar);
}

void aary_reverse(struct aary *array) {
	size_t i, j;
	for(i = 0, j = array->len - 1; i < j; i++, j--) {
		struct arcp_region *tmp;
		tmp = array->items[i];
		array->items[i] = array->items[j];
		array->items[j] = tmp;
	}
}

/* TODO: overwriteary, insertary, removeary, overwriteendary, concat, popary, overwritestartary, concat, shiftary */

struct aary *aary_set_add(struct aary *array, struct arcp_region *region) {
	/* search for region */
	size_t i;
	size_t len;
	len = array->len;
	IF_BSEARCH(array->items, len, region, i) {
		/* Already present; do nothing */
		return array;
	} ENDIF_BSEARCH;
	return aary_insert(array, i, region);
}

struct aary *aary_dup_set_add(struct aary *array,
                              struct arcp_region *region) {
	/* search for region */
	size_t i;
	size_t len;
	len = array->len;
	IF_BSEARCH(array->items, len, region, i) {
		/* Already present; do nothing */
		return aary_dup(array);
	} ENDIF_BSEARCH;
	return aary_dup_insert(array, i, region);
}

struct aary *aary_set_remove(struct aary *array,
                             struct arcp_region *region) {
	/* search for region */
	size_t i;
	size_t len;
	len = array->len;
	IF_BSEARCH(array->items, len, region, i) {
		/* Found it */
		return aary_remove(array, i);
	} ENDIF_BSEARCH;
	/* couldn't find it... */
	return array;
}

struct aary *aary_dup_set_remove(struct aary *array,
                                 struct arcp_region *region) {
	/* search for region */
	size_t i;
	size_t len;
	len = array->len;
	IF_BSEARCH(array->items, len, region, i) {
		/* Found it */
		return aary_dup_remove(array, i);
	} ENDIF_BSEARCH;
	/* couldn't find it... */
	return aary_dup(array);
}

bool aary_set_contains(struct aary *array,
                       struct arcp_region *region) {
	size_t i;
	IF_BSEARCH(array->items, array->len, region, i) {
		return true;
	} ENDIF_BSEARCH;
	return false;
}

/* TODO: union (|), intersection (&), difference (\), disjunction (^), equal, disjoint, subset, proper_subset, superset, proper_superset */
