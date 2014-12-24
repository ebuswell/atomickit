/*
 * array.c
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
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "atomickit/rcp.h"
#include "atomickit/array.h"
#include "atomickit/malloc.h"

static void aary_destroy(struct aary *array) {
	size_t i;
	/* release each value */
	for (i = 0; i < array->len; i++) {
		arcp_release(array->items[i]);
	}
	/* free the memory of the array itself */
	afree(array, AARY_SIZE(array->len));
}

struct aary *aary_dup(struct aary *array) {
	struct aary *ret;
	size_t i;
	/* allocate the memory required to duplicate the array */
	ret = amalloc(AARY_SIZE(array->len));
	if (ret == NULL) {
		/* allocation failure */
		return NULL;
	}
	/* acquire a reference to each item in the old array and copy it
	 * over */
	for (i = 0; i < array->len; i++) {
		ret->items[i] = arcp_acquire(array->items[i]);
	}
	/* set up the array */
	ret->len = array->len;
	arcp_region_init(ret, (arcp_destroy_f) aary_destroy);
	return ret;
}

struct aary *aary_create(size_t len) {
	struct aary *ret;
	/* allocate the memory for the array */
	ret = amalloc(AARY_SIZE(len));
	if (ret == NULL) {
		/* allocation failure */
		return NULL;
	}
	/* set the array values to null */
	memset(ret->items, 0, sizeof(struct arcp_region *) * len);
	/* set up the array */
	ret->len = len;
	arcp_region_init(ret, (arcp_destroy_f) aary_destroy);
	return ret;
}


struct aary *aary_insert(struct aary *array,
			 size_t i, struct arcp_region *region) {
	size_t len;
	len = array->len;
	/* reallocate the array, either in place or by copying stuff over */
	if (atryrealloc(array, AARY_SIZE(len), AARY_SIZE(len + 1))) {
		memmove(&array->items[i + 1], &array->items[i],
			sizeof(struct arcp_region *) * (len - i));
	} else {
		struct aary *new_array;
		new_array = amalloc(AARY_SIZE(len + 1));
		if (new_array == NULL) {
			/* failed to reallocate array */
			return NULL;
		}
		memcpy(new_array, array, AARY_SIZE(i));
		memcpy(&new_array->items[i + 1], &array->items[i],
		       sizeof(struct arcp_region *) * (len - i));
		afree(array, AARY_SIZE(len));
		array = new_array;
	}
	/* add inserted item */
	array->items[i] = arcp_acquire(region);
	/* set up array */
	array->len = len + 1;
	return array;
}

struct aary *aary_dup_insert(struct aary *array,
			     size_t i, struct arcp_region *region) {
	size_t len;
	size_t j;
	struct aary *new_array;
	len = array->len;
	/* allocate space for the new array */
	new_array = amalloc(AARY_SIZE(len + 1));
	if (new_array == NULL) {
		return NULL;
	}
	/* acquire items from the old array and copy them over, leaving a hole
	 * for the new value. */
	for (j = 0; j < i; j++) {
		new_array->items[j] = arcp_acquire(array->items[j]);
	}
	for (; j < len; j++) {
		new_array->items[j + 1] = arcp_acquire(array->items[j]);
	}
	/* add inserted item */
	new_array->items[i] = arcp_acquire(region);
	/* set up array */
	new_array->len = len + 1;
	arcp_region_init(new_array, (arcp_destroy_f) aary_destroy);
	return new_array;
}

