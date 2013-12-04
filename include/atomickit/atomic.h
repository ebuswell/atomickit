/** @file atomic.h
 *
 * This provides `likely`/`unlikely` and will include the C11 atomic
 * header or a compatibility header if the C11 header does not exist
 * for this implementation of C.  If the C11 `stdatomic.h` header
 * exists, you should define `HAVE_STDATOMIC_H` before including this
 * file.
 *
 * This file is a redaction of arch/x86/include/compiler.h and some
 * other files from the Linux Kernel.
 */
/*
 * Copyright 2013 Evan Buswell
 * Copyright 2012 Linus Torvalds et al.
 * Copyright 1991, 1993 The Regents of the University of California
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
 * 
 * This code is derived from software contributed to Berkeley by
 * Berkeley Software Design, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef ATOMICKIT_ATOMIC_H
#define ATOMICKIT_ATOMIC_H 1

#include <stddef.h>
#include <stdint.h>

#ifndef __has_extension
# define __has_extension	__has_feature
#endif
#ifndef	__has_feature
# define __has_feature(x)	0
#endif
#ifndef	__has_include
# define __has_include(x)	0
#endif
#ifndef	__has_builtin
# define __has_builtin(x)	0
#endif
#ifndef	__has_attribute
# define __has_attribute(x)	0
#endif

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L

#if __has_extension(c_atomic)
# define __CLANG_ATOMICS
#elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
# define __GNUC_ATOMICS
#else
# define __ASM_ATOMICS
#endif

#ifndef __CLANG_ATOMICS
/*
 * No native support for _Atomic(). Place object in structure to
 * prevent most forms of direct non-atomic access.
 */
# define _Atomic(T)	struct { T volatile __val; }
#endif

/*
 * 7.17.1 Atomic lock-free macros.
 */

#ifdef __GCC_ATOMIC_BOOL_LOCK_FREE
# define ATOMIC_BOOL_LOCK_FREE		__GCC_ATOMIC_BOOL_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_CHAR_LOCK_FREE
# define ATOMIC_CHAR_LOCK_FREE		__GCC_ATOMIC_CHAR_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_WCHAR_T_LOCK_FREE
# define ATOMIC_WCHAR_T_LOCK_FREE	__GCC_ATOMIC_WCHAR_T_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_SHORT_LOCK_FREE
# define ATOMIC_SHORT_LOCK_FREE		__GCC_ATOMIC_SHORT_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_INT_LOCK_FREE
# define ATOMIC_INT_LOCK_FREE		__GCC_ATOMIC_INT_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_LONG_LOCK_FREE
# define ATOMIC_LONG_LOCK_FREE		__GCC_ATOMIC_LONG_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_LLONG_LOCK_FREE
# define ATOMIC_LLONG_LOCK_FREE		__GCC_ATOMIC_LLONG_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_POINTER_LOCK_FREE
# define ATOMIC_POINTER_LOCK_FREE	__GCC_ATOMIC_POINTER_LOCK_FREE
#endif

/*
 * 7.17.2 Initialization.
 */

#ifdef __CLANG_ATOMICS
# define ATOMIC_VAR_INIT(value)		(value)
# define atomic_init(obj, value)	__c11_atomic_init(obj, value)
#else
# define ATOMIC_VAR_INIT(value)		{ .__val = (value) }
# define atomic_init(obj, value)	((void) ((obj)->__val = (value)))
#endif

/*
 * Clang and recent GCC both provide predefined macros for the memory
 * orderings.  If we are using a compiler that doesn't define them, use the
 * clang values - these will be ignored in the fallback path.
 */

#ifndef __ATOMIC_RELAXED
# define __ATOMIC_RELAXED	0
#endif
#ifndef __ATOMIC_CONSUME
# define __ATOMIC_CONSUME	1
#endif
#ifndef __ATOMIC_ACQUIRE
# define __ATOMIC_ACQUIRE	2
#endif
#ifndef __ATOMIC_RELEASE
# define __ATOMIC_RELEASE	3
#endif
#ifndef __ATOMIC_ACQ_REL
# define __ATOMIC_ACQ_REL	4
#endif
#ifndef __ATOMIC_SEQ_CST
# define __ATOMIC_SEQ_CST	5
#endif

/*
 * 7.17.3 Order and consistency.
 *
 * The memory_order_* constants that denote the barrier behaviour of the atomic
 * operations.
 */

