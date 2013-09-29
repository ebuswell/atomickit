/*
 * test_atomic_h.c
 * 
 * Copyright 2013 Evan Buswell
 * 
 * This file is part of Atomic Kit.
 * 
 * Atomic Kit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License,
 * or (at your option) any later version.
 * 
 * Atomic Kit is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "atomickit/atomic.h"
#include "alltests.h"
#include "test.h"

static volatile atomic_bool abool = ATOMIC_VAR_INIT(false);
static volatile atomic_char achar = ATOMIC_VAR_INIT('\0');
static volatile atomic_schar aschar = ATOMIC_VAR_INIT(0);
static volatile atomic_uchar auchar = ATOMIC_VAR_INIT(0);
static volatile atomic_short ashort = ATOMIC_VAR_INIT(0);
static volatile atomic_ushort aushort = ATOMIC_VAR_INIT(0);
static volatile atomic_int aint = ATOMIC_VAR_INIT(0);
static volatile atomic_uint auint = ATOMIC_VAR_INIT(0);
static volatile atomic_long along = ATOMIC_VAR_INIT(0);
static volatile atomic_ulong aulong = ATOMIC_VAR_INIT(0);
static volatile atomic_wchar_t awchar = ATOMIC_VAR_INIT('\0');
static volatile atomic_intptr_t aintptr = ATOMIC_VAR_INIT(0);
static volatile atomic_uintptr_t auintptr = ATOMIC_VAR_INIT(0);
static volatile atomic_size_t asize = ATOMIC_VAR_INIT(0);
static volatile atomic_ptrdiff_t aptrdiff = ATOMIC_VAR_INIT(0);
static volatile atomic_intmax_t aintmax = ATOMIC_VAR_INIT(0);
static volatile atomic_uintmax_t auintmax = ATOMIC_VAR_INIT(0);
static volatile atomic_flag aflag = ATOMIC_FLAG_INIT;

static void test_atomic_init() {
    CHECKPOINT();
    atomic_init(&aint, 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 21);
}

static void test_atomic_thread_fence() {
    CHECKPOINT();
    atomic_thread_fence(memory_order_relaxed);
    CHECKPOINT();
    atomic_thread_fence(memory_order_consume);
    CHECKPOINT();
    atomic_thread_fence(memory_order_acquire);
    CHECKPOINT();
    atomic_thread_fence(memory_order_release);
    CHECKPOINT();
    atomic_thread_fence(memory_order_acq_rel);
    CHECKPOINT();
    atomic_thread_fence(memory_order_seq_cst);
}

static void test_atomic_signal_fence() {
    CHECKPOINT();
    atomic_signal_fence(memory_order_relaxed);
    CHECKPOINT();
    atomic_signal_fence(memory_order_consume);
    CHECKPOINT();
    atomic_signal_fence(memory_order_acquire);
    CHECKPOINT();
    atomic_signal_fence(memory_order_release);
    CHECKPOINT();
    atomic_signal_fence(memory_order_acq_rel);
    CHECKPOINT();
    atomic_signal_fence(memory_order_seq_cst);
}

static void test_atomic_is_lock_free() {
    _Bool b;
    CHECKPOINT();
    b = atomic_is_lock_free(&abool);
    ASSERT(!b == !ATOMIC_BOOL_LOCK_FREE);
    b = atomic_is_lock_free(&achar);
    ASSERT(!b == !ATOMIC_CHAR_LOCK_FREE);
    b = atomic_is_lock_free(&aschar);
    CHECKPOINT();
    b = atomic_is_lock_free(&auchar);
    CHECKPOINT();
    b = atomic_is_lock_free(&ashort);
    ASSERT(!b == !ATOMIC_SHORT_LOCK_FREE);
    b = atomic_is_lock_free(&aushort);
    CHECKPOINT();
    b = atomic_is_lock_free(&aint);
    ASSERT(!b == !ATOMIC_INT_LOCK_FREE);
    b = atomic_is_lock_free(&auint);
    CHECKPOINT();
    b = atomic_is_lock_free(&along);
    ASSERT(!b == !ATOMIC_LONG_LOCK_FREE);
    b = atomic_is_lock_free(&aulong);
    CHECKPOINT();
    b = atomic_is_lock_free(&awchar);
    ASSERT(!b == !ATOMIC_WCHAR_T_LOCK_FREE);
    b = atomic_is_lock_free(&aintptr);
    ASSERT(!b == !ATOMIC_POINTER_LOCK_FREE);
    b = atomic_is_lock_free(&auintptr);
    CHECKPOINT();
    b = atomic_is_lock_free(&asize);
    CHECKPOINT();
    b = atomic_is_lock_free(&aptrdiff);
    CHECKPOINT();
    b = atomic_is_lock_free(&aintmax);
    CHECKPOINT();
    b = atomic_is_lock_free(&auintmax);
    CHECKPOINT();
    b = atomic_is_lock_free(&aflag);
}

/*****************************************************/

