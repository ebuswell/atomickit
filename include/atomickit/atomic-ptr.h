/*
 * atomic-ptr.h
 * 
 * Copyright 2010 Evan Buswell
 * Copyright 2010 Linus Torvalds et al.
 *
 * This file is part of Atomic Kit.
 * 
 * Atomic Kit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 * 
 * Atomic Kit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _ATOMICKIT_ATOMIC_PTR_H
#define _ATOMICKIT_ATOMIC_PTR_H

#include <atomickit/atomic.h>
#include <atomickit/atomic-types.h>

#ifdef __LP64__
#define ATOMIC_PTR_INIT(p) ATOMIC64_INIT(p)
#define atomic_ptr_read(v) ((void *) atomic64_read(v))
#define atomic_ptr_set(v, p) atomic64_set((v), (long) (p))
#define atomic_ptr_add(i, v) atomic64_add((v), (i))
#define atomic_ptr_sub(i, v) atomic64_sub((v), (i))
#define atomic_ptr_sub_and_test(i, v) atomic64_sub_and_test((v), (i))
#define atomic_ptr_inc(v) atomic64_inc(v)
#define atomic_ptr_dec(v) atomic64_dec(v)
#define atomic_ptr_dec_and_test(v) atomic64_dec_and_test(v)
#define atomic_ptr_add_return(i, v) ((void *) atomic64_add_return((i), (v))
#define atomic_ptr_sub_return(i, v) ((void *) atomic64_sub_return((i), (v))
#define atomic_ptr_inc_return(v) ((void *) atomic64_inc_return(v)
#define atomic_ptr_dec_return(v) ((void *) atomic64_dec_return(v)
#define atomic_ptr_cmpxchg(v, old, new) ((void *) atomic64_cmpxchg((v), (long) (old), (long) (new)))
#define atomic_ptr_xchg(v, new) ((void *) atomic64_xchg((v), (long) (new)))
#define atomic_ptr_add_unless(v, a, u) atomic64_add_unless((v), a, (long) (u))
#define atomic_ptr_inc_not_zero(v) atomic64_inc_not_zero(v)
#else /* #ifdef __LP64__ */
#define atomic_ptr_t atomic_t
#define ATOMIC_PTR_INIT(p) ATOMIC_INIT(p)
#define atomic_ptr_read(v) ((void *) atomic_read(v))
#define atomic_ptr_set(v, p) atomic_set((v), (long) (p))
#define atomic_ptr_add(i, v) atomic_add((v), (i))
#define atomic_ptr_sub(i, v) atomic_sub((v), (i))
#define atomic_ptr_sub_and_test(i, v) atomic_sub_and_test((v), (i))
#define atomic_ptr_inc(v) atomic_inc(v)
#define atomic_ptr_dec(v) atomic_dec(v)
#define atomic_ptr_dec_and_test(v) atomic_dec_and_test(v)
#define atomic_ptr_add_return(i, v) ((void *) atomic_add_return((i), (v))
#define atomic_ptr_sub_return(i, v) ((void *) atomic_sub_return((i), (v))
#define atomic_ptr_inc_return(v) ((void *) atomic_inc_return(v)
#define atomic_ptr_dec_return(v) ((void *) atomic_dec_return(v)
#define atomic_ptr_cmpxchg(v, old, new) ((void *) atomic_cmpxchg((v), (long) (old), (long) (new)))
#define atomic_ptr_xchg(v, new) ((void *) atomic_xchg((v), (long) (new)))
#define atomic_ptr_add_unless(v, a, u) atomic_add_unless((v), a, (long) (u))
#define atomic_ptr_inc_not_zero(v) atomic_inc_not_zero(v)
#endif /* #ifdef __LP64__ */
#endif /* #ifndef _ATOMICKIT_ATOMIC_PTR_H */
