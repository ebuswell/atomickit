/*
 * test_atomic_pointer_h.c
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
#include <atomickit/atomic-pointer.h>
#include "test.h"

static struct {
    char __attribute__((aligned(8))) string1[14];
    char __attribute__((aligned(8))) string2[14];
    char __attribute__((aligned(8))) string3[14];
} ptrtest = {"Test String 1", "Test String 2", "Test String 3"};

static volatile atomic_ptr aptr = ATOMIC_PTR_VAR_INIT(NULL);

static void test_atomic_ptr_blank_fixture(void (*test)()) {
    test();
}

static void test_atomic_ptr_init() {
    CHECKPOINT();
    atomic_ptr_init(&aptr, ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string1);
}

static void test_atomic_ptr_is_lock_free() {
    CHECKPOINT();
    (void) atomic_ptr_is_lock_free(&aptr);
}

/******************************************************/

static void test_atomic_ptr_fixture(void (*test)()) {
    atomic_ptr_init(&aptr, ptrtest.string1);
    test();
}

static void test_atomic_ptr_store_explicit() {
    CHECKPOINT();
    atomic_ptr_store_explicit(&aptr, ptrtest.string2, memory_order_relaxed);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string2);
}

static void test_atomic_ptr_store() {
    CHECKPOINT();
    atomic_ptr_store(&aptr, ptrtest.string2);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string2);
}

static void test_atomic_ptr_load_explicit() {
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string1);
}

static void test_atomic_ptr_load() {
    ASSERT(atomic_ptr_load(&aptr) == ptrtest.string1);
}

static void test_atomic_ptr_exchange_explicit() {
    CHECKPOINT();
    void *ptr = atomic_ptr_exchange_explicit(&aptr, ptrtest.string2, memory_order_relaxed);
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string2);
}

static void test_atomic_ptr_exchange() {
    CHECKPOINT();
    void *ptr = atomic_ptr_exchange(&aptr, ptrtest.string2);
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string2);
}

static void test_atomic_ptr_compare_exchange_strong_explicit() {
    void *ptr = ptrtest.string2;
    ASSERT(!atomic_ptr_compare_exchange_strong_explicit(&aptr, &ptr, ptrtest.string3, memory_order_relaxed, memory_order_relaxed));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_compare_exchange_strong_explicit(&aptr, &ptr, ptrtest.string3, memory_order_relaxed, memory_order_relaxed));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string3);
}

static void test_atomic_ptr_compare_exchange_strong() {
    void *ptr = ptrtest.string2;
    ASSERT(!atomic_ptr_compare_exchange_strong(&aptr, &ptr, ptrtest.string3));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_compare_exchange_strong(&aptr, &ptr, ptrtest.string3));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string3);
}

static void test_atomic_ptr_compare_exchange_weak_explicit() {
    void *ptr = ptrtest.string2;
    ASSERT(!atomic_ptr_compare_exchange_weak_explicit(&aptr, &ptr, ptrtest.string3, memory_order_relaxed, memory_order_relaxed));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_compare_exchange_weak_explicit(&aptr, &ptr, ptrtest.string3, memory_order_relaxed, memory_order_relaxed));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string3);
}

static void test_atomic_ptr_compare_exchange_weak() {
    void *ptr = ptrtest.string2;
    ASSERT(!atomic_ptr_compare_exchange_weak(&aptr, &ptr, ptrtest.string3));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_compare_exchange_weak(&aptr, &ptr, ptrtest.string3));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string3);
}

static void test_atomic_ptr_fetch_add_explicit() {
    ASSERT(atomic_ptr_fetch_add_explicit(&aptr, 4, memory_order_relaxed) == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == &ptrtest.string1[4]);
}

static void test_atomic_ptr_fetch_add() {
    ASSERT(atomic_ptr_fetch_add(&aptr, 4) == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == &ptrtest.string1[4]);
}

static void test_atomic_ptr_fetch_or_explicit() {
    ASSERT(atomic_ptr_fetch_or_explicit(&aptr, 0x1, memory_order_relaxed) == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == &ptrtest.string1[1]);
}

static void test_atomic_ptr_fetch_or() {
    ASSERT(atomic_ptr_fetch_or(&aptr, 0x1) == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == &ptrtest.string1[1]);
}

/********************************************************/

static void test_atomic_ptr_offset_fixture(void (*test)()) {
    atomic_ptr_init(&aptr, &ptrtest.string1[2]);
    test();
}