struct aary *aary_remove(struct aary *array, size_t i) {
	size_t len;
	struct arcp_region *deleted;
	struct arcp_region *last;
	len = array->len;
	/* store the deleted item so we can release it only after we are sure
	 * the reallocation happened. */
	deleted = array->items[i];
	/* store the last item bc realloc will always make it unavailable to
	 * move */
	last = array->items[len - 1];
	/* reallocate the array, either in place or by copying stuff over */
	if (atryrealloc(array, AARY_SIZE(len), AARY_SIZE(len - 1))) {
		if (deleted != last) {
			memmove(&array->items[i], &array->items[i + 1],
				sizeof(struct arcp_region *) * ((len - 1)
								- (i + 1)));
			array->items[len - 2] = last;
		}
	} else {
		struct aary *new_array;
		new_array = amalloc(AARY_SIZE(len - 1));
		if (new_array == NULL) {
			return NULL;
		}
		memcpy(new_array, array, AARY_SIZE(i));
		memcpy(&new_array->items[i], &array->items[i + 1],
		       sizeof(struct arcp_region *) * (len - (i + 1)));
		afree(array, AARY_SIZE(len));
		array = new_array;
	}
	/* set up array */
	array->len = len - 1;
	/* release removed value */
	arcp_release(deleted);
	return array;
}

struct aary *aary_dup_remove(struct aary *array, size_t i) {
	size_t len;
	size_t j;
	struct aary *new_array;
	len = array->len;
	/* allocate space for the new array */
	new_array = amalloc(AARY_SIZE(len - 1));
	if (new_array == NULL) {
		return NULL;
	}
	/* acquire items from the old array and copy them over, skipping the
	 * removed value. */
	for (j = 0; j < i; j++) {
		new_array->items[j] = arcp_acquire(array->items[j]);
	}
	for (; j + 1 < len; j++) {
		new_array->items[j] = arcp_acquire(array->items[j + 1]);
	}
	/* set up array */
	new_array->len = len - 1;
	arcp_region_init(new_array, (arcp_destroy_f) aary_destroy);
	return new_array;
}

struct aary *aary_append(struct aary *array, struct arcp_region *region) {
	size_t len;
	len = array->len;
	/* reallocate the array */
	array = arealloc(array, AARY_SIZE(len), AARY_SIZE(len + 1));
	if (array == NULL) {
		return NULL;
	}
	/* append the last value */
	array->items[len] = arcp_acquire(region);
	/* set up array */
	array->len = len + 1;
	return array;
}

struct aary *aary_dup_append(struct aary *array,
			     struct arcp_region *region) {
	size_t len;
	size_t j;
	struct aary *new_array;
	len = array->len;
	/* allocate space for the new array */
	new_array = amalloc(AARY_SIZE(len + 1));
	if (new_array == NULL) {
		return NULL;
	}
	/* acquire a reference to each item in the old array and copy it
	 * over */
	for (j = 0; j < len; j++) {
		new_array->items[j] = arcp_acquire(array->items[j]);
	}
	/* append the last value */
	new_array->items[len] = arcp_acquire(region);
	/* set up array */
	new_array->len = len + 1;
	arcp_region_init(new_array, (arcp_destroy_f) aary_destroy);
	return new_array;
}

struct aary *aary_pop(struct aary *array) {
	size_t len;
	struct arcp_region *region;
	len = array->len;
	/* store removed region so we can release our reference only when
	 * we've succeeded */
	region = array->items[len - 1];
	/* reallocate the array */
	array = arealloc(array, AARY_SIZE(len), AARY_SIZE(len - 1));
	if (array == NULL) {
		return NULL;
	}
	/* set up array */
	array->len = len - 1;
	/* release removed value */
	arcp_release(region);
	return array;
}

struct aary *aary_dup_pop(struct aary *array) {
	size_t len;
	size_t j;
	struct aary *new_array;
	len = array->len;
	/* allocate space for the new array */
	new_array = amalloc(AARY_SIZE(len - 1));
	if (new_array == NULL) {
		return NULL;
	}
	/* acquire a reference to each item in the old array and copy it
	 * over */
	for (j = 0; j < len - 1; j++) {
		new_array->items[j] = arcp_acquire(array->items[j]);
	}
	/* set up array */
	new_array->len = len - 1;
	arcp_region_init(new_array, (arcp_destroy_f) aary_destroy);
	return new_array;
}

