/*
 * test_atomic_malloc_h.c
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

#include <atomickit/atomic-mtxn.h>
#include "test.h"

static struct {
    char __attribute__((aligned(8))) string1[14];
    char __attribute__((aligned(8))) string2[14];
    char __attribute__((aligned(8))) string3[14];
} ptrtest = { "Test String 1", "Test String 2", "Test String 3" };

static bool item1_destroyed = false;
static bool item2_destroyed = false;
static bool item3_destroyed = false;

static void destroy_item1(struct atxn_item *item __attribute__((unused))) {
    CHECKPOINT();
    item1_destroyed = true;
}

static void destroy_item2(struct atxn_item *item __attribute__((unused))) {
    CHECKPOINT();
    item2_destroyed = true;
}

static void destroy_item3(struct atxn_item *item __attribute__((unused))) {
    CHECKPOINT();
    item3_destroyed = true;
}

static amtxn_t amtxn1;
static amtxn_t amtxn2;

static struct atxn_item *item1;
static struct atxn_item *item2;
static struct atxn_item *item3;

static void *item1_ptr;
static void *item2_ptr;
static void *item3_ptr;

amtxn_handle_t *handle1;
amtxn_handle_t *handle2;

/*************************/

static void test_amtxn_init_item_fixture(void (*test)()) {
    item1 = alloca(ATXN_ITEM_OVERHEAD + 14);
    item2 = alloca(ATXN_ITEM_OVERHEAD + 14);
    item3 = alloca(ATXN_ITEM_OVERHEAD + 14);
    strcpy((char *) item1->data, ptrtest.string1);
    strcpy((char *) item2->data, ptrtest.string2);
    strcpy((char *) item3->data, ptrtest.string3);
    item1_ptr = atxn_item_init(item1, destroy_item1);
    item2_ptr = atxn_item_init(item2, destroy_item2);
    item3_ptr = atxn_item_init(item3, destroy_item3);
    test();
}

void test_amtxn_init() {
    ASSERT(amtxn_init(&amtxn1, item1_ptr) == 0);
    ASSERT(!item1_destroyed);
    void *ptr1 = amtxn_acquire1(&amtxn1);
    ASSERT(ptr1 == item1_ptr);
    ASSERT(strcmp(ptr1, ptrtest.string1) == 0);
}

/*************************/

static void test_amtxn_init_fixture(void (*test)()) {
    item1 = alloca(ATXN_ITEM_OVERHEAD + 14);
    item2 = alloca(ATXN_ITEM_OVERHEAD + 14);
    item3 = alloca(ATXN_ITEM_OVERHEAD + 14);
    strcpy((char *) item1->data, ptrtest.string1);
    strcpy((char *) item2->data, ptrtest.string2);
    strcpy((char *) item3->data, ptrtest.string3);
    item1_ptr = atxn_item_init(item1, destroy_item1);
    item2_ptr = atxn_item_init(item2, destroy_item2);
    item3_ptr = atxn_item_init(item3, destroy_item3);
    int r;
    r = amtxn_init(&amtxn1, item1_ptr);
    if(r != 0) {
	UNRESOLVED("amtxn_init failed");
    }
    r = amtxn_init(&amtxn2, item2_ptr);
    if(r != 0) {
	UNRESOLVED("amtxn_init failed");
    }
    test();
}

static void test_amtxn_destroy() {
    CHECKPOINT();
    amtxn_destroy(&amtxn1);
    ASSERT(!item1_destroyed);
    amtxn_init(&amtxn1, item1_ptr);
    CHECKPOINT();
    amtxn_release1(item1_ptr);
    CHECKPOINT();
    amtxn_destroy(&amtxn1);
    ASSERT(item1_destroyed);
}

