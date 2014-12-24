/** @file atomickit/arch/atomic.h
 * atomic.h
 *
 * Compatibility functions for versions of C without C11 atomics.
 *
 * This file is a redaction of arch/x86/include/arch/cmpxchg.h and some other
 * files from the Linux Kernel. In the future, someone should make similar
 * files for other architectures. See the Linux documentation for more
 * information.
 */
/*
 * Copyright 2014 Evan Buswell
 * Copyright 2012 Linus Torvalds et al.
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

#ifndef ATOMICKIT_ATOMIC_H
# error atomickit/arch/atomic.h should not be included directly; include atomickit/atomic.h instead
#endif

#ifndef ATOMICKIT_ARCH_ATOMIC_H 1
#define ATOMICKIT_ARCH_ATOMIC_H 1

#define ATOMIC_BOOL_LOCK_FREE		1
#define ATOMIC_CHAR_LOCK_FREE		1
#define ATOMIC_WCHAR_T_LOCK_FREE	1
#define ATOMIC_SHORT_LOCK_FREE		1
#define ATOMIC_INT_LOCK_FREE		1
#define ATOMIC_LONG_LOCK_FREE		1
#define ATOMIC_LLONG_LOCK_FREE		1
#define ATOMIC_POINTER_LOCK_FREE	1

#if (!defined(__GNUC__) || __GNUC__ < 4 ||   \
     (__GNUC__ == 4 && __GNUC_MINOR < 3)) && \
    (!__has_attribute(__error__))
# define __error__(x)
#endif

/*
 * Non-existant functions to indicate usage errors at link time
 * (or compile-time if the compiler implements __compiletime_error().
 */
extern void __AK_wrong_size(void)
	__attribute__((__error__("Bad argument size")));

/* void */
#define atomic_thread_fence(/* memory_order */ order)			\
({									\
	switch (order) {						\
	case memory_order_relaxed:					\
		break;							\
	case memory_order_consume:					\
	case memory_order_acquire:					\
	case memory_order_release:					\
	case memory_order_acq_rel:					\
		__asm__ __volatile__("": : :"memory");			\
		break;							\
	case memory_order_seq_cst:					\
	default:							\
		__asm__ __volatile__("mfence": : :"memory");		\
	}								\
})

/* void */
#define atomic_signal_fence(/* memory_order */ order)			\
({									\
	switch (order) {						\
	case memory_order_relaxed:					\
		break;							\
	case memory_order_consume:					\
	case memory_order_acquire:					\
	case memory_order_release:					\
	case memory_order_acq_rel:					\
	case memory_order_seq_cst:					\
	default:							\
		__asm__ __volatile__("": : :"memory");			\
	}								\
})

/* _Bool */
#define atomic_is_lock_free(/* const volatile A * */ obj)		\
	((void) (obj), 1)

/*
 * Constants for operation sizes.
 */
#define __X86_CASE_B	1
#define __X86_CASE_W	2
#define __X86_CASE_L	4
#if UINT32_MAX == UINTPTR_MAX
/* 32 bit */
# define __X86_CASE_Q	-1
# define __X86_CASE_DL	8
# define __X86_CASE_DQ	-1
#else
# define __X86_CASE_Q	8
# define __X86_CASE_DL	-1
# define __X86_CASE_DQ	16
#endif

