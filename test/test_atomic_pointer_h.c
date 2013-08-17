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
#include <string.h>
#include "config.h"

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca(size_t);
#endif

#include <atomickit/atomic-pointer.h>
#include "test.h"

static struct {
    char string1[14];
    char string2[14];
    char string3[14];
} strtest = {"Test String 1", "Test String 2", "Test String 3"};

char *string1 __attribute__((aligned(8)));
char *string2 __attribute__((aligned(8)));
char *string3 __attribute__((aligned(8)));

static volatile atomic_ptr aptr = ATOMIC_PTR_VAR_INIT(NULL);

static void test_atomic_ptr_strinit_fixture(void (*test)()) {
    string1 = alloca(strlen(strtest.string1) + 1);
    strcpy(string1, strtest.string1);
    string2 = alloca(strlen(strtest.string2) + 1);
    strcpy(string2, strtest.string2);
    string3 = alloca(strlen(strtest.string3) + 1);
    strcpy(string3, strtest.string3);
    test();
}

static void test_atomic_ptr_init() {
    CHECKPOINT();
    atomic_ptr_init(&aptr, string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string1);
}

static void test_atomic_ptr_is_lock_free() {
    CHECKPOINT();
    (void) atomic_ptr_is_lock_free(&aptr);
}

/******************************************************/

static void test_atomic_ptr_fixture(void (*test)()) {
    string1 = alloca(strlen(strtest.string1) + 1);
    strcpy(string1, strtest.string1);
    string2 = alloca(strlen(strtest.string2) + 1);
    strcpy(string2, strtest.string2);
    string3 = alloca(strlen(strtest.string3) + 1);
    strcpy(string3, strtest.string3);
    atomic_ptr_init(&aptr, string1);
    test();
}

static void test_atomic_ptr_store_explicit() {
    CHECKPOINT();
    atomic_ptr_store_explicit(&aptr, string2, memory_order_relaxed);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(strcmp(string2, strtest.string2) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string2);
}

static void test_atomic_ptr_store() {
    CHECKPOINT();
    atomic_ptr_store(&aptr, string2);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(strcmp(string2, strtest.string2) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string2);
}

static void test_atomic_ptr_load_explicit() {
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
}