static void test_amtxn_acquire1() {
    CHECKPOINT();
    void *ptr1 = amtxn_acquire1(&amtxn1);
    CHECKPOINT();
    void *ptr2 = amtxn_acquire1(&amtxn1);
    ASSERT(!item1_destroyed);
    ASSERT(ptr1 == item1_ptr);
    ASSERT(ptr2 == item1_ptr);
    ASSERT(strcmp(ptr1, ptr2) == 0);
    ASSERT(strcmp(ptr1, ptrtest.string1) == 0);
    amtxn_release1(ptr1);
    CHECKPOINT();
    amtxn_release1(ptr2);
    ASSERT(!item1_destroyed);
}

static void test_amtxn_peek1() {
    CHECKPOINT();
    void *ptr1 = amtxn_peek1(&amtxn1);
    ASSERT(!item1_destroyed);
    ASSERT(ptr1 == item1_ptr);
    ASSERT(strcmp(item1_ptr, ptrtest.string1) == 0);
    amtxn_release1(item1_ptr);
    ASSERT(!item1_destroyed);
    amtxn_destroy(&amtxn1);
    ASSERT(item1_destroyed);
}

static void test_amtxn_release1() {
    amtxn_release1(item1_ptr);
    ASSERT(!item1_destroyed);
    item1_ptr = amtxn_acquire1(&amtxn1);
    CHECKPOINT();
    amtxn_destroy(&amtxn1);
    CHECKPOINT();
    amtxn_release1(item1_ptr);
    ASSERT(item1_destroyed);
}

static void test_amtxn_check1() {
    ASSERT(amtxn_check1(&amtxn1, item1_ptr));
    ASSERT(!amtxn_check1(&amtxn2, item1_ptr));
}

static void test_amtxn_start() {
    CHECKPOINT();
    handle1 = amtxn_start();
    ASSERT(handle1 != NULL);
}

/*************************/
static void test_amtxn_started_fixture(void (*test)()) {
    item1 = alloca(ATXN_ITEM_OVERHEAD + 14);
    item2 = alloca(ATXN_ITEM_OVERHEAD + 14);
    item3 = alloca(ATXN_ITEM_OVERHEAD + 14);
    strcpy((char *) item1->data, ptrtest.string1);
    strcpy((char *) item2->data, ptrtest.string2);
    strcpy((char *) item3->data, ptrtest.string3);
    item1_ptr = atxn_item_init(item1, destroy_item1);
    item2_ptr = atxn_item_init(item2, destroy_item2);
    item3_ptr = atxn_item_init(item3, destroy_item3);
    int r;
    r = amtxn_init(&amtxn1, item1_ptr);
    if(r != 0) {
	UNRESOLVED("amtxn_init failed");
    }
    r = amtxn_init(&amtxn2, item2_ptr);
    if(r != 0) {
	UNRESOLVED("amtxn_init failed");
    }
    handle1 = amtxn_start();
    if(handle1 == NULL) {
	UNRESOLVED("amtxn_start failed");
    }
    handle2 = amtxn_start();
    if(handle2 == NULL) {
	UNRESOLVED("amtxn_start failed");
    }
    test();
}

void test_amtxn_abort() {
    CHECKPOINT();
    void *ptr __attribute__((unused)) = amtxn_acquire(handle1, &amtxn1);
    amtxn_abort(handle1);
    ASSERT(!item1_destroyed);
    amtxn_destroy(&amtxn1);
    ASSERT(!item1_destroyed);
    atxn_release(item1_ptr);
    ASSERT(item1_destroyed);
}

void test_amtxn_status() {
    ASSERT(amtxn_status(handle1) == AMTXN_PENDING);
    void *ptr __attribute__((unused)) = amtxn_acquire(handle1, &amtxn1);
    ASSERT(amtxn_status(handle1) == AMTXN_PENDING);
    ASSERT(amtxn_status(handle2) == AMTXN_PENDING);
    int r = amtxn_set(handle2, &amtxn1, item2_ptr);
    if(r != 0) {
	UNRESOLVED("amtxn_set failed");	
    }
    ASSERT(amtxn_status(handle2) == AMTXN_PENDING);
    if(amtxn_commit(handle2) != AMTXN_SUCCESS) {
	UNRESOLVED("amtxn_commit failed");
    }
    void *ptr2 __attribute__((unused)) = amtxn_acquire(handle1, &amtxn2);
    ASSERT(amtxn_status(handle1) == AMTXN_FAILURE);
}