#define __atomic_store_lock(object, desired)				\
({									\
	switch (sizeof((object)->__val)) {				\
	case __X86_CASE_B:						\
	{								\
		volatile uint8_t *__ptr =				\
			(volatile uint8_t *) &(object)->__val;		\
		register volatile uint8_t dummy;			\
		__asm__ __volatile__("xchgb %b1,%0"			\
				     : "=m" (*__ptr), "=q" (dummy)	\
				     : "1" (desired)			\
				     : "memory");			\
		break;							\
	}								\
	case __X86_CASE_W:						\
	{								\
		volatile uint16_t *__ptr =				\
			(volatile uint16_t *) &(object)->__val;		\
		register volatile uint16_t dummy;			\
		__asm__ __volatile__("xchgw %w1,%0"			\
				     : "=m" (*__ptr), "=r" (dummy)	\
				     : "1" (desired)			\
				     : "memory");			\
		break;							\
	}								\
	case __X86_CASE_L:						\
	{								\
		volatile uint32_t *__ptr =				\
			(volatile uint32_t *) &(object)->__val;		\
		register volatile uint32_t dummy;			\
		__asm__ __volatile__("xchgl %1,%0"			\
				     : "=m" (*__ptr), "=r" (dummy)	\
				     : "1" (desired)			\
				     : "memory");			\
		break;							\
	}								\
	case __X86_CASE_Q:						\
	{								\
		volatile uint64_t *__ptr =				\
			(volatile uint64_t *) &(object)->__val;		\
		register volatile uint64_t dummy;			\
		__asm__ __volatile__("xchgq %q1,%0"			\
				     : "=m" (*__ptr), "=r" (dummy)	\
				     : "1" (desired)			\
				     : "memory");			\
		break;							\
	}								\
	default:							\
		__AK_wrong_size();					\
	}								\
})

#define __atomic_store(object, desired)					\
({									\
	switch (sizeof((object)->__val)) {				\
	case __X86_CASE_B:						\
	{								\
		volatile uint8_t *__ptr =				\
			(volatile uint8_t *) &(object)->__val;		\
		__asm__ __volatile__("movb %b1,%0"			\
				     : "=m" (*__ptr)			\
				     : "q" (desired)			\
				     : "memory");			\
		break;							\
	}								\
	case __X86_CASE_W:						\
	{								\
		volatile uint16_t *__ptr =				\
			(volatile uint16_t *) &(object)->__val;		\
		__asm__ __volatile__("movw %w1,%0"			\
				     : "=m" (*__ptr)			\
				     : "r" (desired)			\
				     : "memory");			\
		break;							\
	}								\
	case __X86_CASE_L:						\
	{								\
		volatile uint32_t *__ptr =				\
			(volatile uint32_t *) &(object)->__val;		\
		__asm__ __volatile__("movl %1,%0"			\
				     : "=m" (*__ptr)			\
				     : "r" (desired)			\
				     : "memory");			\
		break;							\
	}								\
	case __X86_CASE_Q:						\
	{								\
		volatile uint64_t *__ptr =				\
			(volatile uint64_t *) &(object)->__val;		\
		__asm__ __volatile__("movq %q1,%0"			\
				     : "=m" (*__ptr)			\
				     : "r" (desired)			\
				     : "memory");			\
		break;							\
	}								\
	default:							\
		__AK_wrong_size();					\
	}								\
})

/* void */
#define atomic_store_explicit(/* volatile A * */ object,		\
			      /* C */ desired,				\
			      /* memory_order */ order)			\
({									\
	switch (order) {						\
	case memory_order_relaxed:					\
	{								\
		/* This is the same as __atomic_store, but avoids the	\
		 * compiler barrier */					\
		volatile __typeof__(object) __v = (object);		\
		__v->__val = desired;					\
		break;							\
	}								\
	case memory_order_consume:					\
	case memory_order_acquire:					\
	case memory_order_release:					\
	case memory_order_acq_rel:					\
		__atomic_store((object), (desired));			\
		break;							\
	case memory_order_seq_cst:					\
	default:							\
		__atomic_store_lock((object), (desired));		\
	}								\
})

#define __atomic_load_lock(object)					\
({									\
	__typeof__((object)->__val) __ret;				\
	switch (sizeof((object)->__val)) {				\
	case __X86_CASE_B:						\
	{								\
		volatile uint8_t *__ptr =				\
			(volatile uint8_t *) &(object)->__val;		\
		__asm__ __volatile__("lock; xaddb %b0, %1"		\
				     : "=q" (__ret), "+m" (*__ptr)	\
				     : "0" (0)				\
				     : "memory", "cc");			\
		break;							\
	}								\
	case __X86_CASE_W:						\
	{								\
		volatile uint16_t *__ptr =				\
			(volatile uint16_t *) &(object)->__val;		\
		__asm__ __volatile__("lock; xaddw %w0, %1"		\
				     : "=r" (__ret), "+m" (*__ptr)	\
				     : "0" (0)				\
				     : "memory", "cc");			\
		break;							\
	}								\
	case __X86_CASE_L:						\
	{								\
		volatile uint32_t *__ptr =				\
			(volatile uint32_t *) &(object)->__val;		\
		__asm__ __volatile__("lock; xaddl %0, %1"		\
				     : "=r" (__ret), "+m" (*__ptr)	\
				     : "0" (0)				\
				     : "memory", "cc");			\
		break;							\
	}								\
	case __X86_CASE_Q:						\
	{								\
		volatile uint64_t *__ptr =				\
			(volatile uint64_t *) &(object)->__val;		\
		__asm__ __volatile__("lock; xaddq %q0, %1"		\
				     : "=r" (__ret), "+m" (*__ptr)	\
				     : "0" (0)				\
				     : "memory", "cc");			\
		break;							\
	}								\
	default:							\
		__AK_wrong_size();					\
	}								\
	__ret;								\
})

