#define _GNU_SOURCE
#include <stdlib.h>
#undef _GNU_SOURCE
#include <string.h>
#include <stddef.h>
#include <atomickit/atomic-array.h>
#include <atomickit/atomic-malloc.h>

static void __aary_destroy(struct aary *array) {
    size_t i;
    for(i = 0; i < array->length; i++) {
	arcp_release(array->items[i]);
    }
    afree(array, AARY_SIZE(array->length));
}

struct aary *aary_dup(struct aary *array) {
    struct aary *ret = amalloc(AARY_SIZE(array->length));
    if(ret == NULL) {
	return NULL;
    }
    size_t i;
    for(i = 0; i < array->length; i++) {
	arcp_incref(array->items[i]);
	ret->items[i] = array->items[i];
    }
    ret->length = array->length;
    arcp_region_init((struct arcp_region *) ret, (void (*)(struct arcp_region *)) __aary_destroy);
    return ret;
}

struct aary *aary_new(size_t length) {
    struct aary *ret = amalloc(AARY_SIZE(length));
    if(ret == NULL) {
	return NULL;
    }
    memset(ret->items, 0, sizeof(struct arcp_region *) * length);
    ret->length = length;
    arcp_region_init((struct arcp_region *) ret, (void (*)(struct arcp_region *)) __aary_destroy);
    return ret;
}

#define IF_BSEARCH(list, lower, nitems, value, retidx)	do {	\
    (retidx) = lower;						\
    size_t __l = lower;						\
    size_t __u = (nitems);					\
    while(__l < __u) {						\
        (retidx) = (__l + __u) / 2;				\
	struct arcp_region *__v = (list)[retidx];		\
	if((value) < __v) {					\
	    __u = (retidx);					\
	} else if((value) > __v) {				\
	    __l = ++(retidx);					\
	} else

#define ENDIF_BSEARCH	\
    }			\
} while(0)

struct aary *aary_insert(struct aary *array, size_t i, struct arcp_region *region) {
    size_t length = array->length;
    if(atryrealloc(array, AARY_SIZE(length), AARY_SIZE(length + 1))) {
	memmove(&array->items[i + 1], &array->items[i], sizeof(struct arcp_region *) * (length - i));
    } else {
	struct aary *new_array = amalloc(AARY_SIZE(length + 1));
	if(new_array == NULL) {
	    return NULL;
	}
	memcpy(new_array, array, AARY_SIZE(i));
	memcpy(&new_array->items[i + 1], &array->items[i], sizeof(struct arcp_region *) * (length - i));
	afree(array, AARY_SIZE(length));
	array = new_array;
    }
    array->length = length + 1;
    arcp_incref(region);
    array->items[i] = region;
    return array;
}

struct aary *aary_dup_insert(struct aary *array, size_t i, struct arcp_region *region) {
    size_t length = array->length;
    struct aary *new_array = amalloc(AARY_SIZE(length + 1));
    if(new_array == NULL) {
	return NULL;
    }
    size_t j;
    for(j = 0; j < i; j++) {
	arcp_incref(array->items[j]);
	new_array->items[j] = array->items[j];
    }
    for(; j < length; j++) {
	arcp_incref(array->items[j]);
	new_array->items[j + 1] = array->items[j];
    }
    new_array->length = length + 1;
    arcp_incref(region);
    new_array->items[i] = region;
    arcp_region_init((struct arcp_region *) new_array, (void (*)(struct arcp_region *)) __aary_destroy);
    return new_array;
}

struct aary *aary_remove(struct aary *array, size_t i) {
    size_t length = array->length;
    struct arcp_region *region = array->items[i];
    struct arcp_region *last = array->items[length - 1];
    if(atryrealloc(array, AARY_SIZE(length), AARY_SIZE(length - 1))) {
	memmove(&array->items[i], &array->items[i + 1], sizeof(struct arcp_region *) * ((length - 1) - (i + 1)));
	array->items[length - 2] = last;
    } else {
	struct aary *new_array = amalloc(AARY_SIZE(length - 1));
	if(new_array == NULL) {
	    return NULL;
	}
	memcpy(new_array, array, AARY_SIZE(i));
	memcpy(&new_array->items[i], &array->items[i + 1], sizeof(struct arcp_region *) * (length - (i + 1)));
	afree(array, AARY_SIZE(length));
	array = new_array;
    }
    array->length = length - 1;
    arcp_release(region);
    return array;
}