static void test_atomic_int_fixture(void (*test)()) {
    atomic_init(&aint, 21);
    test();
}

static void test_atomic_store_explicit() {
    atomic_store_explicit(&aint, 31, memory_order_relaxed);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 31);
}

static void test_atomic_store() {
    atomic_store(&aint, 31);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 31);
}

static void test_atomic_load_explicit() {
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 21);    
}

static void test_atomic_load() {
    ASSERT(atomic_load(&aint) == 21);
}

static void test_atomic_exchange_explicit() {
    int r = 0;
    r = atomic_exchange_explicit(&aint, 31, memory_order_relaxed);
    ASSERT(r == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 31);
}

static void test_atomic_exchange() {
    int r = 0;
    r = atomic_exchange(&aint, 31);
    ASSERT(r == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 31);
}

static void test_atomic_compare_exchange_strong_explicit() {
    int r = 0;
    ASSERT(!atomic_compare_exchange_strong_explicit(&aint, &r, 31, memory_order_relaxed, memory_order_relaxed));
    ASSERT(r == 21);
    ASSERT(atomic_compare_exchange_strong_explicit(&aint, &r, 31, memory_order_relaxed, memory_order_relaxed));
    ASSERT(r == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 31);
}

static void test_atomic_compare_exchange_strong() {
    int r = 0;
    ASSERT(!atomic_compare_exchange_strong(&aint, &r, 31));
    ASSERT(r == 21);
    ASSERT(atomic_compare_exchange_strong(&aint, &r, 31));
    ASSERT(r == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 31);
}

static void test_atomic_compare_exchange_weak_explicit() {
    int r = 0;
    ASSERT(!atomic_compare_exchange_weak_explicit(&aint, &r, 31, memory_order_relaxed, memory_order_relaxed));
    ASSERT(r == 21);
    ASSERT(atomic_compare_exchange_weak_explicit(&aint, &r, 31, memory_order_relaxed, memory_order_relaxed));
    ASSERT(r == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 31);
}

static void test_atomic_compare_exchange_weak() {
    int r = 0;
    ASSERT(!atomic_compare_exchange_weak(&aint, &r, 31));
    ASSERT(r == 21);
    ASSERT(atomic_compare_exchange_weak(&aint, &r, 31));
    ASSERT(r == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 31);
}

static void test_atomic_fetch_add_explicit() {
    ASSERT(atomic_fetch_add_explicit(&aint, 10, memory_order_relaxed) == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 31);
}

static void test_atomic_fetch_add() {
    ASSERT(atomic_fetch_add(&aint, 10) == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 31);
}

static void test_atomic_fetch_sub_explicit() {
    ASSERT(atomic_fetch_sub_explicit(&aint, 10, memory_order_relaxed) == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 11);
}

static void test_atomic_fetch_sub() {
    ASSERT(atomic_fetch_sub(&aint, 10) == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 11);
}

static void test_atomic_fetch_or_explicit() {
    ASSERT(atomic_fetch_or_explicit(&aint, 30, memory_order_relaxed) == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 31);
}

static void test_atomic_fetch_or() {
    ASSERT(atomic_fetch_or(&aint, 30) == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 31);
}

static void test_atomic_fetch_and_explicit() {
    ASSERT(atomic_fetch_and_explicit(&aint, 19, memory_order_relaxed) == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 17);
}

static void test_atomic_fetch_and() {
    ASSERT(atomic_fetch_and(&aint, 19) == 21);
    ASSERT(atomic_load_explicit(&aint, memory_order_relaxed) == 17);
}

/*******************************************/

static void test_atomic_flag_unset_fixture(void (*test)()) {
    atomic_flag_clear_explicit(&aflag, memory_order_relaxed);
    test();
}

