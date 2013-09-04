/** @file atomic-array.h
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
    struct arcp_region;
    size_t length;
    struct arcp_region *items[];
};

#define AARY_OVERHEAD (sizeof(struct aary))

#define AARY_SIZE(n) (AARY_OVERHEAD + sizeof(struct arcp_region *) * n)

struct aary *aary_dup(struct aary *array);
struct aary *aary_new(size_t length);

static inline size_t aary_length(struct aary *array) {
    return array->length;
}

static inline struct arcp_region *aary_load(struct aary *array, size_t i) {
    return arcp_acquire(array->items[i]);
}

static inline struct arcp_region *aary_load_phantom(struct aary *array, size_t i) {
    return array->items[i];
}

static inline struct arcp_region *aary_last(struct aary *array) {
    return arcp_acquire(array->items[array->length - 1]);
}

static inline struct arcp_region *aary_last_phantom(struct aary *array) {
    return array->items[array->length - 1];
}

static inline struct arcp_region *aary_first(struct aary *array) {
    return arcp_acquire(array->items[0]);
}

static inline struct arcp_region *aary_first_phantom(struct aary *array) {
    return array->items[0];
}

static inline void aary_store(struct aary *array, size_t i, struct arcp_region *region) {
    arcp_release(array->items[i]);
    array->items[i] = arcp_acquire(region);
}

static inline void aary_storefirst(struct aary *array, struct arcp_region *region) {
    arcp_release(array->items[0]);
    array->items[0] = arcp_acquire(region);
}

static inline void aary_storelast(struct aary *array, struct arcp_region *region) {
    arcp_release(array->items[array->length - 1]);
    array->items[array->length - 1] = arcp_acquire(region);
}

struct aary *aary_insert(struct aary *array, size_t i, struct arcp_region *region);
struct aary *aary_dup_insert(struct aary *array, size_t i, struct arcp_region *region);
struct aary *aary_remove(struct aary *array, size_t i);
struct aary *aary_dup_remove(struct aary *array, size_t i);
struct aary *aary_append(struct aary *array, struct arcp_region *region);
struct aary *aary_dup_append(struct aary *array, struct arcp_region *region);
struct aary *aary_pop(struct aary *array);
struct aary *aary_dup_pop(struct aary *array);
struct aary *aary_prepend(struct aary *array, struct arcp_region *region);
struct aary *aary_dup_prepend(struct aary *array, struct arcp_region *region);
struct aary *aary_shift(struct aary *array);
struct aary *aary_dup_shift(struct aary *array);
void aary_sortx(struct aary *array);
void aary_sort(struct aary *array, int (*compar)(const struct arcp_region *, const struct arcp_region *));
void aary_sort_r(struct aary *array, int (*compar)(const struct arcp_region *, const struct arcp_region *, void *arg), void *arg);
void aary_reverse(struct aary *array);
struct aary *aary_set_add(struct aary *array, struct arcp_region *region);
struct aary *aary_dup_set_add(struct aary *array, struct arcp_region *region);
struct aary *aary_set_remove(struct aary *array, struct arcp_region *region);
struct aary *aary_dup_set_remove(struct aary *array, struct arcp_region *region);
bool aary_set_contains(struct aary *array, struct arcp_region *region);

#endif /* ! ATOMICKIT_ATOMIC_ARRAY_H */