void test_amtxn_acquire() {
    CHECKPOINT();
    void *ptr1 = amtxn_acquire(handle1, &amtxn1);
    CHECKPOINT();
    void *ptr2 = amtxn_acquire(handle1, &amtxn1);
    ASSERT(!item1_destroyed);
    ASSERT(ptr1 == item1_ptr);
    ASSERT(ptr2 == item1_ptr);
    ASSERT(strcmp(ptr1, ptr2) == 0);
    ASSERT(strcmp(ptr1, ptrtest.string1) == 0);
    void *ptr3 = amtxn_acquire(handle1, &amtxn2);
    ASSERT(!item2_destroyed);
    ASSERT(strcmp(ptr3, ptrtest.string2) == 0);
    int r = amtxn_set(handle1, &amtxn2, item3_ptr);
    if(r != 0) {
	UNRESOLVED("amtxn_set failed");
    }
    void *ptr4 = amtxn_acquire(handle1, &amtxn2);
    ASSERT(!item3_destroyed);
    ASSERT(strcmp(ptr4, ptrtest.string3) == 0);
    amtxn_release1(item1_ptr);
    amtxn_abort(handle1);
    amtxn_destroy(&amtxn1);
    ASSERT(!item3_destroyed);
    ASSERT(!item2_destroyed);
    ASSERT(item1_destroyed);
    amtxn_init(&amtxn1, item2_ptr);
    CHECKPOINT();
    handle1 = amtxn_start();
    if(handle1 == NULL) {
    	UNRESOLVED("amtxn_start failed");
    }
    CHECKPOINT();
    ptr1 = amtxn_acquire(handle1, &amtxn1);
    CHECKPOINT();
    r = amtxn_set(handle2, &amtxn1, item3_ptr);
    if(r != 0) {
    	UNRESOLVED("amtxn_set failed");
    }
    CHECKPOINT();
    amtxn_commit(handle2);
    CHECKPOINT();
    ptr2 = amtxn_acquire(handle1, &amtxn2);
    ASSERT(ptr2 == NULL);
    ASSERT(amtxn_status(handle1) == AMTXN_FAILURE);
}

void test_amtxn_set() {
    CHECKPOINT();
    /* Set is visible to our handle */
    ASSERT(amtxn_set(handle1, &amtxn1, item2_ptr) == 0);
    ASSERT(!item1_destroyed);
    ASSERT(!item2_destroyed);
    void *ptr1 = amtxn_acquire(handle1, &amtxn1);
    ASSERT(ptr1 == item2_ptr);
    ASSERT(strcmp(ptr1, ptrtest.string2) == 0);
    /* A second set is also visible to our handle */
    ASSERT(amtxn_set(handle1, &amtxn1, item3_ptr) == 0);
    ASSERT(!item1_destroyed);
    ASSERT(!item2_destroyed);
    ASSERT(!item3_destroyed);
    void *ptr2 = amtxn_acquire(handle1, &amtxn1);
    ASSERT(ptr2 == item3_ptr);
    ASSERT(strcmp(ptr2, ptrtest.string3) == 0);
    /* This set is not visible to other handles */
    void *ptr3 = amtxn_acquire(handle2, &amtxn1);
    ASSERT(ptr3 == item1_ptr);
    ASSERT(strcmp(ptr3, ptrtest.string1) == 0);
    amtxn_abort(handle2);
    /* A committed set is globally visible */
    if(amtxn_commit(handle1) != AMTXN_SUCCESS) {
    	UNRESOLVED("amtxn_commit failed");
    }
    ASSERT(!item1_destroyed);
    ASSERT(!item2_destroyed);
    ASSERT(!item3_destroyed);
    void *ptr4 = amtxn_acquire1(&amtxn1);
    ASSERT(ptr4 == item3_ptr);
    ASSERT(strcmp(ptr4, ptrtest.string3) == 0);
    amtxn_release1(ptr4);
    /* Now check that the commits cleaned up properly. */
    aqueue_destroy(&amtxn_queue); /* Handle is stuck as first item on
				   * queue; artificially retire it */
    amtxn_release1(item1_ptr);
    amtxn_release1(item2_ptr);
    amtxn_release1(item3_ptr);
    ASSERT(item1_destroyed);
    ASSERT(!item2_destroyed);
    ASSERT(!item3_destroyed);
    amtxn_destroy(&amtxn1);
    ASSERT(item3_destroyed);
    ASSERT(!item2_destroyed);
    amtxn_destroy(&amtxn2);
    ASSERT(item2_destroyed);
}

