/**
 * Atomic Array
 *
 * A copy-on-write array that can be used as a list or a set
 */
/*
 * atomic-array.h
 * 
 * Copyright 2012 Evan Buswell
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
#ifndef ATOMICKIT_ATOMIC_ARRAY_H
#define ATOMICKIT_ATOMIC_ARRAY_H 1

#include <stddef.h>
#include <atomickit/atomic-rcp.h>

struct aary {
    struct arcp_region_header header;
    size_t length;
    struct arcp_region *items[1];
};

#define AARY_OVERHEAD (offsetof(struct aary, items))

#define AARY_SIZE(n) (AARY_OVERHEAD + sizeof(struct arcp_region *) * n)

struct aary *aary_dup(struct aary *array);
struct aary *aary_new(size_t length);

static inline size_t aary_length(struct aary *array) {
    return array->length;
}

static inline struct arcp_region *aary_load(struct aary *array, size_t i) {
    struct arcp_region *ret = array->items[i];
    arcp_incref(ret);
    return ret;
}

static inline struct arcp_region *aary_load_weak(struct aary *array, size_t i) {
    return array->items[i];
}

static inline struct arcp_region *aary_last(struct aary *array) {
    struct arcp_region *ret = array->items[array->length - 1];
    arcp_incref(ret);
    return ret;
}

static inline struct arcp_region *aary_last_weak(struct aary *array) {
    return array->items[array->length - 1];
}

static inline struct arcp_region *aary_first(struct aary *array) {
    struct arcp_region *ret = array->items[0];
    arcp_incref(ret);
    return ret;
}

static inline struct arcp_region *aary_first_weak(struct aary *array) {
    return array->items[0];
}

static inline void aary_store(struct aary *array, size_t i, struct arcp_region *region) {
    arcp_release(array->items[i]);
    arcp_incref(region);
    array->items[i] = region;
}

static inline void aary_storefirst(struct aary *array, struct arcp_region *region) {
    arcp_release(array->items[0]);
    arcp_incref(region);
    array->items[0] = region;
}

static inline void aary_storelast(struct aary *array, struct arcp_region *region) {
    arcp_release(array->items[array->length - 1]);
    arcp_incref(region);
    array->items[array->length - 1] = region;
}

struct aary *aary_insert(struct aary *array, size_t i, struct arcp_region *region);
struct aary *aary_remove(struct aary *array, size_t i);
struct aary *aary_append(struct aary *array, struct arcp_region *region);
struct aary *aary_pop(struct aary *array);
struct aary *aary_prepend(struct aary *array, struct arcp_region *region);
struct aary *aary_shift(struct aary *array);
void aary_sortx(struct aary *array);
void aary_sort(struct aary *array, int (*compar)(const struct arcp_region *, const struct arcp_region *, void *), void *arg);
void aary_reverse(struct aary *array);
struct aary *aary_set_add(struct aary *array, struct arcp_region *region);
struct aary *aary_set_remove(struct aary *array, struct arcp_region *region);
bool aary_set_contains(struct aary *array, struct arcp_region *region);

#endif /* ! ATOMICKIT_ATOMIC_ARRAY_H */
