/*
 * test_atomic_txn_h.c
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

#include <atomickit/atomic-txn.h>
#include "test.h"

static struct {
    char __attribute__((aligned(8))) string1[14];
    char __attribute__((aligned(8))) string2[14];
} ptrtest = { "Test String 1", "Test String 2" };

static bool item1_destroyed = false;
static bool item2_destroyed = false;

static void destroy_item1(struct atxn_item *item __attribute__((unused))) {
    CHECKPOINT();
    item1_destroyed = true;
}

static void destroy_item2(struct atxn_item *item __attribute__((unused))) {
    CHECKPOINT();
    item2_destroyed = true;
}

static atxn_t atxn;

static struct atxn_item *item1;
static struct atxn_item *item2;

static void *item1_ptr;
static void *item2_ptr;

/*************************/
static void test_atxn_uninit_fixture(void (*test)()) {
    item1 = alloca(sizeof(struct atxn_item) + 14 - 1);
    item2 = alloca(sizeof(struct atxn_item) + 14 - 1);
    strcpy((char *) item1->data, ptrtest.string1);
    strcpy((char *) item2->data, ptrtest.string2);
    test();
}

static void test_atxn_item_init() {
    item1_ptr = atxn_item_init(item1, destroy_item1);
    ASSERT(item1->destroy == destroy_item1);
    ASSERT(&item1->data == item1_ptr);
    ASSERT(!item1_destroyed);
}

/****************************/
static void test_atxn_init_item_fixture(void (*test)()) {
    item1 = alloca(sizeof(struct atxn_item) + 14 - 1);
    item2 = alloca(sizeof(struct atxn_item) + 14 - 1);
    strcpy((char *) item1->data, ptrtest.string1);
    strcpy((char *) item2->data, ptrtest.string2);
    item1_ptr = atxn_item_init(item1, destroy_item1);
    item2_ptr = atxn_item_init(item2, destroy_item2);
    test();
}

static void test_atxn_init() {
    CHECKPOINT();
    atxn_init(&atxn, item1_ptr);
    ASSERT(!item1_destroyed);
    void *ptr1 = atxn_acquire(&atxn);
    ASSERT(ptr1 == item1_ptr);
    ASSERT(strcmp(ptr1, ptrtest.string1) == 0);
}

/****************************/

static void test_atxn_init_fixture(void (*test)()) {
    item1 = alloca(sizeof(struct atxn_item) + 14 - 1);
    item2 = alloca(sizeof(struct atxn_item) + 14 - 1);
    strcpy((char *) item1->data, ptrtest.string1);
    strcpy((char *) item2->data, ptrtest.string2);
    item1_ptr = atxn_item_init(item1, destroy_item1);
    item2_ptr = atxn_item_init(item2, destroy_item2);
    atxn_init(&atxn, item1_ptr);
    test();
}


static void test_atxn_destroy() {
    CHECKPOINT();
    atxn_destroy(&atxn);
    ASSERT(!item1_destroyed);
    atxn_init(&atxn, item1_ptr);
    CHECKPOINT();
    atxn_item_release(item1_ptr);
    CHECKPOINT();
    atxn_destroy(&atxn);
    ASSERT(item1_destroyed);
}

static void test_atxn_acquire() {
    CHECKPOINT();
    void *ptr1 = atxn_acquire(&atxn);
    CHECKPOINT();
    void *ptr2 = atxn_acquire(&atxn);
    ASSERT(!item1_destroyed);
    ASSERT(ptr1 == item1_ptr);
    ASSERT(ptr2 == item1_ptr);
    ASSERT(strcmp(ptr1, ptr2) == 0);
    ASSERT(strcmp(ptr1, ptrtest.string1) == 0);
    atxn_release(&atxn, ptr1);
    CHECKPOINT();
    atxn_release(&atxn, ptr2);
    ASSERT(!item1_destroyed);
}

static void test_atxn_item_release() {
    atxn_item_release(item1_ptr);
    ASSERT(!item1_destroyed);
    item1_ptr = atxn_acquire(&atxn);
    CHECKPOINT();
    atxn_destroy(&atxn);
    CHECKPOINT();
    atxn_item_release(item1_ptr);
    ASSERT(item1_destroyed);
}

static void test_atxn_commit() {
    /* succeed */
    ASSERT(atxn_commit(&atxn, item1_ptr, item2_ptr));
    ASSERT(!item1_destroyed);
    ASSERT(!item2_destroyed);
    void *ptr1 = atxn_acquire(&atxn);
    ASSERT(ptr1 == item2_ptr);
    ASSERT(strcmp(ptr1, ptrtest.string2) == 0);
      /* fail */
    ASSERT(!atxn_commit(&atxn, item1_ptr, item1_ptr));
    ASSERT(!item1_destroyed);
    ASSERT(!item2_destroyed);
    ptr1 = atxn_acquire(&atxn);
    ASSERT(ptr1 == item2_ptr);
    ASSERT(strcmp(ptr1, ptrtest.string2) == 0);
}

static void test_atxn_check() {
    ASSERT(atxn_check(&atxn, item1_ptr));
    ASSERT(atxn_commit(&atxn, item1_ptr, item2_ptr));
    ASSERT(!atxn_check(&atxn, item1_ptr));
}

static void test_atxn_release() {
    CHECKPOINT();
    atxn_item_release(item1_ptr);
    /* current */
    CHECKPOINT();
    void *ptr1 = atxn_acquire(&atxn);
    CHECKPOINT();
    void *ptr2 = atxn_acquire(&atxn);
    CHECKPOINT();
    atxn_release(&atxn, ptr1);
    ASSERT(!item1_destroyed);
    ASSERT(!item2_destroyed);
    atxn_release(&atxn, ptr2);
    ASSERT(!item1_destroyed);
    ASSERT(!item2_destroyed);
    /* not current */
    ptr1 = atxn_acquire(&atxn);
    CHECKPOINT();
    ptr2 = atxn_acquire(&atxn);
    CHECKPOINT();
    ASSERT(atxn_commit(&atxn, item1_ptr, item2_ptr));
    CHECKPOINT();
    atxn_release(&atxn, ptr1);
    ASSERT(!item1_destroyed);
    ASSERT(!item2_destroyed);
    atxn_release(&atxn, ptr2);
    ASSERT(item1_destroyed);
    ASSERT(!item2_destroyed);
}

int run_atomic_txn_h_test_suite() {
    int r;
    void (*atxn_uninit_tests[])() = { test_atxn_item_init, NULL };
    char *atxn_uninit_test_names[] = { "atxn_item_init", NULL };
    r = run_test_suite(test_atxn_uninit_fixture, atxn_uninit_test_names, atxn_uninit_tests);
    if(r != 0) {
	return r;
    }

    void (*atxn_init_item_tests[])() = { test_atxn_init, NULL };
    char *atxn_init_item_test_names[] = { "atxn_item_init", NULL };
    r = run_test_suite(test_atxn_init_item_fixture, atxn_init_item_test_names, atxn_init_item_tests);
    if(r != 0) {
	return r;
    }

    void (*atxn_init_tests[])() = { test_atxn_destroy, test_atxn_acquire, test_atxn_item_release,
				    test_atxn_commit, test_atxn_check, test_atxn_release, NULL };
    char *atxn_init_test_names[] = { "atxn_destroy", "atxn_acquire", "atxn_item_release",
				     "atxn_commit", "atxn_check", "atxn_release", NULL };
    r = run_test_suite(test_atxn_init_fixture, atxn_init_test_names, atxn_init_tests);
    if(r != 0) {
	return r;
    }

    return 0;
}