struct aary *aary_prepend(struct aary *array, struct arcp_region *region) {
	size_t len;
	struct aary *new_array;
	len = array->len;
	/* reallocate the array, either in place or by copying stuff over */
	if (atryrealloc(array, AARY_SIZE(len), AARY_SIZE(len + 1))) {
		memmove(&array->items[1], &array->items[0],
			sizeof(struct arcp_region *) * len);
	} else {
		new_array = amalloc(AARY_SIZE(len + 1));
		if (new_array == NULL) {
			return NULL;
		}
		memcpy(new_array, array, AARY_OVERHEAD);
		memcpy(&new_array->items[1], &array->items[0],
		       sizeof(struct arcp_region *) * len);
		afree(array, AARY_SIZE(len));
		array = new_array;
	}
	/* prepend new item */
	array->items[0] = arcp_acquire(region);
	/* set up array */
	array->len = len + 1;
	return array;
}

struct aary *aary_dup_prepend(struct aary *array,
			      struct arcp_region *region) {
	size_t len;
	size_t j;
	struct aary *new_array;
	len = array->len;
	/* allocate space for the new array */
	new_array = amalloc(AARY_SIZE(len + 1));
	if (new_array == NULL) {
		return NULL;
	}
	/* acquire items from the old array and copy them over */
	for (j = 0; j < len; j++) {
		new_array->items[j + 1] = arcp_acquire(array->items[j]);
	}
	/* prepend new item */
	new_array->items[0] = arcp_acquire(region);
	/* set up array */
	new_array->len = len + 1;
	arcp_region_init(new_array, (arcp_destroy_f) aary_destroy);
	return new_array;
}

struct aary *aary_shift(struct aary *array) {
	size_t len;
	struct arcp_region *region;
	struct arcp_region *last;
	struct aary *new_array;
	len = array->len;
	/* store removed region so we can release our reference only once
	 * we've succeeded */
	region = array->items[0];
	/* store the last item bc realloc will always make it unavailable to
	 * move */
	last = array->items[len - 1];
	/* reallocate the array, either in place or by copying stuff over */
	if (atryrealloc(array, AARY_SIZE(len), AARY_SIZE(len - 1))) {
		memmove(&array->items[0], &array->items[1],
			sizeof(struct arcp_region *) * ((len - 1) - 1));
		array->items[len - 2] = last;
	} else {
		new_array = amalloc(AARY_SIZE(len - 1));
		if (new_array == NULL) {
			return NULL;
		}
		memcpy(new_array, array, AARY_OVERHEAD);
		memcpy(&new_array->items[0], &array->items[1],
		       sizeof(struct arcp_region *) * (len - 1));
		afree(array, AARY_SIZE(len));
		array = new_array;
	}
	/* set up array */
	array->len = len - 1;
	/* release removed region */
	arcp_release(region);
	return array;
}

struct aary *aary_dup_shift(struct aary *array) {
	size_t len;
	size_t j;
	struct aary *new_array;
	len = array->len;
	/* allocate space for the new array */
	new_array = amalloc(AARY_SIZE(len - 1));
	if (new_array == NULL) {
		return NULL;
	}
	/* acquire items from the old array and copy them over */
	for (j = 1; j < len; j++) {
		new_array->items[j - 1] = arcp_acquire(array->items[j]);
	}
	/* set up array */
	new_array->len = len - 1;
	arcp_region_init(new_array, (arcp_destroy_f) aary_destroy);
	return new_array;
}

bool aary_equal(struct aary *array1, struct aary *array2) {
	size_t i;

	/* not equal if the lengths aren't equal */
	if (array1->len != array2->len) {
		return false;
	}

	/* not equal if any of the values aren't equal */
	for (i = 0; i < array1->len; i++) {
		if (array1->items[i] != array1->items[i]) {
			return false;
		}
	}
	/* otherwise equal */
	return true;
}

void aary_sortx(struct aary *array) {
	/* use the GNU C nested function extension */
	int qsort_compar(const struct arcp_region **region1,
			 const struct arcp_region **region2) {
		return (int) ((intptr_t) *region1 - (intptr_t) *region2);
	}
	qsort(array->items, array->len, sizeof(struct arcp_region *),
	      (int (*)(const void *, const void *)) qsort_compar);
}

