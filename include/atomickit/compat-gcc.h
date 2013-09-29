#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TYPEDEF_ATOMIC2(type, atomic_type)	\
    typedef struct {				\
	type __v;				\
    } atomic_type

#define TYPEDEF_ATOMIC1(type)			\
    typedef struct {				\
	type __v;				\
    } atomic_ ## type

TYPEDEF_ATOMIC2(uint_fast8_t, atomic_flag);
TYPEDEF_ATOMIC2(unsigned char, atomic_uchar);
TYPEDEF_ATOMIC2(signed char, atomic_schar);
TYPEDEF_ATOMIC2(unsigned short, atomic_ushort);
TYPEDEF_ATOMIC2(unsigned int, atomic_uint);
TYPEDEF_ATOMIC2(unsigned long, atomic_ulong);

TYPEDEF_ATOMIC1(bool);
TYPEDEF_ATOMIC1(char);
TYPEDEF_ATOMIC1(short);
TYPEDEF_ATOMIC1(int);
TYPEDEF_ATOMIC1(long);
TYPEDEF_ATOMIC1(wchar_t);
TYPEDEF_ATOMIC1(int_least8_t);
TYPEDEF_ATOMIC1(uint_least8_t);
TYPEDEF_ATOMIC1(int_least16_t);
TYPEDEF_ATOMIC1(uint_least16_t);
TYPEDEF_ATOMIC1(int_least32_t);
TYPEDEF_ATOMIC1(uint_least32_t);
TYPEDEF_ATOMIC1(int_least64_t);
TYPEDEF_ATOMIC1(uint_least64_t);
TYPEDEF_ATOMIC1(int_fast8_t);
TYPEDEF_ATOMIC1(uint_fast8_t);
TYPEDEF_ATOMIC1(int_fast16_t);
TYPEDEF_ATOMIC1(uint_fast16_t);
TYPEDEF_ATOMIC1(int_fast32_t);
TYPEDEF_ATOMIC1(uint_fast32_t);
TYPEDEF_ATOMIC1(int_fast64_t);
TYPEDEF_ATOMIC1(uint_fast64_t);
/* #ifdef __LP64__ */
/* TYPEDEF_ATOMIC2(__int128 __attribute__((aligned(16))), __atomic_int128_t) */
/* TYPEDEF_ATOMIC2(unsigned __int128 __attribute__((aligned(16))), __atomic_uint128_t) */
/* #endif */
TYPEDEF_ATOMIC1(intptr_t);
TYPEDEF_ATOMIC1(uintptr_t);
TYPEDEF_ATOMIC1(size_t);
TYPEDEF_ATOMIC1(ptrdiff_t);
TYPEDEF_ATOMIC1(intmax_t);
TYPEDEF_ATOMIC1(uintmax_t);

#undef TYPEDEF_ATOMIC1
#undef TYPEDEF_ATOMIC2

