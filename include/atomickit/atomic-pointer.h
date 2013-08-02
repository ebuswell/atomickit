/** @file atomic-pointer.h
 * Atomic Pointers
 *
 * C11-like atomic functions for pointers.  These should be usable
 * just like the corresponding C11 atomic functions, with `volatile
 * atomic_ptr *` and such substituted for the usual `volatile <atomic
 * type> *`.
 */
/* 
 * Copyright 2013 Evan Buswell
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
#ifndef ATOMICKIT_ATOMIC_POINTER_H
#define ATOMICKIT_ATOMIC_POINTER_H

#include <atomickit/atomic.h>

typedef struct {
    atomic_uintptr_t uintptr;
} atomic_ptr;

#define ATOMIC_PTR_VAR_INIT(value) { ATOMIC_VAR_INIT((uintptr_t) (value)) }

#define atomic_ptr_is_lock_free(obj) atomic_is_lock_free(obj)

static inline void atomic_ptr_init(volatile atomic_ptr *object, void *value) {
    atomic_init(&object->uintptr, (uintptr_t) value);
}

static inline void atomic_ptr_store_explicit(volatile atomic_ptr *object, void *desired, memory_order order) {
    atomic_store_explicit(&object->uintptr, (uintptr_t) desired, order);
}

static inline void atomic_ptr_store(volatile atomic_ptr *object, void *desired) {
    atomic_ptr_store_explicit(object, desired, memory_order_seq_cst);
}

static inline void *atomic_ptr_load_explicit(volatile atomic_ptr *object, memory_order order) {
    return (void *) atomic_load_explicit(&object->uintptr, order);
}

static inline void *atomic_ptr_load(volatile atomic_ptr *object) {
    return atomic_ptr_load_explicit(object, memory_order_seq_cst);
}

static inline void *atomic_ptr_exchange_explicit(volatile atomic_ptr *object, void *desired, memory_order order) {
    return (void *) atomic_exchange_explicit(&object->uintptr, (uintptr_t) desired, order);
}

static inline void *atomic_ptr_exchange(volatile atomic_ptr *object, void *desired) {
    return atomic_ptr_exchange_explicit(object, desired, memory_order_seq_cst);
}

static inline bool atomic_ptr_compare_exchange_strong_explicit(volatile atomic_ptr *object, void **expected, void *desired, memory_order success, memory_order failure) {
    return atomic_compare_exchange_strong_explicit(&object->uintptr, (uintptr_t *) expected, (uintptr_t) desired, success, failure);
}

static inline bool atomic_ptr_compare_exchange_strong(volatile atomic_ptr *object, void **expected, void *desired) {
    return atomic_ptr_compare_exchange_strong_explicit(object, expected, desired, memory_order_seq_cst, memory_order_seq_cst);
}

static bool atomic_ptr_compare_exchange_weak_explicit(volatile atomic_ptr *object, void **expected, void *desired, memory_order success, memory_order failure) {
    return atomic_compare_exchange_weak_explicit(&object->uintptr, (uintptr_t *) expected, (uintptr_t) desired, success, failure);
}

static inline bool atomic_ptr_compare_exchange_weak(volatile atomic_ptr *object, void **expected, void *desired) {
    return atomic_ptr_compare_exchange_weak_explicit(object, expected, desired, memory_order_seq_cst, memory_order_seq_cst);
}

static inline void *atomic_ptr_fetch_add_explicit(volatile atomic_ptr *object, ptrdiff_t operand, memory_order order) {
    return (void *) atomic_fetch_add_explicit(&object->uintptr, operand, order);
}

static inline void *atomic_ptr_fetch_add(volatile atomic_ptr *object, ptrdiff_t operand) {
    return atomic_ptr_fetch_add_explicit(object, operand, memory_order_seq_cst);
}

static inline void *atomic_ptr_fetch_sub_explicit(volatile atomic_ptr *object, ptrdiff_t operand, memory_order order) {
    return (void *) atomic_fetch_sub_explicit(&object->uintptr, operand, order);
}

static inline void *atomic_ptr_fetch_sub(volatile atomic_ptr *object, ptrdiff_t operand) {
    return atomic_ptr_fetch_sub_explicit(object, operand, memory_order_seq_cst);
}

static inline void *atomic_ptr_fetch_or_explicit(volatile atomic_ptr *object, ptrdiff_t operand, memory_order order) {
    return (void *) atomic_fetch_or_explicit(&object->uintptr, operand, order);
}

static inline void *atomic_ptr_fetch_or(volatile atomic_ptr *object, ptrdiff_t operand) {
    return atomic_ptr_fetch_or_explicit(object, operand, memory_order_seq_cst);
}

static inline void *atomic_ptr_fetch_xor_explicit(volatile atomic_ptr *object, ptrdiff_t operand, memory_order order) {
    return (void *) atomic_fetch_xor_explicit(&object->uintptr, operand, order);
}

static inline void *atomic_ptr_fetch_xor(volatile atomic_ptr *object, ptrdiff_t operand) {
    return atomic_ptr_fetch_xor_explicit(object, operand, memory_order_seq_cst);
}

static inline void *atomic_ptr_fetch_and_explicit(volatile atomic_ptr *object, ptrdiff_t operand, memory_order order) {
    return (void *) atomic_fetch_and_explicit(&object->uintptr, operand, order);
}

static inline void *atomic_ptr_fetch_and(volatile atomic_ptr *object, ptrdiff_t operand) {
    return atomic_ptr_fetch_and_explicit(object, operand, memory_order_seq_cst);
}

#endif /* _ATOMICKIT_ARCH_ATOMIC_PTR_H */