static void test_atomic_flag_test_and_set_explicit() {
    ASSERT(!atomic_flag_test_and_set_explicit(&aflag, memory_order_relaxed));
    ASSERT(atomic_flag_test_and_set_explicit(&aflag, memory_order_relaxed));    
}

static void test_atomic_flag_test_and_set() {
    ASSERT(!atomic_flag_test_and_set(&aflag));
    ASSERT(atomic_flag_test_and_set(&aflag));    
}

/*******************************************/

static void test_atomic_flag_set_fixture(void (*test)()) {
    atomic_flag_test_and_set_explicit(&aflag, memory_order_relaxed);
    test();
}

static void test_atomic_flag_clear_explicit() {
    CHECKPOINT();
    atomic_flag_clear_explicit(&aflag, memory_order_relaxed);
    ASSERT(!atomic_flag_test_and_set_explicit(&aflag, memory_order_relaxed));    
}

static void test_atomic_flag_clear() {
    CHECKPOINT();
    atomic_flag_clear(&aflag);
    ASSERT(!atomic_flag_test_and_set_explicit(&aflag, memory_order_relaxed));    
}

int run_atomic_h_test_suite() {
    int r;
    void (*atomic_blank_tests[])() = { test_atomic_init, test_atomic_thread_fence,
				       test_atomic_signal_fence, test_atomic_is_lock_free, NULL };
    char *atomic_blank_test_names[] = { "atomic_init", "atomic_thread_fence",
					"atomic_signal_fence", "atomic_is_lock_free", NULL };
    void (*atomic_int_tests[])() = { test_atomic_store_explicit, test_atomic_store,
				     test_atomic_load_explicit, test_atomic_load,
				     test_atomic_exchange_explicit, test_atomic_exchange,
				     test_atomic_compare_exchange_strong_explicit,
				     test_atomic_compare_exchange_strong,
				     test_atomic_compare_exchange_weak_explicit,
				     test_atomic_compare_exchange_weak,
				     test_atomic_fetch_add_explicit, test_atomic_fetch_add,
				     test_atomic_fetch_sub_explicit, test_atomic_fetch_sub,
				     test_atomic_fetch_or_explicit, test_atomic_fetch_or,
				     test_atomic_fetch_and_explicit,test_atomic_fetch_and, NULL};
    
    char *atomic_int_test_names[] = { "atomic_store_explicit", "atomic_store",
				      "atomic_load_explicit", "atomic_load",
				      "atomic_exchange_explicit", "atomic_exchange",
				      "atomic_compare_exchange_strong_explicit",
				      "atomic_compare_exchange_strong",
				      "atomic_compare_exchange_weak_explicit",
				      "atomic_compare_exchange_weak",
				      "atomic_fetch_add_explicit", "atomic_fetch_add",
				      "atomic_fetch_sub_explicit", "atomic_fetch_sub",
				      "atomic_fetch_or_explicit", "atomic_fetch_or",
				      "atomic_fetch_and_explicit","atomic_fetch_and", NULL };

    void (*atomic_flag_unset_tests[])() = { test_atomic_flag_test_and_set_explicit,
					    test_atomic_flag_test_and_set, NULL };
    
    char *atomic_flag_unset_test_names[] = { "atomic_flag_test_and_set_explicit",
					     "atomic_flag_test_and_set", NULL };

    void (*atomic_flag_set_tests[])() = { test_atomic_flag_clear_explicit,
					  test_atomic_flag_clear, NULL };
    
    char *atomic_flag_set_test_names[] = { "atomic_flag_test_and_clear_explicit",
    					   "atomic_flag_test_and_clear", NULL };

    r = run_test_suite(NULL, atomic_blank_test_names, atomic_blank_tests);
    if(r != 0) {
	return r;
    }

    r = run_test_suite(test_atomic_int_fixture, atomic_int_test_names, atomic_int_tests);
    if(r != 0) {
	return r;
    }

    r = run_test_suite(test_atomic_flag_unset_fixture, atomic_flag_unset_test_names,
		       atomic_flag_unset_tests);
    if(r != 0) {
	return r;
    }

    r = run_test_suite(test_atomic_flag_set_fixture, atomic_flag_set_test_names,
		       atomic_flag_set_tests);
    if(r != 0) {
	return r;
    }

    return r;
}
