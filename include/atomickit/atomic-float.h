/*
 * atomic-float.h
 * 
 * Copyright 2012 Evan Buswell
 *
 * This file is a redaction of arch/x86/include/atomic*.h and some
 * other files from the Linux Kernel.  In the future, someone should
 * make similar files for other architectures.  For now, though, we
 * depend on x86.  See the Linux documentation for more information.
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
#ifndef ATOMICKIT_ATOMIC_FLOAT_H
#define ATOMICKIT_ATOMIC_FLOAT_H

#include <math.h>
#include <atomickit/atomic.h>

typedef atomic_uint_least32_t atomic_float;
typedef atomic_uint_least64_t atomic_double;

#define atomic_float_is_lock_free(obj) atomic_is_lock_free(obj)
#define atomic_double_is_lock_free(obj) atomic_is_lock_free(obj)

#define __AK_canonical_float(f)					\
    (unlikely(isnan(f)) ? NAN : ((f) == 0.0f ? 0.0f : (f)))

#define __AK_canonical_double(d)				\
    (unlikely(isnan(d)) ? NAN : ((d) == 0.0 ? 0.0 : (d)))

union float_least32_pun {
    float f;
    uint_least32_t u;
};

union double_least64_pun {
    double d;
    uint_least64_t u;
};

static inline void atomic_float_init(volatile atomic_float *object, float value) {
    union float_least32_pun pun;
    pun.f = __AK_canonical_float(value);
    atomic_init(object, pun.u);
}

static inline void atomic_double_init(volatile atomic_double *object, double value) {
    union double_least64_pun pun;
    pun.d = __AK_canonical_double(value);
    atomic_init(object, pun.u);
}

static inline void atomic_float_store_explicit(volatile atomic_float *object, float desired, memory_order order) {
    union float_least32_pun pun;
    pun.f = __AK_canonical_float(desired);
    atomic_store_explicit(object, pun.u, order);
}

static inline void atomic_double_store_explicit(volatile atomic_double *object, double desired, memory_order order) {
    union double_least64_pun pun;
    pun.d = __AK_canonical_double(desired);
    atomic_store_explicit(object, pun.u, order);
}

static inline void atomic_float_store(volatile atomic_float *object, float desired) {
    atomic_float_store_explicit(object, desired, memory_order_seq_cst);
}

static inline void atomic_double_store(volatile atomic_double *object, double desired) {
    atomic_double_store_explicit(object, desired, memory_order_seq_cst);
}

static inline float atomic_float_load_explicit(volatile atomic_float *object, memory_order order) {
    union float_least32_pun pun;
    pun.u = atomic_load_explicit(object, order);
    return pun.f;
}

static inline double atomic_double_load_explicit(volatile atomic_double *object, memory_order order) {
    union double_least64_pun pun;
    pun.u = atomic_load_explicit(object, order);
    return pun.d;
}

static inline float atomic_float_load(volatile atomic_float *object) {
    return atomic_float_load_explicit(object, memory_order_seq_cst);
}

static inline double atomic_double_load(volatile atomic_double *object) {
    return atomic_double_load_explicit(object, memory_order_seq_cst);
}

static inline float atomic_float_exchange_explicit(volatile atomic_float *object, float desired, memory_order order) {
    union float_least32_pun pun;
    pun.f = __AK_canonical_float(desired);
    pun.u = atomic_exchange_explicit(object, pun.u, order);
    return pun.f;
}

static inline double atomic_double_exchange_explicit(volatile atomic_double *object, double desired, memory_order order) {
    union double_least64_pun pun;
    pun.d = __AK_canonical_double(desired);
    pun.u = atomic_exchange_explicit(object, pun.u, order);
    return pun.d;
}

static inline float atomic_float_exchange(volatile atomic_float *object, float desired) {
    return atomic_float_exchange_explicit(object, desired, memory_order_seq_cst);
}

static inline double atomic_double_exchange(volatile atomic_double *object, double desired) {
    return atomic_double_exchange_explicit(object, desired, memory_order_seq_cst);
}

static inline bool atomic_float_compare_exchange_strong_explicit(volatile atomic_float *object, float *expected, float desired, memory_order success, memory_order failure) {
    union float_least32_pun pun;
    pun.f = __AK_canonical_float(desired);
    *expected = __AK_canonical_float(*expected);
    return atomic_compare_exchange_strong_explicit(object, (uint_least32_t *) expected, pun.u, success, failure);
}

static inline bool atomic_double_compare_exchange_strong_explicit(volatile atomic_double *object, double *expected, double desired, memory_order success, memory_order failure) {
    union double_least64_pun pun;
    pun.d = __AK_canonical_double(desired);
    *expected = __AK_canonical_double(*expected);
    return atomic_compare_exchange_strong_explicit(object, (uint_least64_t *) expected, pun.u, success, failure);
}

static inline bool atomic_float_compare_exchange_strong(volatile atomic_float *object, float *expected, float desired) {
    return atomic_float_compare_exchange_strong_explicit(object, expected, desired, memory_order_seq_cst, memory_order_seq_cst);
}

static inline bool atomic_double_compare_exchange_strong(volatile atomic_double *object, double *expected, double desired) {
    return atomic_double_compare_exchange_strong_explicit(object, expected, desired, memory_order_seq_cst, memory_order_seq_cst);
}

static bool atomic_float_compare_exchange_weak_explicit(volatile atomic_float *object, float *expected, float desired, memory_order success, memory_order failure) {
    union float_least32_pun pun;
    pun.f = __AK_canonical_float(desired);
    *expected = __AK_canonical_float(*expected);
    return atomic_compare_exchange_weak_explicit(object, (uint_least32_t *) expected, pun.u, success, failure);
}

static bool atomic_double_compare_exchange_weak_explicit(volatile atomic_double *object, double *expected, double desired, memory_order success, memory_order failure) {
    union double_least64_pun pun;
    pun.d = __AK_canonical_double(desired);
    *expected = __AK_canonical_double(*expected);
    return atomic_compare_exchange_weak_explicit(object, (uint_least64_t *) expected, pun.u, success, failure);
}

static inline bool atomic_float_compare_exchange_weak(volatile atomic_float *object, float *expected, float desired) {
    return atomic_float_compare_exchange_weak_explicit(object, expected, desired, memory_order_seq_cst, memory_order_seq_cst);
}

static inline bool atomic_double_compare_exchange_weak(volatile atomic_double *object, double *expected, double desired) {
    return atomic_double_compare_exchange_weak_explicit(object, expected, desired, memory_order_seq_cst, memory_order_seq_cst);
}

#endif /* ATOMICKIT_ATOMIC_FLOAT_H */