struct aary *aary_dup_remove(struct aary *array, size_t i) {
    size_t length = array->length;
    struct arcp_region *region = array->items[i];
    struct aary *new_array = amalloc(AARY_SIZE(length - 1));
    if(new_array == NULL) {
	return NULL;
    }
    size_t j;
    for(j = 0; j < i; j++) {
	arcp_incref(array->items[j]);
	new_array->items[j] = array->items[j];
    }
    for(; j + 1 < length; j++) {
	arcp_incref(array->items[j + 1]);
	new_array->items[j] = array->items[j + 1];
    }
    new_array->length = length - 1;
    arcp_release(region);
    arcp_region_init((struct arcp_region *) new_array, (void (*)(struct arcp_region *)) __aary_destroy);
    return new_array;
}

struct aary *aary_append(struct aary *array, struct arcp_region *region) {
    size_t length = array->length;
    array = arealloc(array, AARY_SIZE(length), AARY_SIZE(length + 1));
    if(array == NULL) {
	return NULL;
    }
    array->length = length + 1;
    arcp_incref(region);
    array->items[length] = region;
    return array;
}

struct aary *aary_dup_append(struct aary *array, struct arcp_region *region) {
    size_t length = array->length;
    struct aary *new_array = amalloc(AARY_SIZE(length + 1));
    if(new_array == NULL) {
	return NULL;
    }
    size_t j;
    for(j = 0; j < length; j++) {
	arcp_incref(array->items[j]);
	new_array->items[j] = array->items[j];
    }
    new_array->length = length + 1;
    arcp_incref(region);
    new_array->items[length] = region;
    arcp_region_init((struct arcp_region *) new_array, (void (*)(struct arcp_region *)) __aary_destroy);
    return new_array;
}

struct aary *aary_pop(struct aary *array) {
    size_t length = array->length;
    struct arcp_region *region = array->items[length - 1];
    array = arealloc(array, AARY_SIZE(length), AARY_SIZE(length - 1));
    if(array == NULL) {
	return NULL;
    }
    array->length = length - 1;
    arcp_release(region);
    return array;
}

struct aary *aary_dup_pop(struct aary *array) {
    size_t length = array->length;
    struct arcp_region *region = array->items[length - 1];
    struct aary *new_array = amalloc(AARY_SIZE(length - 1));
    if(new_array == NULL) {
	return NULL;
    }
    size_t j;
    for(j = 0; j < length - 1; j++) {
	arcp_incref(array->items[j]);
	new_array->items[j] = array->items[j];
    }
    new_array->length = length - 1;
    arcp_release(region);
    arcp_region_init((struct arcp_region *) new_array, (void (*)(struct arcp_region *)) __aary_destroy);
    return new_array;
}

struct aary *aary_prepend(struct aary *array, struct arcp_region *region) {
    size_t length = array->length;
    if(atryrealloc(array, AARY_SIZE(length), AARY_SIZE(length + 1))) {
	memmove(&array->items[1], &array->items[0], sizeof(struct arcp_region *) * length);
    } else {
	struct aary *new_array = amalloc(AARY_SIZE(length + 1));
	if(new_array == NULL) {
	    return NULL;
	}
	memcpy(new_array, array, AARY_OVERHEAD);
	memcpy(&new_array->items[1], &array->items[0], sizeof(struct arcp_region *) * length);
	afree(array, AARY_SIZE(length));
	array = new_array;
    }
    array->length = length + 1;
    arcp_incref(region);
    array->items[0] = region;
    return array;
}

struct aary *aary_dup_prepend(struct aary *array, struct arcp_region *region) {
    size_t length = array->length;
    struct aary *new_array = amalloc(AARY_SIZE(length + 1));
    if(new_array == NULL) {
	return NULL;
    }
    size_t j;
    for(j = 0; j < length; j++) {
	arcp_incref(array->items[j]);
	new_array->items[j + 1] = array->items[j];
    }
    new_array->length = length + 1;
    arcp_incref(region);
    new_array->items[0] = region;
    arcp_region_init((struct arcp_region *) new_array, (void (*)(struct arcp_region *)) __aary_destroy);
    return new_array;
}

