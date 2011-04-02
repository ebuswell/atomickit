/*
 * atomic-float.h
 * 
 * Copyright 2010 Evan Buswell
 * Copyright 2010 Linus Torvalds et al.
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
#ifndef _ATOMICKIT_ARCH_ATOMIC_FLOAT_H
#define _ATOMICKIT_ARCH_ATOMIC_FLOAT_H

#include <atomickit/atomic.h>
#include <atomickit/atomic-types.h>

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */

#define ATOMIC_FLOAT_INIT(f)	{ (f) }

/**
 * atomic_float_read - read atomic variable
 * @v: pointer of type atomic_float_t
 *
 * Atomically reads the value of @v.
 */
static inline float atomic_float_read(const atomic_float_t *v)
{
	return v->counter;
}

/**
 * atomic_float_set - set atomic variable
 * @v: pointer of type atomic_float_t
 * @f: required value
 *
 * Atomically sets the value of @v to @f.
 */
static inline void atomic_float_set(atomic_float_t *v, float f)
{
	v->counter = f;
}

static inline float atomic_float_cmpxchg(atomic_float_t *v, float old, float new)
{
	union {
		float f;
		int i;
	} u_old;
	union {
		float f;
		int i;
	} u_new;
	union {
		float f;
		int i;
	} r;
	u_old.f = old;
	u_new.f = new;
	r.i = atomic_cmpxchg((atomic_t *) v, u_old.i, u_new.i);
	return r.f;
}

static inline float atomic_float_xchg(atomic_float_t *v, float new)
{
	union {
		float f;
		int i;
	} u_new;
	union {
		float f;
		int i;
	} r;
	u_new.f = new;
	r.i = atomic_xchg((atomic_t *) v, new);

	return r.f;
}

/**
 * atomic_float_add - add integer to atomic variable
 * @f: float value to add
 * @v: pointer of type atomic_float_t
 *
 * Atomically adds @f to @v.
 */
static inline void atomic_float_add(float f, atomic_float_t *v)
{
	float old;
	float c;
	c = atomic_float_read(v);
	for(;;) {
		old = atomic_float_cmpxchg((v), c, c + (f));
		if(likely(old == c))
			break;
		c = old;
	}
}

/**
 * atomic_float_sub - subtract the atomic variable
 * @f: float value to subtract
 * @v: pointer of type atomic_float_t
 *
 * Atomically subtracts @f from @v.
 */
static inline void atomic_float_sub(float f, atomic_float_t *v)
{
	float old;
	float c;
	c = atomic_float_read(v);
	for(;;) {
		old = atomic_float_cmpxchg(v, c, c - f);
		if(likely(old == c))
			break;
		c = old;
	}
}

/**
 * atomic_float_add_return - add and return
 * @f: float value to add
 * @v: pointer of type atomic_float_t
 *
 * Atomically adds @f to @v and returns @f + @v
 */
static inline float atomic_float_add_return(float f, atomic_float_t *v)
{
	float old;
	float new;
	float c;
	c = atomic_float_read(v);
	for(;;) {
		new = c + f;
		old = atomic_float_cmpxchg(v, c, new);
		if(likely(old == c))
			break;
		c = old;
	}
	return new;
}

static inline float atomic_float_sub_return(float f, atomic_float_t *v)
{
	float old;
	float new;
	float c;
	c = atomic_float_read(v);
	for(;;) {
		new = c - f;
		old = atomic_float_cmpxchg(v, c, new);
		if(likely(old == c))
			break;
		c = old;
	}
	return new;
}

/**
 * atomic_float_sub_and_test - subtract value from variable and test result
 * @f: float value to subtract
 * @v: pointer of type atomic_float_t
 *
 * Atomically subtracts @f from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static inline int atomic_float_sub_and_test(float f, atomic_float_t *v)
{
	return atomic_float_sub_return(f, v) == 0.0f;
}

/**
 * atomic_float_inc - increment atomic variable
 * @v: pointer of type atomic_float_t
 *
 * Atomically increments @v by 1.
 */
static inline void atomic_float_inc(atomic_float_t *v)
{
	atomic_float_add(1.0f, v);
}

/**
 * atomic_float_dec - decrement atomic variable
 * @v: pointer of type atomic_float_t
 *
 * Atomically decrements @v by 1.
 */
static inline void atomic_float_dec(atomic_float_t *v)
{
	atomic_float_sub(1.0f, v);
}

/**
 * atomic_float_dec_and_test - decrement and test
 * @v: pointer of type atomic_float_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static inline int atomic_float_dec_and_test(atomic_float_t *v)
{
	return atomic_float_sub_and_test(1.0f, v);
}

/**
 * atomic_float_inc_and_test - increment and test
 * @v: pointer of type atomic_float_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static inline int atomic_float_inc_and_test(atomic_float_t *v)
{
	return atomic_float_add_return(1.0f, v) == 0.0f;
}

/**
 * atomic_float_add_negative - add and test if negative
 * @f: float value to add
 * @v: pointer of type atomic_float_t
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static inline int atomic_float_add_negative(float f, atomic_float_t *v)
{
	return atomic_float_add_return(f, v) < 0.0f;
}

#define atomic_float_inc_return(v)  (atomic_float_add_return(1.0f, (v)))
#define atomic_float_dec_return(v)  (atomic_float_sub_return(1.0f, (v)))


/**
 * atomic_float_add_unless - add unless the number is a given value
 * @v: pointer of type atomic_float_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so long as it was not @u.
 * Returns non-zero if @v was not @u, and zero otherwise.
 */
static inline int atomic_float_add_unless(atomic_float_t *v, float a, float u)
{
	float c, old;
	c = atomic_float_read(v);
	for (;;) {
		if (unlikely(c == (u)))
			break;
		old = atomic_float_cmpxchg((v), c, c + (a));
		if (likely(old == c))
			break;
		c = old;
	}
	return c != (u);
}

#define atomic_float_inc_not_zero(v) atomic_float_add_unless((v), 1.0, 0.0)

#endif /* _ATOMICKIT_ARCH_ATOMIC_FLOAT_H */