static void test_atomic_ptr_load() {
    ASSERT(atomic_ptr_load(&aptr) == string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
}

static void test_atomic_ptr_exchange_explicit() {
    CHECKPOINT();
    void *ptr = atomic_ptr_exchange_explicit(&aptr, string2, memory_order_relaxed);
    ASSERT(ptr == string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(strcmp(string2, strtest.string2) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string2);
}

static void test_atomic_ptr_exchange() {
    CHECKPOINT();
    void *ptr = atomic_ptr_exchange(&aptr, string2);
    ASSERT(ptr == string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(strcmp(string2, strtest.string2) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string2);
}

static void test_atomic_ptr_compare_exchange_strong_explicit() {
    void *ptr = string2;
    ASSERT(!atomic_ptr_compare_exchange_strong_explicit(&aptr, &ptr, string3, memory_order_relaxed, memory_order_relaxed));
    ASSERT(ptr == string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(strcmp(string2, strtest.string2) == 0);
    ASSERT(strcmp(string3, strtest.string3) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string1);
    ASSERT(atomic_ptr_compare_exchange_strong_explicit(&aptr, &ptr, string3, memory_order_relaxed, memory_order_relaxed));
    ASSERT(ptr == string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(strcmp(string2, strtest.string2) == 0);
    ASSERT(strcmp(string3, strtest.string3) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string3);
}

static void test_atomic_ptr_compare_exchange_strong() {
    void *ptr = string2;
    ASSERT(!atomic_ptr_compare_exchange_strong(&aptr, &ptr, string3));
    ASSERT(ptr == string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(strcmp(string2, strtest.string2) == 0);
    ASSERT(strcmp(string3, strtest.string3) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string1);
    ASSERT(atomic_ptr_compare_exchange_strong(&aptr, &ptr, string3));
    ASSERT(ptr == string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(strcmp(string2, strtest.string2) == 0);
    ASSERT(strcmp(string3, strtest.string3) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string3);
}

static void test_atomic_ptr_compare_exchange_weak_explicit() {
    void *ptr = string2;
    ASSERT(!atomic_ptr_compare_exchange_weak_explicit(&aptr, &ptr, string3, memory_order_relaxed, memory_order_relaxed));
    ASSERT(ptr == string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(strcmp(string2, strtest.string2) == 0);
    ASSERT(strcmp(string3, strtest.string3) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string1);
    ASSERT(atomic_ptr_compare_exchange_weak_explicit(&aptr, &ptr, string3, memory_order_relaxed, memory_order_relaxed));
    ASSERT(ptr == string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(strcmp(string2, strtest.string2) == 0);
    ASSERT(strcmp(string3, strtest.string3) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string3);
}

static void test_atomic_ptr_compare_exchange_weak() {
    void *ptr = string2;
    ASSERT(!atomic_ptr_compare_exchange_weak(&aptr, &ptr, string3));
    ASSERT(ptr == string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(strcmp(string2, strtest.string2) == 0);
    ASSERT(strcmp(string3, strtest.string3) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string1);
    ASSERT(atomic_ptr_compare_exchange_weak(&aptr, &ptr, string3));
    ASSERT(ptr == string1);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(strcmp(string2, strtest.string2) == 0);
    ASSERT(strcmp(string3, strtest.string3) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string3);
}

static void test_atomic_ptr_fetch_add_explicit() {
    ASSERT(atomic_ptr_fetch_add_explicit(&aptr, 4, memory_order_relaxed) == string1);
    ASSERT(strcmp(string1, string1) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == &string1[4]);
}

static void test_atomic_ptr_fetch_add() {
    ASSERT(atomic_ptr_fetch_add(&aptr, 4) == string1);
    ASSERT(strcmp(string1, string1) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == &string1[4]);
}

/********************************************************/

static void test_atomic_ptr_offset_fixture(void (*test)()) {
    string1 = alloca(strlen(strtest.string1) + 1);
    strcpy(string1, strtest.string1);
    string2 = alloca(strlen(strtest.string2) + 1);
    strcpy(string2, strtest.string2);
    string3 = alloca(strlen(strtest.string3) + 1);
    strcpy(string3, strtest.string3);
    atomic_ptr_init(&aptr, &string1[2]);
    test();
}

static void test_atomic_ptr_fetch_sub_explicit() {
    ASSERT(atomic_ptr_fetch_sub_explicit(&aptr, 2, memory_order_relaxed) == &string1[2]);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string1);
}

static void test_atomic_ptr_fetch_sub() {
    ASSERT(atomic_ptr_fetch_sub(&aptr, 2) == &string1[2]);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string1);
}

static void test_atomic_ptr_fetch_or_explicit() {
    ASSERT(atomic_ptr_fetch_or_explicit(&aptr, 0x1, memory_order_relaxed) == &string1[2]);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == &string1[3]);
}

static void test_atomic_ptr_fetch_or() {
    ASSERT(atomic_ptr_fetch_or(&aptr, 0x1) == &string1[2]);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == &string1[3]);
}

static void test_atomic_ptr_fetch_xor_explicit() {
    ASSERT(atomic_ptr_fetch_xor_explicit(&aptr, 0x3, memory_order_relaxed) == &string1[2]);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == &string1[1]);
}

static void test_atomic_ptr_fetch_xor() {
    ASSERT(atomic_ptr_fetch_xor(&aptr, 0x3) == &string1[2]);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == &string1[1]);
}

static void test_atomic_ptr_fetch_and_explicit() {
    ASSERT(atomic_ptr_fetch_and_explicit(&aptr, ~((uintptr_t) 7), memory_order_relaxed) == &string1[2]);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string1);
}

static void test_atomic_ptr_fetch_and() {
    ASSERT(atomic_ptr_fetch_and(&aptr, ~((uintptr_t) 7)) == &string1[2]);
    ASSERT(strcmp(string1, strtest.string1) == 0);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == string1);
}

int run_atomic_pointer_h_test_suite() {
    int r;
    void (*atomic_ptr_blank_tests[])() = { test_atomic_ptr_init, test_atomic_ptr_is_lock_free, NULL };
    char *atomic_ptr_blank_test_names[] = { "atomic_ptr_init", "atomic_ptr_is_lock_free", NULL };

    r = run_test_suite(test_atomic_ptr_strinit_fixture, atomic_ptr_blank_test_names, atomic_ptr_blank_tests);
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
				     test_atomic_ptr_fetch_add, NULL };
    char *atomic_ptr_test_names[] = { "atomic_ptr_store_explicit", "atomic_ptr_store",
				      "atomic_ptr_load_explicit", "atomic_ptr_load",
				      "atomic_ptr_exchange_explicit", "atomic_ptr_exchange",
				      "atomic_ptr_compare_exchange_strong_explicit",
				      "atomic_ptr_compare_exchange_strong",
				      "atomic_ptr_compare_exchange_weak_explicit",
				      "atomic_ptr_compare_exchange_weak",
				      "atomic_ptr_fetch_add_explicit",
				      "atomic_ptr_fetch_add", NULL };
    r = run_test_suite(test_atomic_ptr_fixture, atomic_ptr_test_names, atomic_ptr_tests);
    if(r != 0) {
	return r;
    }

    void (*atomic_ptr_offset_tests[])() = { test_atomic_ptr_fetch_sub_explicit,
					    test_atomic_ptr_fetch_sub,
					    test_atomic_ptr_fetch_or_explicit,
					    test_atomic_ptr_fetch_or,
					    test_atomic_ptr_fetch_xor_explicit,
					    test_atomic_ptr_fetch_xor,
					    test_atomic_ptr_fetch_and_explicit,
					    test_atomic_ptr_fetch_and, NULL };
    char *atomic_ptr_offset_test_names[] = { "atomic_ptr_fetch_sub_explicit",
					     "atomic_ptr_fetch_sub",
					     "atomic_ptr_fetch_or_explicit",
					     "atomic_ptr_fetch_or",
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