#define __atomic_load(object)						\
({									\
	__typeof__((object)->__val) __ret;				\
	switch (sizeof((object)->__val)) {				\
	case __X86_CASE_B:						\
	{								\
		volatile uint8_t *__ptr =				\
			(volatile uint8_t *) &(object)->__val;		\
		__asm__ __volatile__("movb %1,%b0"			\
				     : "=q" (__ret)			\
				     : "m" (*__ptr)			\
				     : "memory");			\
		break;							\
	}								\
	case __X86_CASE_W:						\
	{								\
		volatile uint16_t *__ptr =				\
			(volatile uint16_t *)&(object)->__val;		\
		__asm__ __volatile__("movw %1,%w0"			\
				     : "=r" (__ret)			\
				     : "m" (*__ptr)			\
				     : "memory");			\
		break;							\
	}								\
	case __X86_CASE_L:						\
	{								\
		volatile uint32_t *__ptr =				\
			(volatile uint32_t *) &(object)->__val;		\
		__asm__ __volatile__("movl %1,%0"			\
				     : "=r" (__ret)			\
				     : "m" (*__ptr)			\
				     : "memory");			\
		break;							\
	}								\
	case __X86_CASE_Q:						\
	{								\
		volatile uint64_t *__ptr =				\
			(volatile uint64_t *) &(object)->__val;		\
		__asm__ __volatile__("movq %1,%q0"			\
				     : "=r" (__ret)			\
				     : "m" (*__ptr)			\
				     : "memory");			\
		break;							\
	}								\
	default:							\
		__AK_wrong_size();					\
	}								\
	__ret;								\
})

/* C */
#define atomic_load_explicit(/* volatile A * */ object,			\
			     /* memory_order */ order)			\
({									\
	__typeof__((object)->__val) __ret2;				\
	switch (order) {						\
	case memory_order_relaxed:					\
	{								\
		/* This is the same as __atomic_load, but avoids the	\
		 * compiler barrier */					\
		volatile __typeof__(object) __v = (object);		\
		__ret2 = __v->__val;					\
		break;							\
	}								\
	case memory_order_consume:					\
	case memory_order_acquire:					\
	case memory_order_release:					\
	case memory_order_acq_rel:					\
		__ret2 = __atomic_load(object);				\
		break;							\
	case memory_order_seq_cst:					\
	default:							\
		__ret2 = __atomic_load_lock(object);			\
	}								\
	__ret2;								\
})

/*
 * Note: no "lock" prefix: xchg always implies lock anyway. Since this is
 * generally used to protect other memory information, we use "asm volatile"
 * and "memory" clobbers to prevent gcc from moving information around.
 */
