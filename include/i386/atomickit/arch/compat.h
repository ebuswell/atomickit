/** @file compat.h
 * compat.h
 *
 * Compatibility functions for versions of C without C11 atomics.
 *
 * This file is a redaction of arch/x86/include/arch/cmpxchg.h and
 * some other files from the Linux Kernel.  In the future, someone
 * should make similar files for other architectures and compilers.
 * For now, though, we depend on x86(_64) and gcc.  See the Linux
 * documentation for more information.
 */
/*
 * Copyright 2013 Evan Buswell
 * Copyright 2012 Linus Torvalds et al.
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

#ifndef ATOMICKIT_ATOMIC_H
# error atomickit/arch/compat.h should not be included directly; include atomickit/atomic.h instead
#endif

#ifndef ATOMICKIT_ARCH_COMPAT_H
#define ATOMICKIT_ARCH_COMPAT_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ATOMIC_BOOL_LOCK_FREE true
#define ATOMIC_CHAR_LOCK_FREE true
#define ATOMIC_WCHAR_T_LOCK_FREE true
#define ATOMIC_SHORT_LOCK_FREE true
#define ATOMIC_INT_LOCK_FREE true
#define ATOMIC_LONG_LOCK_FREE true
#define ATOMIC_LLONG_LOCK_FREE true
#define ATOMIC_POINTER_LOCK_FREE true

#define ATOMIC_FLAG_INIT { 0 }

typedef enum {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
} memory_order;

typedef struct {
    uint16_t counter;
} atomic_flag;

#if __GNUC__ < 4 || \
    (__GNUC__ == 4 && (__GNUC_MINOR < 3))
#define __error__(x)
#endif

/*
 * Non-existant functions to indicate usage errors at link time
 * (or compile-time if the compiler implements __compiletime_error().
 */
extern void __AK_wrong_size(void)
    __attribute__((__error__("Bad argument size")));

extern void __AK_64not_implemented(void)
    __attribute__((__error__("64 bit on 32 bit architecture not yet implemented")));

#define ATOMIC_VAR_INIT(/* C */ value) { (value) }

/* void */
#define atomic_init(/* volatile A * */ obj, /* C */ value)\
({							  \
    volatile __typeof__(obj) __ptr = (obj);		  \
    __ptr->counter = (value);				  \
})

/* void */
#define atomic_thread_fence(/* memory_order */ order)	\
({							\
    switch(order) {					\
    case memory_order_relaxed:				\
	break;						\
    case memory_order_consume:				\
    case memory_order_acquire:				\
    case memory_order_release:				\
    case memory_order_acq_rel:				\
	__asm__ __volatile__("": : :"memory");		\
	break;						\
    case memory_order_seq_cst:				\
    default:						\
	__asm__ __volatile__("mfence": : :"memory");	\
    }							\
})

/* void */
#define atomic_signal_fence(/* memory_order */ order)	\
({							\
    switch(order) {					\
    case memory_order_relaxed:				\
	break;						\
    case memory_order_consume:				\
    case memory_order_acquire:				\
    case memory_order_release:				\
    case memory_order_acq_rel:				\
    case memory_order_seq_cst:				\
    default:						\
	__asm__ __volatile__("": : :"memory");		\
    }							\
})

/* _Bool */
#define atomic_is_lock_free(/* const volatile A * */ obj)	\
    ((void) (obj), true)

typedef struct {
    bool counter;
} atomic_bool;

typedef struct {
    char counter;
} atomic_char;

typedef struct {
    signed char counter;
} atomic_schar;

typedef struct {
    unsigned char counter;
} atomic_uchar;

typedef struct {
    short counter;
} atomic_short;

typedef struct {
    unsigned short counter;
} atomic_ushort;

typedef struct {
    int counter;
} atomic_int;

typedef struct {
    unsigned int counter;
} atomic_uint;

typedef struct {
    long counter;
} atomic_long;

typedef struct {
    unsigned long counter;
} atomic_ulong;

typedef struct {
    wchar_t counter;
} atomic_wchar_t;

typedef struct {
    int_least8_t counter;
} atomic_int_least8_t;

typedef struct {
    uint_least8_t counter;
} atomic_uint_least8_t;

typedef struct {
    int_least16_t counter;
} atomic_int_least16_t;

