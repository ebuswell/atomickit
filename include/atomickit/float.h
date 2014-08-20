/** @file float.h
 * Atomic Floats
 *
 * C11-like atomic functions for floating point numbers.  These should
 * be usable just like the corresponding C11 atomic functions, with
 * `volatile atomic_float *` and such substituted for the usual
 * `volatile <atomic type> *`.  Atomic add and subtract are not
 * included since these are would not be generically possible without
 * locks.
 */
/*
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
#ifndef ATOMICKIT_FLOAT_H
#define ATOMICKIT_FLOAT_H

#include <math.h>
#include <stdint.h>
#include <atomickit/atomic.h>

typedef _Atomic(uint32_t) atomic_float;
typedef _Atomic(uint64_t) atomic_double;

#define atomic_float_is_lock_free(obj) atomic_is_lock_free(obj)
#define atomic_double_is_lock_free(obj) atomic_is_lock_free(obj)

#define __AK_CANONICALF(f)						\
	(unlikely(isnanf(f)) ? NAN : ((f) == 0.0f ? 0.0f : (f)))

#define __AK_CANONICALD(d)					\
	(unlikely(isnan(d)) ? ((double) NAN) : ((d) == 0.0 ? 0.0 : (d)))

#define __AK_FLOAT2INT(f)						\
	(((union { float __f; uint32_t __i; } ) { .__f = f }).__i)

#define __AK_DOUBLE2INT(f)						\
	(((union { double __f; uint64_t __i; } ) { .__f = f }).__i)

#define __AK_INT2FLOAT(i)						\
	(((union { float __f; uint32_t __i; } ) { .__i = i }).__f)

#define __AK_INT2DOUBLE(i)						\
	(((union { double __f; uint64_t __i; } ) { .__i = i }).__f)

#define ak_float_init(object, value)					\
	ak_init((object), __AK_FLOAT2INT(__AK_CANONICALF(value)))

#define ak_double_init(object, value)					\
	ak_init((object), __AK_DOUBLE2INT(__AK_CANONICALD(value)))

#define ak_float_store(object, desired, order)				\
	ak_store((object), __AK_FLOAT2INT(__AK_CANONICALF(desired)),	\
	         (order))

#define ak_double_store(object, desired, order)				\
	ak_store((object), __AK_DOUBLE2INT(__AK_CANONICALD(desired)),	\
	         (order))

#define ak_float_load(object, order)					\
	__AK_INT2FLOAT(ak_load((object), (order)))

#define ak_double_load(object, order)					\
	__AK_INT2DOUBLE(ak_load((object), (order)))

#define ak_float_swap(object, desired, order)				\
	__AK_INT2FLOAT(ak_swap((object),				\
	                       __AK_FLOAT2INT(__AK_CANONICALF(desired)), \
	                       (order)))

#define ak_double_swap(object, desired, order)				\
	__AK_INT2DOUBLE(ak_swap((object),				\
	                        __AK_DOUBLE2INT(__AK_CANONICALD(desired)), \
	                        (order)))

#define ak_float_cas(object, expected, desired, success, failure) ({	\
	*(expected) = __AK_CANONICALF(*(expected));			\
	ak_cas((object), ((uint32_t *) (expected)),			\
	       __AK_FLOAT2INT(__AK_CANONICALF(desired)),		\
	       (success), (failure));					\
})

#define ak_double_cas(object, expected, desired, success, failure) ({	\
	*(expected) = __AK_CANONICALD(*(expected));			\
	ak_cas((object), (uint64_t *) (expected),			\
	       __AK_DOUBLE2INT(__AK_CANONICALD(desired)),		\
	       (success), (failure));					\
})

#define ak_float_cas_strong(object, expected, desired,			\
                            success, failure) ({			\
	*(expected) = __AK_CANONICALF(*(expected));			\
	ak_cas_strong((object), (uint32_t *) (expected),		\
	              __AK_FLOAT2INT(__AK_CANONICALF(desired)),		\
	              (success), (failure));				\
})

#define ak_double_cas_strong(object, expected, desired,			\
                             success, failure) ({			\
	*(expected) = __AK_CANONICALD(*(expected));			\
	ak_cas_strong((object), (uint64_t *) (expected),		\
	              __AK_DOUBLE2INT(__AK_CANONICALD(desired)),	\
	              (success), (failure));				\
})

#endif /* ATOMICKIT_FLOAT_H */