#define __atomic_exchange(object, desired)				\
({									\
	__typeof__((object)->__val) __ret = (desired);			\
	switch (sizeof((object)->__val)) {				\
	case __X86_CASE_B:						\
	{								\
		volatile uint8_t *__ptr =				\
			(volatile uint8_t *) &(object)->__val;		\
		__asm__ __volatile__("xchgb %b0,%1"			\
				     : "=q" (__ret), "+m" (*__ptr)	\
				     : "0" (__ret)			\
				     : "memory");			\
		break;							\
	}								\
	case __X86_CASE_W:						\
	{								\
		volatile uint16_t *__ptr =				\
			(volatile uint16_t *) &(object)->__val;		\
		__asm__ __volatile__("xchgw %w0,%1"			\
				     : "=r" (__ret), "+m" (*__ptr)	\
				     : "0" (__ret)			\
				     : "memory");			\
		break;							\
	}								\
	case __X86_CASE_L:						\
	{								\
		volatile uint32_t *__ptr =				\
			(volatile uint32_t *) &(object)->__val;		\
		__asm__ __volatile__("xchgl %0,%1"			\
				     : "=r" (__ret), "+m" (*__ptr)	\
				     : "0" (__ret)			\
				     : "memory");			\
		break;							\
	}								\
	case __X86_CASE_Q:						\
	{								\
		volatile uint64_t *__ptr =				\
			(volatile uint64_t *) &(object)->__val;		\
		__asm__ __volatile__("xchgq %q0,%1"			\
				     : "=r" (__ret), "+m" (*__ptr)	\
				     : "0" (__ret)			\
				     : "memory");			\
		break;							\
	}								\
	default:							\
		__AK_wrong_size();					\
	}								\
	__ret;								\
})

/* C */
#define atomic_exchange_explicit(/* volatile A * */ object,		\
				 /* C */ desired,			\
				 /* memory_order */ order)		\
({									\
	__typeof__((object)->__val) __ret2;				\
	switch (order) {						\
	case memory_order_relaxed:					\
	case memory_order_consume:					\
	case memory_order_acquire:					\
	case memory_order_release:					\
	case memory_order_acq_rel:					\
	case memory_order_seq_cst:					\
	default:							\
		__ret2 = __atomic_exchange((object), (desired));	\
	}								\
	__ret2;								\
})

/* C */

/*
 * Atomic compare and exchange.  Compare EXPECTED with OBJECT, if identical,
 * store DESIRED in OBJECT.  Set EXPECTED to the initial value in OBJECT.
 * Return true on success.
 */