typedef enum {
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_consume = __ATOMIC_CONSUME,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order;

#define ATOMIC_BOOL_LOCK_FREE __atomic_always_lock_free(sizeof(bool), 0)
#define ATOMIC_CHAR_LOCK_FREE __atomic_always_lock_free(sizeof(char), 0)
#define ATOMIC_WCHAR_T_LOCK_FREE __atomic_always_lock_free(sizeof(wchar_t), 0)
#define ATOMIC_SHORT_LOCK_FREE __atomic_always_lock_free(sizeof(short), 0)
#define ATOMIC_INT_LOCK_FREE __atomic_always_lock_free(sizeof(int), 0)
#define ATOMIC_LONG_LOCK_FREE __atomic_always_lock_free(sizeof(long), 0)
#define ATOMIC_LLONG_LOCK_FREE __atomic_always_lock_free(sizeof(long long), 0)
#define ATOMIC_POINTER_LOCK_FREE __atomic_always_lock_free(sizeof(void *), 0)

#define ATOMIC_FLAG_INIT { 0 }

#define ATOMIC_VAR_INIT(/* C */ value) { (value) }

/* void */
#define atomic_init(/* volatile A * */ obj, /* C */ value)	\
    ((void) ((obj)->__v = (value)))

/* void */
#define atomic_thread_fence(/* memory_order */ order)	\
    __atomic_thread_fence(order)

/* void */
#define atomic_signal_fence(/* memory_order */ order)	\
    __atomic_signal_fence(order)

/* bool */
#define atomic_is_lock_free(/* const volatile A * */ obj)	\
    __atomic_is_lock_free(sizeof((obj)->__v), &(obj)->__v)

#ifdef __LP64__

# define atomic_store_explicit(/* volatile A * */ obj,			\
			       /* C */ desired,				\
			       /* memory_order */ order)		\
    __builtin_choose_expr(sizeof((obj)->__v) == 1, __atomic_store_1(&(obj)->__v, (desired), (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 2, __atomic_store_2(&(obj)->__v, (desired), (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 4, __atomic_store_4(&(obj)->__v, (desired), (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 8, __atomic_store_8(&(obj)->__v, (desired), (order)), \
			  __atomic_store_16(&(obj)->__v, (desired), (order))))))

#else /* ! __LP64__ */

# define atomic_store_explicit(/* volatile A * */ obj,			\
			       /* C */ desired,				\
			       /* memory_order */ order)		\
    __builtin_choose_expr(sizeof((obj)->__v) == 1, __atomic_store_1(&(obj)->__v, (desired), (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 2, __atomic_store_2(&(obj)->__v, (desired), (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 4, __atomic_store_4(&(obj)->__v, (desired), (order)), \
			  __atomic_store_8(&(obj)->__v, (desired), (order)))))

#endif /* __LP64__ */

#define atomic_store(/* volatile A * */ obj,		\
		     /* C */ desired)			\
    atomic_store_explicit((obj), (desired), memory_order_seq_cst)

#ifdef __LP64__

# define atomic_load_explicit(/* volatile A * */ obj,			\
			      /* memory_order */ order)			\
    __builtin_choose_expr(sizeof((obj)->__v) == 1, __atomic_load_1(&(obj)->__v, (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 2, __atomic_load_2(&(obj)->__v, (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 4, __atomic_load_4(&(obj)->__v, (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 8, __atomic_load_8(&(obj)->__v, (order)), \
			  __atomic_load_16(&(obj)->__v, (order))))))

#else /* ! __LP64__ */

# define atomic_load_explicit(/* volatile A * */ obj,			\
			      /* memory_order */ order)			\
    __builtin_choose_expr(sizeof((obj)->__v) == 1, __atomic_load_1(&(obj)->__v, (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 2, __atomic_load_2(&(obj)->__v, (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 4, __atomic_load_4(&(obj)->__v, (order)), \
			  __atomic_load_8(&(obj)->__v, (order)))))

#endif /* __LP64__ */

#define atomic_load(/* volatile A * */ obj)	\
    atomic_load_explicit((obj), memory_order_seq_cst)

#ifdef __LP64__

# define atomic_exchange_explicit(/* volatile A * */ obj,		\
				  /* C */ desired,			\
				  /* memory_order */ order)		\
    __builtin_choose_expr(sizeof((obj)->__v) == 1, __atomic_exchange_1(&(obj)->__v, (desired), (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 2, __atomic_exchange_2(&(obj)->__v, (desired), (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 4, __atomic_exchange_4(&(obj)->__v, (desired), (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 8, __atomic_exchange_8(&(obj)->__v, (desired), (order)), \
			  __atomic_exchange_16(&(obj)->__v, (desired), (order))))))

#else /* ! __LP64__ */

# define atomic_exchange_explicit(/* volatile A * */ obj,		\
				  /* C */ desired,			\
				  /* memory_order */ order)		\
    __builtin_choose_expr(sizeof((obj)->__v) == 1, __atomic_exchange_1(&(obj)->__v, (desired), (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 2, __atomic_exchange_2(&(obj)->__v, (desired), (order)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 4, __atomic_exchange_4(&(obj)->__v, (desired), (order)), \
			  __atomic_exchange_8(&(obj)->__v, (desired), (order)))))

#endif /* __LP64__ */

#define atomic_exchange(/* volatile A * */ obj,				\
			/* C */ desired)				\
    atomic_exchange_explicit((obj), (desired), memory_order_seq_cst)

#ifdef __LP64__

# define __atomic_compare_exchange_explicit(/* volatile A * */ obj,	\
					    /* C * */ expected,		\
					    /* C */ desired,		\
					    /* bool */ weak,		\
					    /* memory_order */ success,	\
					    /* memory_order */ failure)	\
    __builtin_choose_expr(sizeof((obj)->__v) == 1, __atomic_compare_exchange_1(&(obj)->__v, (expected), (desired), (weak), (success), (failure)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 2, __atomic_compare_exchange_2(&(obj)->__v, (expected), (desired), (weak), (success), (failure)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 4, __atomic_compare_exchange_4(&(obj)->__v, (expected), (desired), (weak), (success), (failure)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 8, __atomic_compare_exchange_8(&(obj)->__v, (expected), (desired), (weak), (success), (failure)), \
			  __atomic_compare_exchange_16(&(obj)->__v, (expected), (desired), (weak), (success), (failure))))))

#else /* ! __LP64__ */

# define __atomic_compare_exchange_explicit(/* volatile A * */ obj,	\
					    /* C * */ expected,		\
					    /* C */ desired,		\
					    /* bool */ weak,		\
					    /* memory_order */ success,	\
					    /* memory_order */ failure)	\
    __builtin_choose_expr(sizeof((obj)->__v) == 1, __atomic_compare_exchange_1(&(obj)->__v, (expected), (desired), (weak), (success), (failure)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 2, __atomic_compare_exchange_2(&(obj)->__v, (expected), (desired), (weak), (success), (failure)), \
    __builtin_choose_expr(sizeof((obj)->__v) == 4, __atomic_compare_exchange_4(&(obj)->__v, (expected), (desired), (weak), (success), (failure)), \
			  __atomic_compare_exchange_8(&(obj)->__v, (expected), (desired), (weak), (success), (failure)))))

#endif /* __LP64__ */

#define atomic_compare_exchange_weak_explicit(/* volatile A * */ obj,	\
					      /* C * */ expected,	\
					      /* C */ desired,		\
					      /* memory_order */ success, \
					      /* memory_order */ failure) \
    __atomic_compare_exchange_explicit((obj), (expected), (desired), false, \
				       (success), (failure))

#define atomic_compare_exchange_weak(/* volatile A * */ obj,		\
				     /* C * */ expected,		\
				     /* C */ desired)			\
    atomic_compare_exchange_weak_explicit((obj), (expected), (desired), \
					  memory_order_seq_cst, memory_order_seq_cst)

#define atomic_compare_exchange_strong_explicit(/* volatile A * */ obj,	\
					      /* C * */ expected,	\
					      /* C */ desired,		\
					      /* memory_order */ success, \
					      /* memory_order */ failure) \
    __atomic_compare_exchange_explicit((obj), (expected), (desired), true, \
				       (success), (failure))

#define atomic_compare_exchange_strong(/* volatile A * */ obj,		\
				       /* C * */ expected,		\
				       /* C */ desired)			\
    atomic_compare_exchange_strong_explicit((obj), (expected), (desired), \
					    memory_order_seq_cst, memory_order_seq_cst)

/* C */
#define atomic_fetch_add_explicit(/* volatile A * */ obj,		\
				  /* M */ operand,			\
				  /* memory_order */ order)		\
    __atomic_fetch_add(&(obj)->__v, (operand), (order))