typedef struct {
    uint_least16_t counter;
} atomic_uint_least16_t;

typedef struct {
    int_least32_t counter;
} atomic_int_least32_t;

typedef struct {
    uint_least32_t counter;
} atomic_uint_least32_t;

typedef struct {
    int_least64_t counter;
} atomic_int_least64_t;

typedef struct {
    uint_least64_t counter;
} atomic_uint_least64_t;

typedef struct {
    int_fast8_t counter;
} atomic_int_fast8_t;

typedef struct {
    uint_fast8_t counter;
} atomic_uint_fast8_t;

typedef struct {
    int_fast16_t counter;
} atomic_int_fast16_t;

typedef struct {
    uint_fast16_t counter;
} atomic_uint_fast16_t;

typedef struct {
    int_fast32_t counter;
} atomic_int_fast32_t;

typedef struct {
    uint_fast32_t counter;
} atomic_uint_fast32_t;

typedef struct {
    int_fast64_t counter;
} atomic_int_fast64_t;

typedef struct {
    uint_fast64_t counter;
} atomic_uint_fast64_t;

typedef struct {
    intptr_t counter;
} atomic_intptr_t;

typedef struct {
    uintptr_t counter;
} atomic_uintptr_t;

typedef struct {
    size_t counter;
} atomic_size_t;

typedef struct {
    ptrdiff_t counter;
} atomic_ptrdiff_t;

typedef struct {
    intmax_t counter;
} atomic_intmax_t;

typedef struct {
    uintmax_t counter;
} atomic_uintmax_t;

/*
 * Constants for operation sizes. On 32-bit, the 64-bit size is set to
 * -1 because sizeof will never return -1, thereby making those switch
 * case statements guaranteeed dead code which the compiler will
 * eliminate, and allowing the "missing symbol in the default case" to
 * indicate a usage error.
 */
#define __X86_CASE_B	1
#define __X86_CASE_W	2
#define __X86_CASE_L	4
#if defined __x86_64__
# define __X86_CASE_Q	8
# define __X86_CASE_Q_EMU -1
#else
# define __X86_CASE_Q	-1		/* sizeof will never return -1 */
# define __X86_CASE_Q_EMU 8
#endif