#define __atomic_compare_exchange(object, expected, desired, lock)	\
({									\
	_Bool __ret;							\
	__typeof__((object)->__val) *__old = (expected);		\
	__typeof__((object)->__val) __new = (desired);			\
	switch (sizeof((object)->__val)) {				\
	case __X86_CASE_B:						\
	{								\
		volatile uint8_t *__ptr =				\
			(volatile uint8_t *) &(object)->__val;		\
		__asm__ __volatile__(lock "cmpxchgb %b3,%1; setz %2"	\
				     : "=a" (*__old), "+m" (*__ptr),	\
				       "=r" (__ret)			\
				     : "q" (__new), "0" (*__old)	\
				     : "memory", "cc");			\
		break;							\
	}								\
	case __X86_CASE_W:						\
	{								\
		volatile uint16_t *__ptr =				\
			(volatile uint16_t *) &(object)->__val;		\
		__asm__ __volatile__(lock "cmpxchgw %w3,%1; setz %2"	\
				     : "=a" (*__old), "+m" (*__ptr),	\
				       "=r" (__ret)			\
				     : "r" (__new), "0" (*__old)	\
				     : "memory", "cc");			\
		break;							\
	}								\
	case __X86_CASE_L:						\
	{								\
		volatile uint32_t *__ptr =				\
			(volatile uint32_t *) &(object)->__val;		\
		__asm__ __volatile__(lock "cmpxchgl %3,%1; setz %2"	\
				     : "=a" (*__old), "+m" (*__ptr),	\
				       "=r" (__ret)			\
				     : "r" (__new), "0" (*__old)	\
				     : "memory", "cc");			\
		break;							\
	}								\
	case __X86_CASE_Q:						\
	{								\
		volatile uint64_t *__ptr =				\
			(volatile uint64_t *) &(object)->__val;		\
		__asm__ __volatile__(lock "cmpxchgq %q3,%1; setz %2"	\
				     : "=a" (*__old), "+m" (*__ptr),	\
				       "=r" (__ret)			\
				     : "r" (__new), "0" (*__old)	\
				     : "memory", "cc");			\
		break;							\
	}								\
	case __X86_CASE_DL:						\
	{								\
		union {							\
			__typeof__((object)->__val) *o;			\
			uint64_t *i;					\
		} __old2 = { .o = __old };				\
		union {							\
			__typeof__((object)->__val) o;			\
 		       	uint64_t i;					\
		} __new2 = { .o = __new };				\
#ifdef __PIC__								\
		volatile uint64_t *__ptr =				\
			(volatile uint64_t *) &(object)->__val;		\
		__asm__ __volatile__("xchgl %2, %%ebx;"			\
				     lock "cmpxchg8b %1;"		\
				     "movl %2, %%ebx;"			\
				     "setz %2"				\
				     : "=A" (*__old2.i), "+m" (*__ptr),	\
				       "=r" (__ret)			\
				     : "2" ((uint32_t) __new2.i),	\
				       "c" ((uint32_t) __new2.i >> 32)	\
				       "0" (*__old2.i)			\
				     : "memory", "cc");			\
#else									\
		volatile uint64_t *__ptr =				\
			(volatile uint64_t *) &(object)->__val;		\
		__asm__ __volatile__(lock "cmpxchg8b %1; setz %2"	\
				     : "=A" (*__old2.i), "+m" (*__ptr),	\
				       "=r" (__ret)			\
				     : "b" ((uint32_t) __new2.i),	\
				       "c" ((uint32_t) __new2.i >> 32)	\
				       "0" (*__old2.i)			\
				     : "memory", "cc");			\
#endif									\
		break;							\
	}								\
	case __X86_CASE_DQ:						\
	{								\
		union {							\
			__typeof__((object)->__val) *o;			\
			__uint128_t *i;					\
		} __old2 = { .o = __old };				\
		union {							\
			__typeof__((object)->__val) o;			\
 		       	__uint128_t i;					\
		} __new2 = { .o = __new };				\
#ifdef __PIC__								\
		volatile uint64_t *__ptr =				\
			(volatile __uint128_t *) &(object)->__val;	\
		__asm__ __volatile__("xchgq %2, %%rbx;"			\
				     lock "cmpxchg16b %1;"		\
				     "movq %2, %%rbx;"			\
				     "setz %2"				\
				     : "=A" (*__old2.i), "+m" (*__ptr),	\
				       "=r" (__ret)			\
				     : "2" ((uint64_t) __new2.i),	\
				       "c" ((uint64_t) __new2.i >> 64)	\
				       "0" (*__old2.i)			\
				     : "memory", "cc");			\
#else									\
		volatile __uint128_t *__ptr =				\
			(volatile __uint128_t *) &(object)->__val;	\
		__asm__ __volatile__(lock "cmpxchg16b %1; setz %2"	\
				     : "=A" (*__old2.i), "+m" (*__ptr),	\
				       "=r" (__ret)			\
				     : "b" ((uint64_t) __new2.i),	\
				       "c" ((uint64_t) __new2.i >> 64)	\
				       "0" (*__old2.i)			\
				     : "memory", "cc");			\
#endif									\
		break;							\
	}								\
	default:							\
		__AK_wrong_size();					\
	}								\
	__ret;								\
})

/* _Bool */
#define atomic_compare_exchange_strong_explicit(/* volatile A * */ object,  \
						/* C * */ expected,	    \
						/* C */ desired,	    \
						/* memory_order */ success, \
						/* memory_order */ failure) \
({									    \
	_Bool __ret2;							    \
	switch ((success)) {						    \
	case memory_order_relaxed:					    \
	case memory_order_consume:					    \
	case memory_order_acquire:					    \
	case memory_order_release:					    \
	case memory_order_acq_rel:					    \
		if ((failure) == memory_order_seq_cst) {		    \
			__typeof__(expected) __expected = (expected);	    \
			__typeof__(object) __object = (object);		    \
			__ret2 = __atomic_compare_exchange(		    \
				__object, __expected, (desired), "");	    \
			if (!__ret2) {					    \
				*__expected = __atomic_load_lock(__object); \
			}						    \
		} else {						    \
			__ret2 = __atomic_compare_exchange(		    \
				(object), (expected), desired), "");	    \
		}							    \
	break;								    \
	case memory_order_seq_cst:					    \
	default:							    \
		(void) failure;						    \
		__ret2 = __atomic_compare_exchange(			    \
			(object), (expected), (desired), "lock;");	    \
	}								    \
	__ret2;								    \
})

