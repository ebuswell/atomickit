/*
 * test_atomic_float_h.c
 *
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
#include <atomickit/atomic-float.h>
#include "alltests.h"
#include "test.h"

static volatile atomic_float afloat;
static volatile atomic_double adouble;

static void test_atomic_float_init() {
    CHECKPOINT();
    atomic_float_init(&afloat, 2.1f);
    ASSERT(atomic_float_load_explicit(&afloat, memory_order_relaxed) == 2.1f);
}

static void test_atomic_float_is_lock_free() {
    CHECKPOINT();
    (void) atomic_float_is_lock_free(&afloat);
}

/******************************************************/

static void test_atomic_float_fixture(void (*test)()) {
    atomic_float_init(&afloat, 2.1f);
    test();
}

static void test_atomic_float_store_explicit() {
    CHECKPOINT();
    atomic_float_store_explicit(&afloat, 3.1f, memory_order_relaxed);
    ASSERT(atomic_float_load_explicit(&afloat, memory_order_relaxed) == 3.1f);
}

static void test_atomic_float_store() {
    CHECKPOINT();
    atomic_float_store(&afloat, 3.1f);
    ASSERT(atomic_float_load_explicit(&afloat, memory_order_relaxed) == 3.1f);
}

static void test_atomic_float_load_explicit() {
    ASSERT(atomic_float_load_explicit(&afloat, memory_order_relaxed) == 2.1f);
}

static void test_atomic_float_load() {
    ASSERT(atomic_float_load(&afloat) == 2.1f);
}

static void test_atomic_float_exchange_explicit() {
    float fr;
    CHECKPOINT();
    fr = atomic_float_exchange_explicit(&afloat, 3.1f, memory_order_relaxed);
    ASSERT(fr == 2.1f);
    ASSERT(atomic_float_load_explicit(&afloat, memory_order_relaxed) == 3.1f);
}

static void test_atomic_float_exchange() {
    float fr;
    CHECKPOINT();
    fr = atomic_float_exchange(&afloat, 3.1f);
    ASSERT(fr == 2.1f);
    ASSERT(atomic_float_load_explicit(&afloat, memory_order_relaxed) == 3.1f);
}

static void test_atomic_float_compare_exchange_strong_explicit() {
    float fr = 1.1f;
    ASSERT(!atomic_float_compare_exchange_strong_explicit(&afloat, &fr, 3.1f, memory_order_relaxed, memory_order_relaxed));
    ASSERT(fr == 2.1f);
    ASSERT(atomic_float_compare_exchange_strong_explicit(&afloat, &fr, 3.1f, memory_order_relaxed, memory_order_relaxed));
    ASSERT(fr == 2.1f);
    ASSERT(atomic_float_load_explicit(&afloat, memory_order_relaxed) == 3.1f);
}

static void test_atomic_float_compare_exchange_strong() {
    float fr = 1.1f;
    ASSERT(!atomic_float_compare_exchange_strong(&afloat, &fr, 3.1f));
    ASSERT(fr == 2.1f);
    ASSERT(atomic_float_compare_exchange_strong(&afloat, &fr, 3.1f));
    ASSERT(fr == 2.1f);
    ASSERT(atomic_float_load_explicit(&afloat, memory_order_relaxed) == 3.1f);
}

static void test_atomic_float_compare_exchange_weak_explicit() {
    float fr = 1.1f;
    ASSERT(!atomic_float_compare_exchange_weak_explicit(&afloat, &fr, 3.1f, memory_order_relaxed, memory_order_relaxed));
    ASSERT(fr == 2.1f);
    ASSERT(atomic_float_compare_exchange_weak_explicit(&afloat, &fr, 3.1f, memory_order_relaxed, memory_order_relaxed));
    ASSERT(fr == 2.1f);
    ASSERT(atomic_float_load_explicit(&afloat, memory_order_relaxed) == 3.1f);
}

static void test_atomic_float_compare_exchange_weak() {
    float fr = 1.1f;
    ASSERT(!atomic_float_compare_exchange_weak(&afloat, &fr, 3.1f));
    ASSERT(fr == 2.1f);
    ASSERT(atomic_float_compare_exchange_weak(&afloat, &fr, 3.1f));
    ASSERT(fr == 2.1f);
    ASSERT(atomic_float_load_explicit(&afloat, memory_order_relaxed) == 3.1f);
}

/******************************************************/

static void test_atomic_double_init() {
    CHECKPOINT();
    atomic_double_init(&adouble, 2.1);
    ASSERT(atomic_double_load_explicit(&adouble, memory_order_relaxed) == 2.1);
}

static void test_atomic_double_is_lock_free() {
    CHECKPOINT();
    (void) atomic_double_is_lock_free(&adouble);
}

/******************************************************/

static void test_atomic_double_fixture(void (*test)()) {
    atomic_double_init(&adouble, 2.1);
    test();
}

static void test_atomic_double_store_explicit() {
    CHECKPOINT();
    atomic_double_store_explicit(&adouble, 3.1, memory_order_relaxed);
    ASSERT(atomic_double_load_explicit(&adouble, memory_order_relaxed) == 3.1);
}

static void test_atomic_double_store() {
    CHECKPOINT();
    atomic_double_store(&adouble, 3.1);
    ASSERT(atomic_double_load_explicit(&adouble, memory_order_relaxed) == 3.1);
}

static void test_atomic_double_load_explicit() {
    ASSERT(atomic_double_load_explicit(&adouble, memory_order_relaxed) == 2.1);
}

static void test_atomic_double_load() {
    ASSERT(atomic_double_load(&adouble) == 2.1);
}