struct aary *aary_shift(struct aary *array) {
    size_t length = array->length;
    struct arcp_region *region = array->items[0];
    struct arcp_region *last = array->items[length - 1];
    if(atryrealloc(array, AARY_SIZE(length), AARY_SIZE(length - 1))) {
	memmove(&array->items[0], &array->items[1], sizeof(struct arcp_region *) * ((length - 1) - 1));
	array->items[length - 2] = last;
    } else {
	struct aary *new_array = amalloc(AARY_SIZE(length - 1));
	if(new_array == NULL) {
	    return NULL;
	}
	memcpy(new_array, array, AARY_OVERHEAD);
	memcpy(&new_array->items[0], &array->items[1], sizeof(struct arcp_region *) * (length - 1));
	afree(array, AARY_SIZE(length));
	array = new_array;
    }
    array->length = length - 1;
    arcp_release(region);
    return array;
}

struct aary *aary_dup_shift(struct aary *array) {
    size_t length = array->length;
    struct arcp_region *region = array->items[0];
    struct aary *new_array = amalloc(AARY_SIZE(length - 1));
    if(new_array == NULL) {
	return NULL;
    }
    size_t j;
    for(j = 1; j < length; j++) {
	arcp_incref(array->items[j]);
	new_array->items[j - 1] = array->items[j];
    }
    new_array->length = length - 1;
    arcp_release(region);
    arcp_region_init((struct arcp_region *) new_array, (void (*)(struct arcp_region *)) __aary_destroy);
    return new_array;
}

int __aary_compar(const struct arcp_region *region1, const struct arcp_region *region2) {
    if(region1 < region2) {
	return -1;
    } else if(region1 == region2) {
	return 0;
    } else {
	return 1;
    }
}

void aary_sortx(struct aary *array) {
    qsort(array->items, array->length, sizeof(struct arcp_region *), (int (*)(const void *, const void *)) __aary_compar);
}

void aary_sort(struct aary *array, int (*compar)(const struct arcp_region *, const struct arcp_region *, void *), void *arg) {
    qsort_r(array->items, array->length, sizeof(struct arcp_region *), (int (*)(const void *, const void *, void *)) compar, arg);
}

void aary_reverse(struct aary *array) {
    size_t i, j;
    for(i = 0, j = array->length - 1; i < j; i++, j--) {
	struct arcp_region *tmp = array->items[i];
	array->items[i] = array->items[j];
	array->items[j] = tmp;
    }
}

/* overwriteary, insertary, removeary, overwriteendary, concat, popary, overwritestartary, concat, shiftary */

struct aary *aary_set_add(struct aary *array, struct arcp_region *region) {
    /* search for region */
    size_t i;
    size_t length = array->length;
    IF_BSEARCH(array->items, 0, length, region, i) {
	/* Already present; do nothing */
	return array;
    } ENDIF_BSEARCH;
    return aary_insert(array, i, region);
}

struct aary *aary_dup_set_add(struct aary *array, struct arcp_region *region) {
    /* search for region */
    size_t i;
    size_t length = array->length;
    IF_BSEARCH(array->items, 0, length, region, i) {
	/* Already present; do nothing */
	return aary_dup(array);
    } ENDIF_BSEARCH;
    return aary_dup_insert(array, i, region);
}

struct aary *aary_set_remove(struct aary *array, struct arcp_region *region) {
    /* search for region */
    size_t i;
    size_t length = array->length;
    IF_BSEARCH(array->items, 0, length, region, i) {
	/* Found it */
	return aary_remove(array, i);
    } ENDIF_BSEARCH;
    /* couldn't find it... */
    return array;
}

struct aary *aary_dup_set_remove(struct aary *array, struct arcp_region *region) {
    /* search for region */
    size_t i;
    size_t length = array->length;
    IF_BSEARCH(array->items, 0, length, region, i) {
	/* Found it */
	return aary_dup_remove(array, i);
    } ENDIF_BSEARCH;
    /* couldn't find it... */
    return aary_dup(array);
}

bool aary_set_contains(struct aary *array, struct arcp_region *region) {
    size_t i;
    IF_BSEARCH(array->items, 0, array->length, region, i) {
	return true;
    } ENDIF_BSEARCH;
    return false;
}

/* union (|), intersection (&), difference (\), disjunction (^), equal, disjoint, subset, proper_subset, superset, proper_superset */