typedef enum {
	memory_order_relaxed = __ATOMIC_RELAXED,
	memory_order_consume = __ATOMIC_CONSUME,
	memory_order_acquire = __ATOMIC_ACQUIRE,
	memory_order_release = __ATOMIC_RELEASE,
	memory_order_acq_rel = __ATOMIC_ACQ_REL,
	memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order;

/*
 * 7.17.4 Fences.
 */

#ifdef __CLANG_ATOMICS
# define atomic_thread_fence(/* memory_order */ __order)		\
	__c11_atomic_thread_fence(__order)
#elif defined(__GNUC_ATOMICS)
# define atomic_thread_fence(/* memory_order */ __order)		\
	__atomic_thread_fence(__order)
#endif

#ifdef __CLANG_ATOMICS
# define atomic_signal_fence(/* memory_order */ __order)		\
	__c11_atomic_signal_fence(__order)
#elif defined(__GNUC_ATOMICS)
# define atomic_signal_fence(/* memory_order */ __order)		\
	__atomic_signal_fence(__order)
#endif

/*
 * 7.17.5 Lock-free property.
 */
#ifdef __CLANG_ATOMICS
# define atomic_is_lock_free(obj)					\
	__atomic_is_lock_free(sizeof(*(obj)), obj)
#elif defined(__GNUC_ATOMICS)
# define atomic_is_lock_free(obj)					\
	__atomic_is_lock_free(sizeof((obj)->__val), &(obj)->__val)
#endif

/*
 * 7.17.6 Atomic integer types.
 */

typedef _Atomic(_Bool)			atomic_bool;
typedef _Atomic(char)			atomic_char;
typedef _Atomic(signed char)		atomic_schar;
typedef _Atomic(unsigned char)		atomic_uchar;
typedef _Atomic(short)			atomic_short;
typedef _Atomic(unsigned short)		atomic_ushort;
typedef _Atomic(int)			atomic_int;
typedef _Atomic(unsigned int)		atomic_uint;
typedef _Atomic(long)			atomic_long;
typedef _Atomic(unsigned long)		atomic_ulong;
typedef _Atomic(long long)		atomic_llong;
typedef _Atomic(unsigned long long)	atomic_ullong;
typedef _Atomic(wchar_t)		atomic_wchar_t;
typedef _Atomic(int_least8_t)		atomic_int_least8_t;
typedef _Atomic(uint_least8_t)		atomic_uint_least8_t;
typedef _Atomic(int_least16_t)		atomic_int_least16_t;
typedef _Atomic(uint_least16_t)		atomic_uint_least16_t;
typedef _Atomic(int_least32_t)		atomic_int_least32_t;
typedef _Atomic(uint_least32_t)		atomic_uint_least32_t;
typedef _Atomic(int_least64_t)		atomic_int_least64_t;
typedef _Atomic(uint_least64_t)		atomic_uint_least64_t;
typedef _Atomic(int_fast8_t)		atomic_int_fast8_t;
typedef _Atomic(uint_fast8_t)		atomic_uint_fast8_t;
typedef _Atomic(int_fast16_t)		atomic_int_fast16_t;
typedef _Atomic(uint_fast16_t)		atomic_uint_fast16_t;
typedef _Atomic(int_fast32_t)		atomic_int_fast32_t;
typedef _Atomic(uint_fast32_t)		atomic_uint_fast32_t;
typedef _Atomic(int_fast64_t)		atomic_int_fast64_t;
typedef _Atomic(uint_fast64_t)		atomic_uint_fast64_t;
typedef _Atomic(intptr_t)		atomic_intptr_t;
typedef _Atomic(uintptr_t)		atomic_uintptr_t;
typedef _Atomic(size_t)			atomic_size_t;
typedef _Atomic(ptrdiff_t)		atomic_ptrdiff_t;
typedef _Atomic(intmax_t)		atomic_intmax_t;
typedef _Atomic(uintmax_t)		atomic_uintmax_t;

/*
 * 7.17.7 Operations on atomic types.
 */

#if defined(__CLANG_ATOMICS)
# define atomic_compare_exchange_strong_explicit(object, expected, desired \
                                                 success, failure)	   \
	__c11_atomic_compare_exchange_strong(object, expected, desired,	   \
	                                     success, failure)
# define atomic_compare_exchange_weak_explicit(object, expected, desired \
                                               success, failure)	 \
	__c11_atomic_compare_exchange_weak(object, expected, desired,	 \
                                           success, failure)
# define atomic_exchange_explicit(object, desired, order)		\
	__c11_atomic_exchange(object, desired, order)
# define atomic_fetch_add_explicit(object, operand, order)		\
	__c11_atomic_fetch_add(object, operand, order)
# define atomic_fetch_and_explicit(object, operand, order)		\
	__c11_atomic_fetch_and(object, operand, order)
# define atomic_fetch_or_explicit(object, operand, order)		\
	__c11_atomic_fetch_or(object, operand, order)
# define atomic_fetch_sub_explicit(object, operand, order)		\
	__c11_atomic_fetch_sub(object, operand, order)
# define atomic_fetch_xor_explicit(object, operand, order)		\
	__c11_atomic_fetch_xor(object, operand, order)
# define atomic_load_explicit(object, order)				\
	__c11_atomic_load(object, order)