static void test_atomic_double_exchange_explicit() {
    double dr;
    CHECKPOINT();
    dr = atomic_double_exchange_explicit(&adouble, 3.1, memory_order_relaxed);
    ASSERT(dr == 2.1);
    ASSERT(atomic_double_load_explicit(&adouble, memory_order_relaxed) == 3.1);
}

static void test_atomic_double_exchange() {
    double dr;
    CHECKPOINT();
    dr = atomic_double_exchange(&adouble, 3.1);
    ASSERT(dr == 2.1);
    ASSERT(atomic_double_load_explicit(&adouble, memory_order_relaxed) == 3.1);
}

static void test_atomic_double_compare_exchange_strong_explicit() {
    double dr = 1.1;
    ASSERT(!atomic_double_compare_exchange_strong_explicit(&adouble, &dr, 3.1, memory_order_relaxed, memory_order_relaxed));
    ASSERT(dr == 2.1);
    ASSERT(atomic_double_compare_exchange_strong_explicit(&adouble, &dr, 3.1, memory_order_relaxed, memory_order_relaxed));
    ASSERT(dr == 2.1);
    ASSERT(atomic_double_load_explicit(&adouble, memory_order_relaxed) == 3.1);
}

static void test_atomic_double_compare_exchange_strong() {
    double dr = 1.1;
    ASSERT(!atomic_double_compare_exchange_strong(&adouble, &dr, 3.1));
    ASSERT(dr == 2.1);
    ASSERT(atomic_double_compare_exchange_strong(&adouble, &dr, 3.1));
    ASSERT(dr == 2.1);
    ASSERT(atomic_double_load_explicit(&adouble, memory_order_relaxed) == 3.1);
}

static void test_atomic_double_compare_exchange_weak_explicit() {
    double dr = 1.1;
    ASSERT(!atomic_double_compare_exchange_weak_explicit(&adouble, &dr, 3.1, memory_order_relaxed, memory_order_relaxed));
    ASSERT(dr == 2.1);
    ASSERT(atomic_double_compare_exchange_weak_explicit(&adouble, &dr, 3.1, memory_order_relaxed, memory_order_relaxed));
    ASSERT(dr == 2.1);
    ASSERT(atomic_double_load_explicit(&adouble, memory_order_relaxed) == 3.1);
}

static void test_atomic_double_compare_exchange_weak() {
    double dr = 1.1;
    ASSERT(!atomic_double_compare_exchange_weak(&adouble, &dr, 3.1));
    ASSERT(dr == 2.1);
    ASSERT(atomic_double_compare_exchange_weak(&adouble, &dr, 3.1));
    ASSERT(dr == 2.1);
    ASSERT(atomic_double_load_explicit(&adouble, memory_order_relaxed) == 3.1);
}


int run_atomic_float_h_test_suite() {
    int r;
    void (*atomic_float_blank_tests[])() = { test_atomic_float_init, test_atomic_float_is_lock_free, NULL };
    char *atomic_float_blank_test_names[] = { "atomic_float_init", "atomic_float_is_lock_free", NULL };

    void (*atomic_float_tests[])() = { test_atomic_float_store_explicit, test_atomic_float_store,
				       test_atomic_float_load_explicit, test_atomic_float_load,
				       test_atomic_float_exchange_explicit, test_atomic_float_exchange,
				       test_atomic_float_compare_exchange_strong_explicit,
				       test_atomic_float_compare_exchange_strong,
				       test_atomic_float_compare_exchange_weak_explicit,
				       test_atomic_float_compare_exchange_weak, NULL };
    char *atomic_float_test_names[] = { "atomic_float_store_explicit", "atomic_float_store",
					"atomic_float_load_explicit", "atomic_float_load",
					"atomic_float_exchange_explicit", "atomic_float_exchange",
					"atomic_float_compare_exchange_strong_explicit",
					"atomic_float_compare_exchange_strong",
					"atomic_float_compare_exchange_weak_explicit",
					"atomic_float_compare_exchange_weak", NULL };
    void (*atomic_double_blank_tests[])() = { test_atomic_double_init, test_atomic_double_is_lock_free, NULL };
    char *atomic_double_blank_test_names[] = { "atomic_double_init", "atomic_double_is_lock_free", NULL };

    void (*atomic_double_tests[])() = { test_atomic_double_store_explicit, test_atomic_double_store,
				       test_atomic_double_load_explicit, test_atomic_double_load,
				       test_atomic_double_exchange_explicit, test_atomic_double_exchange,
				       test_atomic_double_compare_exchange_strong_explicit,
				       test_atomic_double_compare_exchange_strong,
				       test_atomic_double_compare_exchange_weak_explicit,
				       test_atomic_double_compare_exchange_weak, NULL };
    char *atomic_double_test_names[] = { "atomic_double_store_explicit", "atomic_double_store",
					"atomic_double_load_explicit", "atomic_double_load",
					"atomic_double_exchange_explicit", "atomic_double_exchange",
					"atomic_double_compare_exchange_strong_explicit",
					"atomic_double_compare_exchange_strong",
					"atomic_double_compare_exchange_weak_explicit",
					"atomic_double_compare_exchange_weak", NULL };

    r = run_test_suite(NULL, atomic_float_blank_test_names, atomic_float_blank_tests);
    if(r != 0) {
	return r;
    }

    r = run_test_suite(test_atomic_float_fixture, atomic_float_test_names, atomic_float_tests);
    if(r != 0) {
	return r;
    }

    r = run_test_suite(NULL, atomic_double_blank_test_names, atomic_double_blank_tests);
    if(r != 0) {
	return r;
    }

    r = run_test_suite(test_atomic_double_fixture, atomic_double_test_names, atomic_double_tests);
    if(r != 0) {
	return r;
    }

    return 0;
}