#define __atomic_store_lock(object, desired)				\
({									\
    switch(sizeof((object)->counter)) {					\
    case __X86_CASE_B:							\
    {									\
	volatile uint8_t *__ptr = (volatile uint8_t *)&(object)->counter; \
	register volatile uint8_t dummy;				\
	__asm__ __volatile__("xchgb %b1,%0"				\
			     : "=m" (*__ptr), "=q" (dummy)		\
			     : "1" (desired)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_W:							\
    {									\
	volatile uint16_t *__ptr = (volatile uint16_t *)&(object)->counter; \
	register volatile uint16_t dummy;				\
	__asm__ __volatile__("xchgw %w1,%0"				\
			     : "=m" (*__ptr), "=r" (dummy)		\
			     : "1" (desired)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_L:							\
    {									\
	volatile uint32_t *__ptr = (volatile uint32_t *)&(object)->counter; \
	register volatile uint32_t dummy;				\
	__asm__ __volatile__("xchgl %1,%0"				\
			     : "=m" (*__ptr), "=r" (dummy)		\
			     : "1" (desired)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_Q:							\
    {									\
	volatile uint64_t *__ptr = (volatile uint64_t *)&(object)->counter; \
	register volatile uint64_t dummy;				\
	__asm__ __volatile__("xchgq %q1,%0"				\
			     : "=m" (*__ptr), "=r" (dummy)		\
			     : "1" (desired)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_Q_EMU:						\
	__AK_64not_implemented();					\
	break;								\
    default:								\
	__AK_wrong_size();						\
    }									\
})

#define __atomic_store(object, desired)					\
({									\
    switch(sizeof((object)->counter)) {					\
    case __X86_CASE_B:							\
    {									\
	volatile uint8_t *__ptr = (volatile uint8_t *)&(object)->counter; \
	__asm__ __volatile__("movb %b1,%0"				\
			     : "=m" (*__ptr)				\
			     : "q" (desired)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_W:							\
    {									\
	volatile uint16_t *__ptr = (volatile uint16_t *)&(object)->counter; \
	__asm__ __volatile__("movw %w1,%0"				\
			     : "=m" (*__ptr)				\
			     : "r" (desired)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_L:							\
    {									\
	volatile uint32_t *__ptr = (volatile uint32_t *)&(object)->counter; \
	__asm__ __volatile__("movl %1,%0"				\
			     : "=m" (*__ptr)				\
			     : "r" (desired)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_Q:							\
    {									\
	volatile uint64_t *__ptr = (volatile uint64_t *)&(object)->counter; \
	__asm__ __volatile__("movq %q1,%0"				\
			     : "=m" (*__ptr)				\
			     : "r" (desired)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_Q_EMU:						\
	__AK_64not_implemented();					\
	break;								\
    default:								\
	__AK_wrong_size();						\
    }									\
})

/* void */
#define atomic_store_explicit(/* volatile A * */ object,		\
			      /* C */ desired,				\
			      /* memory_order */ order)			\
({									\
    switch(order) {							\
    case memory_order_relaxed:						\
    {									\
	/* This is the same as __atomic_store, but avoids the compiler	\
	 * barrier */							\
	volatile __typeof__(object) __v = (object);			\
	__v->counter = desired;						\
	break;								\
    }									\
    case memory_order_consume:						\
    case memory_order_acquire:						\
    case memory_order_release:						\
    case memory_order_acq_rel:						\
        __atomic_store((object), (desired));				\
	break;								\
    case memory_order_seq_cst:						\
    default:								\
        __atomic_store_lock((object), (desired));			\
    }									\
})

/* void */
#define atomic_store(/* volatile A * */ object,				\
		     /* C */ desired)					\
    atomic_store_explicit((object), (desired), memory_order_seq_cst)

#define __atomic_load_lock(object)					\
({									\
    __typeof__((object)->counter) __ret;				\
    switch(sizeof((object)->counter)) {					\
    case __X86_CASE_B:							\
    {									\
	volatile uint8_t *__ptr = (volatile uint8_t *)&(object)->counter; \
	__asm__ __volatile__("lock; xaddb %b0, %1"			\
			     : "=q" (__ret), "+m" (*__ptr)		\
			     : "0" (0)					\
			     : "memory", "cc");				\
	break;								\
    }									\
    case __X86_CASE_W:							\
    {									\
	volatile uint16_t *__ptr = (volatile uint16_t *)&(object)->counter; \
	__asm__ __volatile__("lock; xaddw %w0, %1"			\
			     : "=r" (__ret), "+m" (*__ptr)		\
			     : "0" (0)					\
			     : "memory", "cc");				\
	break;								\
    }									\
    case __X86_CASE_L:							\
    {									\
	volatile uint32_t *__ptr = (volatile uint32_t *)&(object)->counter; \
	__asm__ __volatile__("lock; xaddl %0, %1"			\
			     : "=r" (__ret), "+m" (*__ptr)		\
			     : "0" (0)					\
			     : "memory", "cc");				\
	break;								\
    }									\
    case __X86_CASE_Q:							\
    {									\
	volatile uint64_t *__ptr = (volatile uint64_t *)&(object)->counter; \
	__asm__ __volatile__("lock; xaddq %q0, %1"			\
			     : "=r" (__ret), "+m" (*__ptr)		\
			     : "0" (0)					\
			     : "memory", "cc");				\
	break;								\
    }									\
    case __X86_CASE_Q_EMU:						\
	__AK_64not_implemented();					\
	break;								\
    default:								\
	__AK_wrong_size();						\
    }									\
    __ret;								\
})

#define __atomic_load(object)						\
({									\
    __typeof__((object)->counter) __ret;				\
    switch(sizeof((object)->counter)) {					\
    case __X86_CASE_B:							\
    {									\
	volatile uint8_t *__ptr = (volatile uint8_t *)&(object)->counter; \
	__asm__ __volatile__("movb %1,%b0"				\
			     : "=q" (__ret)				\
			     : "m" (*__ptr)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_W:							\
    {									\
	volatile uint16_t *__ptr = (volatile uint16_t *)&(object)->counter; \
	__asm__ __volatile__("movw %1,%w0"				\
			     : "=r" (__ret)				\
			     : "m" (*__ptr)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_L:							\
    {									\
	volatile uint32_t *__ptr = (volatile uint32_t *)&(object)->counter; \
	__asm__ __volatile__("movl %1,%0"				\
			     : "=r" (__ret)				\
			     : "m" (*__ptr)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_Q:							\
    {									\
	volatile uint64_t *__ptr = (volatile uint64_t *)&(object)->counter; \
	__asm__ __volatile__("movq %1,%q0"				\
			     : "=r" (__ret)				\
			     : "m" (*__ptr)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_Q_EMU:						\
	__AK_64not_implemented();					\
	break;								\
    default:								\
	__AK_wrong_size();						\
    }									\
    __ret;								\
})

/* C */
#define atomic_load_explicit(/* volatile A * */ object,			\
			     /* memory_order */ order)			\
({									\
    __typeof__((object)->counter) __ret2;				\
    switch(order) {							\
    case memory_order_relaxed:						\
    {									\
	/* This is the same as __atomic_load, but			\
	 * avoids the compiler barrier */				\
	volatile __typeof__(object) __v = (object);			\
	__ret2 = __v->counter;						\
	break;								\
    }									\
    case memory_order_consume:						\
    case memory_order_acquire:						\
    case memory_order_release:						\
    case memory_order_acq_rel:						\
	__ret2 = __atomic_load(object);					\
	break;								\
    case memory_order_seq_cst:						\
    default:								\
	__ret2 = __atomic_load_lock(object);				\
    }									\
    __ret2;								\
})

/* C */
#define atomic_load(/* volatile A * */ object)			\
    atomic_load_explicit((object), memory_order_seq_cst)


/*
 * Note: no "lock" prefix even on SMP: xchg always implies lock anyway.
 * Since this is generally used to protect other memory information, we
 * use "asm volatile" and "memory" clobbers to prevent gcc from moving
 * information around.
 */
#define __atomic_exchange(object, desired)				\
({									\
    __typeof__((object)->counter) __ret = (desired);			\
    switch(sizeof((object)->counter)) {					\
    case __X86_CASE_B:							\
    {									\
	volatile uint8_t *__ptr = (volatile uint8_t *)&(object)->counter; \
	__asm__ __volatile__("xchgb %b0,%1"				\
			     : "=q" (__ret), "+m" (*__ptr)		\
			     : "0" (__ret)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_W:							\
    {									\
	volatile uint16_t *__ptr = (volatile uint16_t *)&(object)->counter; \
	__asm__ __volatile__("xchgw %w0,%1"				\
			     : "=r" (__ret), "+m" (*__ptr)		\
			     : "0" (__ret)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_L:							\
    {									\
	volatile uint32_t *__ptr = (volatile uint32_t *)&(object)->counter; \
	__asm__ __volatile__("xchgl %0,%1"				\
			     : "=r" (__ret), "+m" (*__ptr)		\
			     : "0" (__ret)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_Q:							\
    {									\
	volatile uint64_t *__ptr = (volatile uint64_t *)&(object)->counter; \
	__asm__ __volatile__("xchgq %q0,%1"				\
			     : "=r" (__ret), "+m" (*__ptr)		\
			     : "0" (__ret)				\
			     : "memory");				\
	break;								\
    }									\
    case __X86_CASE_Q_EMU:						\
	__AK_64not_implemented();					\
	break;								\
    default:								\
	__AK_wrong_size();						\
    }									\
    __ret;								\
})

/* C */
#define atomic_exchange_explicit(/* volatile A * */ object,	\
				 /* C */ desired,		\
				 /* memory_order */ order)	\
({								\
    __typeof__((object)->counter) __ret2;			\
    switch(order) {						\
    case memory_order_relaxed:					\
    case memory_order_consume:					\
    case memory_order_acquire:					\
    case memory_order_release:					\
    case memory_order_acq_rel:					\
    case memory_order_seq_cst:					\
    default:							\
	__ret2 = __atomic_exchange((object), (desired));	\
    }								\
    __ret2;							\
})

/* C */
#define atomic_exchange(/* volatile A * */ object,			\
			/* C */ desired)				\
    atomic_exchange_explicit((object), (desired), memory_order_seq_cst)

/*
 * Atomic compare and exchange.  Compare EXPECTED with OBJECT, if identical,
 * store DESIRED in OBJECT.  Set EXPECTED to the initial value in OBJECT.
 * Return true on success.
 */
#define __atomic_compare_exchange(object, expected, desired, lock)	\
({									\
    bool __ret;								\
    __typeof__((object)->counter) *__old = (expected);			\
    __typeof__((object)->counter) __new = (desired);			\
    switch(sizeof((object)->counter)) {					\
    case __X86_CASE_B:							\
    {									\
	volatile uint8_t *__ptr = (volatile uint8_t *)&(object)->counter; \
	__asm__ __volatile__(lock "cmpxchgb %b3,%1; setz %2"		\
			     : "=a" (*__old), "+m" (*__ptr), "=r" (__ret) \
			     : "q" (__new), "0" (*__old)		\
			     : "memory", "cc");				\
	break;								\
    }									\
    case __X86_CASE_W:							\
    {									\
	volatile uint16_t *__ptr = (volatile uint16_t *)&(object)->counter; \
	__asm__ __volatile__(lock "cmpxchgw %w3,%1; setz %2"		\
			     : "=a" (*__old), "+m" (*__ptr), "=r" (__ret) \
			     : "r" (__new), "0" (*__old)		\
			     : "memory", "cc");				\
	break;								\
    }									\
    case __X86_CASE_L:							\
    {									\
	volatile uint32_t *__ptr = (volatile uint32_t *)&(object)->counter; \
	__asm__ __volatile__(lock "cmpxchgl %3,%1; setz %2"		\
			     : "=a" (*__old), "+m" (*__ptr), "=r" (__ret) \
			     : "r" (__new), "0" (*__old)		\
			     : "memory", "cc");				\
	break;								\
    }									\
    case __X86_CASE_Q:							\
    {									\
	volatile uint64_t *__ptr = (volatile uint64_t *)&(object)->counter; \
	__asm__ __volatile__(lock "cmpxchgq %q3,%1; setz %2"		\
			     : "=a" (*__old), "+m" (*__ptr), "=r" (__ret) \
			     : "r" (__new), "0" (*__old)		\
			     : "memory", "cc");				\
	break;								\
    }									\
    case __X86_CASE_Q_EMU:						\
	__AK_64not_implemented();					\
	break;								\
    default:								\
	__AK_wrong_size();						\
    }									\
    __ret;								\
})

/* _Bool */
#define atomic_compare_exchange_strong_explicit(/* volatile A * */ object, \
						/* C * */ expected,	\
						/* C */ desired,	\
						/* memory_order */ success, \
						/* memory_order */ failure) \
({									\
    bool __ret2;							\
    switch((success)) { /* evaluate and discard the failure argument */ \
    case memory_order_relaxed:						\
    case memory_order_consume:						\
    case memory_order_acquire:						\
    case memory_order_release:						\
    case memory_order_acq_rel:						\
    if((failure) == memory_order_seq_cst) {				\
	typeof(expected) __expected = (expected);			\
	typeof(object) __object = (object);				\
	__ret2 = __atomic_compare_exchange(__object, __expected, (desired), ""); \
	if(!__ret2) {							\
	    *__expected = __atomic_load_lock(__object);			\
	}								\
    } else {								\
	__ret2 = __atomic_compare_exchange((object), (expected), (desired), ""); \
    }									\
    break;								\
    case memory_order_seq_cst:						\
    default:								\
	(void) failure;							\
	__ret2 = __atomic_compare_exchange((object), (expected), (desired), "lock;"); \
    }									\
    __ret2;								\
})

/* _Bool */
#define atomic_compare_exchange_strong(/* volatile A * */ object,	\
				       /* C * */ expected,		\
				       /* C */ desired)			\
    atomic_compare_exchange_strong_explicit((object),			\
					    (expected),			\
					    (desired),			\
					    memory_order_seq_cst,	\
					    memory_order_seq_cst)

/* _Bool */
#define atomic_compare_exchange_weak_explicit(/* volatile A * */ object, \
						/* C * */ expected,	\
						/* C */ desired,	\
						/* memory_order */ success, \
						/* memory_order */ failure) \
    atomic_compare_exchange_strong_explicit((object), (expected),	\
					    (desired), (success),	\
					    (failure))

/* _Bool */
#define atomic_compare_exchange_weak(/* volatile A * */ object,		\
				     /* C * */ expected,		\
				     /* C */ desired)			\
    atomic_compare_exchange_strong((object), (expected), (desired))


#define __atomic_fetch_add(object, operand, lock)			\
({									\
    __typeof__(operand) __operand = (operand);				\
    __typeof__((object)->counter) __ret = (operand);			\
    switch(sizeof((object)->counter)) {					\
    case __X86_CASE_B:							\
    {									\
	volatile uint8_t *__ptr = (volatile uint8_t *)&(object)->counter; \
	__asm__ __volatile__ (lock "xaddb %b0, %1"			\
			      : "=q" (__ret), "+m" (*__ptr)		\
			      : "0" (__operand)				\
			      : "memory", "cc");			\
	break;								\
    }									\
    case __X86_CASE_W:							\
    {									\
	volatile uint16_t *__ptr = (volatile uint16_t *)&(object)->counter; \
	__asm__ __volatile__ (lock "xaddw %w0, %1"			\
			      : "=r" (__ret), "+m" (*__ptr)	\
			      : "0" (__operand)				\
			      : "memory", "cc");			\
	break;								\
    }									\
    case __X86_CASE_L:							\
    {									\
	volatile uint32_t *__ptr = (volatile uint32_t *)&(object)->counter; \
	__asm__ __volatile__ (lock "xaddl %0, %1"			\
			      : "=r" (__ret), "+m" (*__ptr)	\
			      : "0" (__operand)				\
			      : "memory", "cc");			\
	break;								\
    }									\
    case __X86_CASE_Q:							\
    {									\
	volatile uint64_t *__ptr = (volatile uint64_t *)&(object)->counter; \
	__asm__ __volatile__ (lock "xaddq %q0, %1"			\
			      : "=r" (__ret), "+m" (*__ptr)	\
			      : "0" (__operand)				\
			      : "memory", "cc");			\
	break;								\
    }									\
    case __X86_CASE_Q_EMU:						\
	__AK_64not_implemented();					\
	break;								\
    default:								\
	__AK_wrong_size();						\
    }									\
    __ret;								\
})


/* C */
#define atomic_fetch_add_explicit(/* volatile A * */ object,		\
				  /* M */ operand,			\
				  /* memory_order */ order)		\
({									\
    __typeof__((object)->counter) __ret2;				\
    switch(order) {							\
    case memory_order_relaxed:						\
    case memory_order_consume:						\
    case memory_order_acquire:						\
    case memory_order_release:						\
    case memory_order_acq_rel:						\
	__ret2 = __atomic_fetch_add((object), (operand), "");		\
	break;								\
    case memory_order_seq_cst:						\
    default:								\
	__ret2 = __atomic_fetch_add((object), (operand), "lock;");	\
    }									\
    __ret2;								\
})

/* C */
#define atomic_fetch_add(/* volatile A * */ object,			\
			 /* M */ operand)				\
    atomic_fetch_add_explicit((object), (operand), memory_order_seq_cst)


/* C */
#define atomic_fetch_sub_explicit(/* volatile A * */ object,		\
				  /* M */ operand,			\
				  /* memory_order */ order)		\
    atomic_fetch_add_explicit((object), -(operand), (order))

/* C */
#define atomic_fetch_sub(/* volatile A * */ object,			\
			 /* M */ operand)				\
    atomic_fetch_sub_explicit((object), (operand), memory_order_seq_cst)


/* C */
#define atomic_fetch_or_explicit(/* volatile A * */ object,		\
				 /* M */ operand,			\
				 /* memory_order */ order)		\
({									\
    __typeof__(object) __obj3 = (object);				\
    __typeof__(operand) __operand = (operand);				\
    memory_order __order = (order);					\
    __typeof__(__obj3->counter) __ret3 = atomic_load_explicit(__obj3, memory_order_relaxed); \
    __typeof__(__obj3->counter) __new3;					\
    do {								\
	__new3 = __ret3 | __operand;					\
    } while(unlikely(!atomic_compare_exchange_weak_explicit(__obj3, &__ret3, \
							    __new3,	\
							    __order,	\
							    memory_order_relaxed))); \
    __ret3;								\
})

/* C */
#define atomic_fetch_or(/* volatile A * */ object,			\
			/* M */ operand)				\
    atomic_fetch_or_explicit((object), (operand), memory_order_seq_cst)


/* C */
#define atomic_fetch_xor_explicit(/* volatile A * */ object,		\
				 /* M */ operand,			\
				 /* memory_order */ order)		\
({									\
    __typeof__(object) __obj3 = (object);				\
    __typeof__(operand) __operand = (operand);				\
    memory_order __order = (order);					\
    __typeof__(__obj3->counter) __ret3 = atomic_load_explicit(__obj3, memory_order_relaxed); \
    __typeof__(__obj3->counter) __new3;					\
    do {								\
	__new3 = __ret3 ^ __operand;					\
    } while(unlikely(!atomic_compare_exchange_weak_explicit(__obj3, &__ret3, \
							    __new3,	\
							    __order,	\
							    memory_order_relaxed))); \
    __ret3;								\
})

/* C */
#define atomic_fetch_xor(/* volatile A * */ object,			\
			 /* M */ operand)				\
    atomic_fetch_xor_explicit((object), (operand), memory_order_seq_cst)


/* C */
#define atomic_fetch_and_explicit(/* volatile A * */ object,		\
				 /* M */ operand,			\
				 /* memory_order */ order)		\
({									\
    __typeof__(object) __obj3 = (object);				\
    __typeof__(operand) __operand = (operand);				\
    memory_order __order = (order);					\
    __typeof__(__obj3->counter) __ret3 = atomic_load_explicit(__obj3, memory_order_relaxed); \
    __typeof__(__obj3->counter) __new3;					\
    do {								\
	__new3 = __ret3 & __operand;					\
    } while(unlikely(!atomic_compare_exchange_weak_explicit(__obj3, &__ret3, \
							    __new3,	\
							    __order,	\
							    memory_order_relaxed))); \
    __ret3;								\
})

/* C */
#define atomic_fetch_and(/* volatile A * */ object,			\
			 /* M */ operand)				\
    atomic_fetch_and_explicit((object), (operand), memory_order_seq_cst)

#define __atomic_flag_test_and_set(object, lock)			\
({									\
    bool __ret;								\
    volatile uint16_t *__ptr = (volatile uint16_t *)&(object)->counter;	\
    __asm__ __volatile__ (lock "btsw $0, %1; setc %0" \
			  : "=r" (__ret), "+m" (*__ptr)			\
			  : : "memory", "cc");				\
    __ret;								\
})

/* _Bool */
#define atomic_flag_test_and_set_explicit(/* volatile atomic_flag * */ object, \
					  /* memory_order */ order)	\
({									\
    bool __ret2;							\
    switch(order) {							\
    case memory_order_relaxed:						\
    case memory_order_consume:						\
    case memory_order_acquire:						\
    case memory_order_release:						\
    case memory_order_acq_rel:						\
	__ret2 = __atomic_flag_test_and_set((object), "");		\
	break;								\
    case memory_order_seq_cst:						\
    default:								\
	__ret2 = __atomic_flag_test_and_set((object), "lock;");		\
    }									\
    __ret2;								\
})

/* _Bool */
#define atomic_flag_test_and_set(/* volatile atomic_flag * */ object)	\
    atomic_flag_test_and_set_explicit((object), memory_order_seq_cst)

#define __atomic_flag_clear(object, lock)				\
({									\
    volatile uint16_t *__ptr = (volatile uint16_t *)&(object)->counter;	\
    __asm__ __volatile__ (lock "andw $0, %0"				\
			  : "+m" (*__ptr)				\
			  : : "memory", "cc");				\
})

/* void */
#define atomic_flag_clear_explicit(/* volatile atomic_flag * */ object, \
				   /* memory_order */ order)		\
({									\
    switch(order) {							\
    case memory_order_relaxed:						\
    case memory_order_consume:						\
    case memory_order_acquire:						\
    case memory_order_release:						\
    case memory_order_acq_rel:						\
	__atomic_flag_clear((object), "");				\
	break;								\
    case memory_order_seq_cst:						\
    default:								\
	__atomic_flag_clear((object), "lock;");				\
    }									\
})

#define atomic_flag_clear(/* volatile atomic_flag * */ object) \
    atomic_flag_clear_explicit((object), memory_order_seq_cst)

#endif /* ! ATOMICKIT_ARCH_COMPAT_H */