# define atomic_store_explicit(object, desired, order)			\
	__c11_atomic_store(object, desired, order)
#elif defined(__GNUC_ATOMICS)
# define atomic_compare_exchange_strong_explicit(object, expected, desired, \
                                                 success, failure)	    \
	__atomic_compare_exchange_n(&(object)->__val, expected, desired,    \
	                            0, success, failure)
# define atomic_compare_exchange_weak_explicit(object, expected, desired,  \
                                               success, failure)	   \
	__atomic_compare_exchange_n(&(object)->__val, expected, desired, \
	                            1, success, failure)
# define atomic_exchange_explicit(object, desired, order)		\
	__atomic_exchange_n(&(object)->__val, desired, order)
# define atomic_fetch_add_explicit(object, operand, order)		\
	__atomic_fetch_add(&(object)->__val, operand, order)
# define atomic_fetch_and_explicit(object, operand, order)		\
	__atomic_fetch_and(&(object)->__val, operand, order)
# define atomic_fetch_or_explicit(object, operand, order)		\
	__atomic_fetch_or(&(object)->__val, operand, order)
# define atomic_fetch_sub_explicit(object, operand, order)		\
	__atomic_fetch_sub(&(object)->__val, operand, order)
# define atomic_fetch_xor_explicit(object, operand, order)		\
	__atomic_fetch_xor(&(object)->__val, operand, order)
# define atomic_load_explicit(object, order)				\
	__atomic_load_n(&(object)->__val, order)
# define atomic_store_explicit(object, desired, order)			\
	__atomic_store_n(&(object)->__val, desired, order)
#endif

/*
 * 7.17.8 Atomic flag type and operations.
 */

#ifdef __CLANG_ATOMICS
typedef struct {
	atomic_bool __flag;
} atomic_flag;
#else
typedef struct {
	_Bool volatile __flag;
} atomic_flag;
#endif

#define ATOMIC_FLAG_INIT	{ 0 }

#ifdef __CLANG_ATOMICS
# define atomic_flag_test_and_set_explicit(object, order)		\
	__c11_atomic_exchange(&(object)->__flag, 1, order)
# define atomic_flag_clear_explicit(object, order)			\
	__c11_atomic_store(object, 0, order)
#elif defined(__GNUC_ATOMICS)
# define atomic_flag_test_and_set_explicit(object, order)		\
	__atomic_test_and_set(&(object)->__flag, order)
# define atomic_flag_clear_explicit(object, order)			\
	__atomic_clear(&(object)->__flag, order)
#endif

#ifdef __ASM_ATOMICS
# include <atomickit/arch/atomic.h>
#endif

/*
 * Convenience functions.
 */

#define atomic_compare_exchange_strong(object, expected, desired)	   \
	atomic_compare_exchange_strong_explicit(object, expected, desired, \
	                                        memory_order_seq_cst,	   \
	                                        memory_order_seq_cst)
#define atomic_compare_exchange_weak(object, expected, desired)		 \
	atomic_compare_exchange_weak_explicit(object, expected, desired, \
	                                      memory_order_seq_cst,	 \
	                                      memory_order_seq_cst)
#define atomic_exchange(object, desired)				\
	atomic_exchange_explicit(object, desired, memory_order_seq_cst)
#define atomic_fetch_add(object, operand)				 \
	atomic_fetch_add_explicit(object, operand, memory_order_seq_cst)
#define atomic_fetch_and(object, operand)				 \
	atomic_fetch_and_explicit(object, operand, memory_order_seq_cst)
#define atomic_fetch_or(object, operand)				\
	atomic_fetch_or_explicit(object, operand, memory_order_seq_cst)
#define atomic_fetch_sub(object, operand)				 \
	atomic_fetch_sub_explicit(object, operand, memory_order_seq_cst)
#define atomic_fetch_xor(object, operand)				 \
	atomic_fetch_xor_explicit(object, operand, memory_order_seq_cst)
#define atomic_load(object)						\
	atomic_load_explicit(object, memory_order_seq_cst)
#define atomic_store(object, desired)					\
	atomic_store_explicit(object, desired, memory_order_seq_cst)
#define atomic_flag_test_and_set(object)				\
	atomic_flag_test_and_set_explicit(object, memory_order_seq_cst)
#define atomic_flag_clear(object)					\
	atomic_flag_clear_explicit(object, memory_order_seq_cst)

#else /* defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L */
# include <stdatomic.h>
#endif /* !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L */

#ifndef likely
/**
 * Marks the value as likely to be logically true.
 */
# define likely(x)	__builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
/**
 * Marks the value as likely to be logically false.
 */
# define unlikely(x)	__builtin_expect(!!(x), 0)
#endif

#include <atomickit/arch/misc.h>

#endif /* ! ATOMICKIT_ATOMIC_H */