static void test_atomic_ptr_fetch_sub_explicit() {
    ASSERT(atomic_ptr_fetch_sub_explicit(&aptr, 2, memory_order_relaxed) == &ptrtest.string1[2]);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string1);
}

static void test_atomic_ptr_fetch_sub() {
    ASSERT(atomic_ptr_fetch_sub(&aptr, 2) == &ptrtest.string1[2]);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string1);
}

static void test_atomic_ptr_fetch_xor_explicit() {
    ASSERT(atomic_ptr_fetch_xor_explicit(&aptr, 0x3, memory_order_relaxed) == &ptrtest.string1[2]);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == &ptrtest.string1[1]);
}

static void test_atomic_ptr_fetch_xor() {
    ASSERT(atomic_ptr_fetch_xor(&aptr, 0x3) == &ptrtest.string1[2]);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == &ptrtest.string1[1]);
}

static void test_atomic_ptr_fetch_and_explicit() {
    ASSERT(atomic_ptr_fetch_and_explicit(&aptr, ~((uintptr_t) 7), memory_order_relaxed) == &ptrtest.string1[2]);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string1);
}

static void test_atomic_ptr_fetch_and() {
    ASSERT(atomic_ptr_fetch_and(&aptr, ~((uintptr_t) 7)) == &ptrtest.string1[2]);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string1);
}

int run_atomic_pointer_h_test_suite() {
    int r;
    void (*atomic_ptr_blank_tests[])() = { test_atomic_ptr_init, test_atomic_ptr_is_lock_free, NULL };
    char *atomic_ptr_blank_test_names[] = { "atomic_ptr_init", "atomic_ptr_is_lock_free", NULL };

    r = run_test_suite(test_atomic_ptr_blank_fixture, atomic_ptr_blank_test_names, atomic_ptr_blank_tests);
    if(r != 0) {
	return r;
    }

    void (*atomic_ptr_tests[])() = { test_atomic_ptr_store_explicit, test_atomic_ptr_store,
				     test_atomic_ptr_load_explicit, test_atomic_ptr_load,
				     test_atomic_ptr_exchange_explicit, test_atomic_ptr_exchange,
				     test_atomic_ptr_compare_exchange_strong_explicit,
				     test_atomic_ptr_compare_exchange_strong,
				     test_atomic_ptr_compare_exchange_weak_explicit,
				     test_atomic_ptr_compare_exchange_weak,
				     test_atomic_ptr_fetch_add_explicit,
				     test_atomic_ptr_fetch_add,
				     test_atomic_ptr_fetch_or_explicit,
				     test_atomic_ptr_fetch_or, NULL };
    char *atomic_ptr_test_names[] = { "atomic_ptr_store_explicit", "atomic_ptr_store",
				      "atomic_ptr_load_explicit", "atomic_ptr_load",
				      "atomic_ptr_exchange_explicit", "atomic_ptr_exchange",
				      "atomic_ptr_compare_exchange_strong_explicit",
				      "atomic_ptr_compare_exchange_strong",
				      "atomic_ptr_compare_exchange_weak_explicit",
				      "atomic_ptr_compare_exchange_weak",
				      "atomic_ptr_fetch_add_explicit",
				      "atomic_ptr_fetch_add",
				      "atomic_ptr_fetch_or_explicit",
				      "atomic_ptr_fetch_or", NULL };
    r = run_test_suite(test_atomic_ptr_fixture, atomic_ptr_test_names, atomic_ptr_tests);
    if(r != 0) {
	return r;
    }

    void (*atomic_ptr_offset_tests[])() = { test_atomic_ptr_fetch_sub_explicit,
					    test_atomic_ptr_fetch_sub,
					    test_atomic_ptr_fetch_xor_explicit,
					    test_atomic_ptr_fetch_xor,
					    test_atomic_ptr_fetch_and_explicit,
					    test_atomic_ptr_fetch_and, NULL };
    char *atomic_ptr_offset_test_names[] = { "atomic_ptr_fetch_sub_explicit",
					     "atomic_ptr_fetch_sub",
					     "atomic_ptr_fetch_xor_explicit",
					     "atomic_ptr_fetch_xor",
					     "atomic_ptr_fetch_and_explicit",
					     "atomic_ptr_fetch_and", NULL };
    r = run_test_suite(test_atomic_ptr_offset_fixture, atomic_ptr_offset_test_names, atomic_ptr_offset_tests);
    if(r != 0) {
	return r;
    }

    return 0;
}