/* _Bool */
#define atomic_compare_exchange_weak_explicit(/* volatile A * */ object,  \
					      /* C * */ expected,	  \
					      /* C */ desired,		  \
					      /* memory_order */ success, \
					      /* memory_order */ failure) \
	atomic_compare_exchange_strong_explicit((object), (expected),	  \
						(desired), (success),	  \
						(failure))

#define __atomic_fetch_add(object, operand, lock)			\
({									\
	__typeof__(operand) __operand = (operand);			\
	__typeof__((object)->__val) __ret = (operand);			\
	switch (sizeof((object)->__val)) {				\
	case __X86_CASE_B:						\
	{								\
		volatile uint8_t *__ptr =				\
			(volatile uint8_t *) &(object)->__val;		\
		__asm__ __volatile__ (lock "xaddb %b0, %1"		\
				      : "=q" (__ret), "+m" (*__ptr)	\
				      : "0" (__operand)			\
				      : "memory", "cc");		\
		break;							\
	}								\
	case __X86_CASE_W:						\
	{								\
		volatile uint16_t *__ptr =				\
			(volatile uint16_t *)&(object)->__val;		\
		__asm__ __volatile__ (lock "xaddw %w0, %1"		\
				      : "=r" (__ret), "+m" (*__ptr)	\
				      : "0" (__operand)			\
				      : "memory", "cc");		\
		break;							\
	}								\
	case __X86_CASE_L:						\
	{								\
		volatile uint32_t *__ptr =				\
			(volatile uint32_t *)&(object)->__val;		\
		__asm__ __volatile__ (lock "xaddl %0, %1"		\
				      : "=r" (__ret), "+m" (*__ptr)	\
				      : "0" (__operand)			\
				      : "memory", "cc");		\
		break;							\
	}								\
	case __X86_CASE_Q:						\
	{								\
		volatile uint64_t *__ptr =				\
			(volatile uint64_t *)&(object)->__val;		\
		__asm__ __volatile__ (lock "xaddq %q0, %1"		\
				      : "=r" (__ret), "+m" (*__ptr)	\
				      : "0" (__operand)			\
				      : "memory", "cc");		\
		break;							\
	}								\
	default:							\
		__AK_wrong_size();					\
	}								\
	__ret;								\
})


/* C */
#define atomic_fetch_add_explicit(/* volatile A * */ object,		\
				  /* M */ operand,			\
				  /* memory_order */ order)		\
({									\
	__typeof__((object)->__val) __ret2;				\
	switch (order) {						\
	case memory_order_relaxed:					\
	case memory_order_consume:					\
	case memory_order_acquire:					\
	case memory_order_release:					\
	case memory_order_acq_rel:					\
		__ret2 = __atomic_fetch_add((object), (operand), "");	\
		break;							\
	case memory_order_seq_cst:					\
	default:							\
		__ret2 = __atomic_fetch_add((object), (operand),	\
					    "lock;");			\
	}								\
	__ret2;								\
})

/* C */
#define atomic_fetch_sub_explicit(/* volatile A * */ object,		\
				  /* M */ operand,			\
				  /* memory_order */ order)		\
	atomic_fetch_add_explicit((object), -(operand), (order))

/* C */
#define atomic_fetch_or_explicit(/* volatile A * */ object,		\
				 /* M */ operand,			\
				 /* memory_order */ order)		\
({									\
	__typeof__(object) __obj3 = (object);				\
	__typeof__(operand) __operand = (operand);			\
	memory_order __order = (order);					\
	__typeof__(__obj3->__val) __ret3 =				\
		atomic_load_explicit(__obj3, memory_order_relaxed);	\
	__typeof__(__obj3->__val) __new3;				\
	do {								\
		__new3 = __ret3 | __operand;				\
	} while (unlikely(						\
		!atomic_compare_exchange_weak_explicit(			\
			__obj3, &__ret3, __new3, __order,		\
			memory_order_relaxed)));			\
	__ret3;								\
})

/* C */
#define atomic_fetch_xor_explicit(/* volatile A * */ object,		\
				  /* M */ operand,			\
				  /* memory_order */ order)		\