void aary_sort(struct aary *array,
	       int (*compar)(const struct arcp_region *,
			     const struct arcp_region *)) {
	/* use the GNU C nested function extension */
	int qsort_compar(const struct arcp_region **region1,
			 const struct arcp_region **region2) {
		return compar(*region1, *region2);
	}

	qsort(array->items, array->len, sizeof(struct arcp_region *),
	      (int (*)(const void *, const void *)) qsort_compar);
}

void aary_sort_r(struct aary *array,
		 int (*compar)(const struct arcp_region *,
			       const struct arcp_region *,
			       void *arg),
		 void *arg) {
	/* use the GNU C nested function extension */
	int qsort_compar(const struct arcp_region **region1,
			 const struct arcp_region **region2) {
		return compar(*region1, *region2, arg);
	}

	qsort(array->items, array->len, sizeof(struct arcp_region *),
	      (int (*)(const void *, const void *)) qsort_compar);
}

void aary_reverse(struct aary *array) {
	size_t i, j;
	for (i = 0, j = array->len - 1; i < j; i++, j--) {
		struct arcp_region *tmp;
		tmp = array->items[i];
		array->items[i] = array->items[j];
		array->items[j] = tmp;
	}
}

/*****************
 * Set Functions *
 *****************/

/* IF_BSEARCH and ENDIF_BSEARCH macros for internal use.
 * list - the array in which to search
 * nitems - the number of items in the list
 * value - the value to search for
 * retidx - this will be set to the index at which the value was found, or if
 * the value was not found, the place at which the value should be put to keep
 * the array sorted */
#define IF_BSEARCH(list, nitems, value, retidx) do {			\
	size_t __l;			/* lower limit */		\
	size_t __u;			/* upper limit */		\
	struct arcp_region *__v;					\
	(retidx) = __l = (0);						\
	__u = (nitems);							\
	while (__l < __u) {						\
		(retidx) = (__l + __u) / 2;				\
		__v = (list)[retidx];					\
		if ((value) < __v) {					\
			__u = (retidx);					\
		} else if ((value) > __v) {				\
			__l = ++(retidx);				\
		} else

#define ENDIF_BSEARCH							\
	}								\
} while (0)


struct aary *aary_set_add(struct aary *array, struct arcp_region *region) {
	size_t i;
	size_t len;
	len = array->len;
	/* search for region */
	IF_BSEARCH(array->items, len, region, i) {
		/* Already present; do nothing */
		return array;
	} ENDIF_BSEARCH;
	return aary_insert(array, i, region);
}

struct aary *aary_dup_set_add(struct aary *array,
			      struct arcp_region *region) {
	size_t i;
	size_t len;
	len = array->len;
	/* search for region */
	IF_BSEARCH(array->items, len, region, i) {
		/* Already present; do nothing */
		return aary_dup(array);
	} ENDIF_BSEARCH;
	return aary_dup_insert(array, i, region);
}

struct aary *aary_set_remove(struct aary *array,
			     struct arcp_region *region) {
	size_t i;
	size_t len;
	len = array->len;
	/* search for region */
	IF_BSEARCH(array->items, len, region, i) {
		/* Found it */
		return aary_remove(array, i);
	} ENDIF_BSEARCH;
	/* couldn't find it... */
	return array;
}

struct aary *aary_dup_set_remove(struct aary *array,
				 struct arcp_region *region) {
	size_t i;
	size_t len;
	len = array->len;
	/* search for region */
	IF_BSEARCH(array->items, len, region, i) {
		/* Found it */
		return aary_dup_remove(array, i);
	} ENDIF_BSEARCH;
	/* couldn't find it... */
	return aary_dup(array);
}

bool aary_set_contains(struct aary *array, struct arcp_region *region) {
	size_t i;
	size_t len;
	len = array->len;
	/* search for region */
	IF_BSEARCH(array->items, len, region, i) {
		/* Found it */
		return true;
	} ENDIF_BSEARCH;
	/* couldn't find it... */
	return false;
}