/* C */
#define atomic_fetch_add(/* volatile A * */ obj,		\
			 /* M */ operand)			\
    atomic_fetch_add_explicit((obj), (operand), memory_order_seq_cst)

/* C */
#define atomic_fetch_sub_explicit(/* volatile A * */ obj,		\
				  /* M */ operand,			\
				  /* memory_order */ order)		\
    __atomic_fetch_sub(&(obj)->__v, (operand), (order))

/* C */
#define atomic_fetch_sub(/* volatile A * */ obj,		\
			 /* M */ operand)			\
      atomic_fetch_sub_explicit((obj), (operand), memory_order_seq_cst)

/* C */
#define atomic_fetch_and_explicit(/* volatile A * */ obj,		\
				  /* M */ operand,			\
				  /* memory_order */ order)		\
    __atomic_fetch_and(&(obj)->__v, (operand), (order))

/* C */
#define atomic_fetch_and(/* volatile A * */ obj,		\
			 /* M */ operand)			\
      atomic_fetch_and_explicit((obj), (operand), memory_order_seq_cst)

/* C */
#define atomic_fetch_xor_explicit(/* volatile A * */ obj,		\
				  /* M */ operand,			\
				  /* memory_order */ order)		\
    __atomic_fetch_xor(&(obj)->__v, (operand), (order))

/* C */
#define atomic_fetch_xor(/* volatile A * */ obj,		\
			 /* M */ operand)			\
      atomic_fetch_xor_explicit((obj), (operand), (order))

/* C */
#define atomic_fetch_or_explicit(/* volatile A * */ obj,		\
				  /* M */ operand,			\
				  /* memory_order */ order)		\
    __atomic_fetch_or(&(obj)->__v, (operand), (order))

/* C */
#define atomic_fetch_or(/* volatile A * */ obj,		\
			 /* M */ operand)			\
      atomic_fetch_or_explicit((obj), (operand), memory_order_seq_cst)

static inline bool atomic_flag_test_and_set_explicit(volatile atomic_flag *obj, memory_order order) {
    return __atomic_test_and_set(&obj->__v, order);
}

static inline bool atomic_flag_test_and_set(volatile atomic_flag *obj) {
    return atomic_flag_test_and_set_explicit(obj, memory_order_seq_cst);
}

static inline void atomic_flag_clear_explicit(volatile atomic_flag *obj, memory_order order) {
    __atomic_clear(&obj->__v, order);
}

static inline void atomic_flag_clear(volatile atomic_flag *obj) {
    atomic_flag_clear_explicit(obj, memory_order_seq_cst);
}