({									\
	__typeof__(object) __obj3 = (object);				\
	__typeof__(operand) __operand = (operand);			\
	memory_order __order = (order);					\
	__typeof__(__obj3->__val) __ret3 =				\
		atomic_load_explicit(__obj3, memory_order_relaxed);	\
	__typeof__(__obj3->__val) __new3;				\
	do {								\
		__new3 = __ret3 ^ __operand;				\
	} while (unlikely(						\
		!atomic_compare_exchange_weak_explicit(			\
			__obj3, &__ret3, __new3, __order,		\
			memory_order_relaxed)));			\
	__ret3;								\
})

/* C */
#define atomic_fetch_and_explicit(/* volatile A * */ object,		\
				  /* M */ operand,			\
				  /* memory_order */ order)		\
({									\
	__typeof__(object) __obj3 = (object);				\
	__typeof__(operand) __operand = (operand);			\
	memory_order __order = (order);					\
	__typeof__(__obj3->__val) __ret3 =				\
		atomic_load_explicit(__obj3, memory_order_relaxed);	\
	__typeof__(__obj3->__val) __new3;				\
	do {								\
		__new3 = __ret3 & __operand;				\
	} while (unlikely(						\
		!atomic_compare_exchange_weak_explicit(			\
			__obj3, &__ret3, __new3, __order,		\
			memory_order_relaxed)));			\
	__ret3;								\
})

#define __atomic_flag_test_and_set(object, lock)			\
({									\
	_Bool __ret;							\
	switch (sizeof(_Bool)) {					\
	case __X86_CASE_B:						\
	{								\
		volatile uint8_t *__ptr =				\
			(volatile uint8_t *)&(object)->__flag;		\
		__asm__ __volatile__ (lock "btsb $0, %1; setc %0"	\
				      : "=r" (__ret), "+m" (*__ptr)	\
				      : : "memory", "cc");		\
		break;							\
	}								\
	case __X86_CASE_W:						\
	{								\
		volatile uint16_t *__ptr =				\
			(volatile uint16_t *)&(object)->__flag;		\
		__asm__ __volatile__ (lock "btsw $0, %1; setc %0"	\
				      : "=r" (__ret), "+m" (*__ptr)	\
				      : : "memory", "cc");		\
		break;							\
	}								\
	case __X86_CASE_L:						\
	{								\
		volatile uint32_t *__ptr =				\
			(volatile uint32_t *)&(object)->__flag;		\
		__asm__ __volatile__ (lock "btsl $0, %1; setc %0"	\
				      : "=r" (__ret), "+m" (*__ptr)	\
				      : : "memory", "cc");		\
		break;							\
	}								\
	case __X86_CASE_Q:						\
	{								\
		volatile uint64_t *__ptr =				\
			(volatile uint64_t *)&(object)->__flag;		\
		__asm__ __volatile__ (lock "btsq $0, %1; setc %0"	\
				      : "=r" (__ret), "+m" (*__ptr)	\
				      : : "memory", "cc");		\
		break;							\
	}								\
	default:							\
		__AK_wrong_size();					\
	}								\
	__ret;								\
})

/* _Bool */
#define atomic_flag_test_and_set_explicit(				\
	/* volatile atomic_flag * */ object,				\
	/* memory_order */ order)					\
({									\
	_Bool __ret2;							\
	switch (order) {						\
	case memory_order_relaxed:					\
	case memory_order_consume:					\
	case memory_order_acquire:					\
	case memory_order_release:					\
	case memory_order_acq_rel:					\
		__ret2 = __atomic_flag_test_and_set((object), "");	\
		break;							\
	case memory_order_seq_cst:					\
	default:							\
		__ret2 = __atomic_flag_test_and_set((object), "lock;");	\
	}								\
	__ret2;								\
})