void test_amtxn_commit() {
    CHECKPOINT();
    void *ptr1 = amtxn_acquire(handle1, &amtxn1);
    ASSERT(ptr1 == item1_ptr);
    int r = amtxn_set(handle1, &amtxn1, item3_ptr);
    if(r != 0) {
	UNRESOLVED("amtxn_set failed");
    }
    void *ptr2 __attribute__((unused)) = amtxn_acquire(handle2, &amtxn1);
    r = amtxn_set(handle2, &amtxn2, item3_ptr);
    if(r != 0) {
	UNRESOLVED("amtxn_set failed");
    }
    ASSERT(amtxn_commit(handle1) == AMTXN_SUCCESS);
    void *ptr3 = amtxn_acquire1(&amtxn1);
    ASSERT(ptr3 == item3_ptr);
    ASSERT(strcmp(ptr3, ptrtest.string3) == 0);
    amtxn_release1(ptr3);
    ASSERT(amtxn_commit(handle2) == AMTXN_FAILURE);
    ptr3 = amtxn_acquire1(&amtxn1);
    ASSERT(ptr3 == item3_ptr);
    ASSERT(strcmp(ptr3, ptrtest.string3) == 0);
    amtxn_release1(ptr3);
    ASSERT(!item1_destroyed);
    ASSERT(!item2_destroyed);
    ASSERT(!item3_destroyed);
    aqueue_destroy(&amtxn_queue); /* Handle is stuck as first item on
				   * queue; artificially retire it */
    amtxn_release1(item1_ptr);
    amtxn_release1(item2_ptr);
    amtxn_release1(item3_ptr);
    ASSERT(item1_destroyed);
    ASSERT(!item2_destroyed);
    ASSERT(!item3_destroyed);
    amtxn_destroy(&amtxn1);
    ASSERT(item3_destroyed);
    ASSERT(!item2_destroyed);
    amtxn_destroy(&amtxn2);
    ASSERT(item2_destroyed);
}

/*************************/
int run_atomic_mtxn_h_test_suite() {
    int r;
    void (*init_item_tests[])() = { test_amtxn_init, NULL };
    char *init_item_test_names[] = { "amtxn_init", NULL };
    r = run_test_suite(test_amtxn_init_item_fixture, init_item_test_names, init_item_tests);
    if(r != 0) {
    	return r;
    }

    void (*init_tests[])() = { test_amtxn_destroy, test_amtxn_acquire1, test_amtxn_peek1,
    			       test_amtxn_check1, test_amtxn_release1, test_amtxn_start,
    			       NULL };
    char *init_test_names[] = { "amtxn_destroy", "amtxn_acquire1", "amtxn_peek1",
    				"amtxn_check1", "amtxn_release1", "amtxn_start",
    				NULL };
    r = run_test_suite(test_amtxn_init_fixture, init_test_names, init_tests);
    if(r != 0) {
    	return r;
    }

    void (*started_tests[])() = { test_amtxn_abort, test_amtxn_status, test_amtxn_acquire,
    				  test_amtxn_set, test_amtxn_commit, NULL };
    char *started_test_names[] = { "amtxn_abort", "amtxn_status", "amtxn_acquire",
    				   "amtxn_set", "amtxn_commit", NULL };
    r = run_test_suite(test_amtxn_started_fixture, started_test_names, started_tests);
    if(r != 0) {
    	return r;
    }

    return 0;
}