#define __atomic_flag_clear(object, lock)				\
({									\
	switch (sizeof(_Bool)) {					\
	case __X86_CASE_B:						\
	{								\
		volatile uint8_t *__ptr =				\
			(volatile uint8_t *)&(object)->__flag;		\
		__asm__ __volatile__ (lock "andb $0, %0"		\
				      : "+m" (*__ptr)			\
				      : : "memory", "cc");		\
	}								\
	case __X86_CASE_W:						\
	{								\
		volatile uint16_t *__ptr =				\
			(volatile uint16_t *)&(object)->__flag;		\
		__asm__ __volatile__ (lock "andw $0, %0"		\
				      : "+m" (*__ptr)			\
				      : : "memory", "cc");		\
	}								\
	case __X86_CASE_L:						\
	{								\
		volatile uint32_t *__ptr =				\
			(volatile uint32_t *)&(object)->__flag;		\
		__asm__ __volatile__ (lock "andl $0, %0"		\
				      : "+m" (*__ptr)			\
				      : : "memory", "cc");		\
	}								\
	case __X86_CASE_Q:						\
	{								\
		volatile uint64_t *__ptr =				\
			(volatile uint64_t *)&(object)->__flag;		\
		__asm__ __volatile__ (lock "andq $0, %0"		\
				      : "+m" (*__ptr)			\
				      : : "memory", "cc");		\
	}								\
	default:							\
		__AK_wrong_size();					\
	}								\
})

/* void */
#define atomic_flag_clear_explicit(/* volatile atomic_flag * */ object,	\
				   /* memory_order */ order)		\
({									\
	switch (order) {						\
	case memory_order_relaxed:					\
	case memory_order_consume:					\
	case memory_order_acquire:					\
	case memory_order_release:					\
	case memory_order_acq_rel:					\
		__atomic_flag_clear((object), "");			\
		break;							\
	case memory_order_seq_cst:					\
	default:							\
		__atomic_flag_clear((object), "lock;");			\
	}								\
})

/*
 * Atomic double compare and exchange.  Compare EXPECTED with OBJECT, if
 * identical, store DESIRED in OBJECT.  Set EXPECTED to the initial value in
 * OBJECT. Return true on success.
 */
#define __ak_dcas(object, expected, desired, lock)			\
({									\
	_Bool __ret;							\
 	__typeof__((object)->__val) *__old = (expected);		\
	union {								\
		__typeof__((object)->__val) dval;			\
		struct {						\
			uintptr_t lo;					\
			uintptr_t hi;					\
		} dsplit;						\
	} __new = { .dval = (desired) };				\
	switch (sizeof(uintptr_t)) {					\
	case __X86_CASE_L:						\
	{								\
		volatile uint64_t *__ptr =				\
			(volatile uint64_t *) &(object)->__val;		\
		__asm__ __volatile__(lock "cmpxchg8b %1; setz %2"	\
				     : "=A" (*__old), "+m" (*__ptr),	\
				       "=r" (__ret)			\
				     : "b" (__new.lo), "c" (__new.hi),	\
				       "0" (*__old)			\
				     : "memory", "cc");			\
		break;							\
	}								\
	case __X86_CASE_Q:						\
	{								\
		volatile __int128_t *__ptr =				\
			(volatile __int128_t *) &(object)->__val;	\
		__asm__ __volatile__(lock "cmpxchg16b %1; setz %2"	\
				     : "=A" (*__old), "+m" (*__ptr),	\
				       "=r" (__ret)			\
				     : "b" (__new.lo), "c" (__new.hi),	\
				       "0" (*__old)			\
				     : "memory", "cc");			\
		break;							\
	}								\
	default:							\
		__AK_wrong_size();					\
	}								\
	__ret;								\
})

/* _Bool */
#define ak_dcas(/* volatile A * */ object,				\
		/* C * */ expected, /* C */ desired,			\
		/* memory_order */ success, /* memory_order */ failure) \
({									\
	_Bool __ret2;							\
	switch ((success)) {						\
	case memory_order_relaxed:					\
	case memory_order_consume:					\
	case memory_order_acquire:					\
	case memory_order_release:					\
	case memory_order_acq_rel:					\
		__ret2 = __ak_dcas((object), (expected), desired), "");	\
		break;							\
	case memory_order_seq_cst:					\
	default:							\
		__ret2 = __ak_dcas((object), (expected), desired), "lock"); \
		break;							\
	}								\
	__ret2;								\
})

/* _Bool */
#define ak_dcas_strong(/* volatile A * */ object, /* C * */ expected,	\
		       /* C */ desired, /* memory_order */ success,	\
		       /* memory_order */ failure)			\
	ak_dcas((object), (expected), (desired), (success), (failure))

#endif /* ! ATOMICKIT_ARCH_ATOMIC_H */
