/*
 * test_atomic_queue_h.c
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
#include <alloca.h>
#include <atomickit/atomic-rcp.h>
#include <atomickit/atomic-queue.h>
#include "alltests.h"
#include "test.h"

static struct {
    char string1[14];
    char string2[14];
} ptrtest = { "Test String 1", "Test String 2" };

static bool region1_destroyed;
static bool region2_destroyed;

static void destroy_region1(struct arcp_region *region __attribute__((unused))) {
    CHECKPOINT();
    region1_destroyed = true;
}

static void destroy_region2(struct arcp_region *region __attribute__((unused))) {
    CHECKPOINT();
    region2_destroyed = true;
}

static struct arcp_region *region1;
static struct arcp_region *region2;

static aqueue_t aqueue;

/****************************/
static void test_aqueue_init() {
    int r;
    struct arcp_region *rg1;
    CHECKPOINT();
    r = aqueue_init(&aqueue);
    ASSERT(r == 0);
    rg1 = aqueue_deq(&aqueue);
    ASSERT(rg1 == NULL);
}

/****************************/

static void test_aqueue_init_fixture(void (*test)()) {
    int r;
    region1_destroyed = false;
    region2_destroyed = false;
    region1 = alloca(ARCP_REGION_OVERHEAD + 14);
    region2 = alloca(ARCP_REGION_OVERHEAD + 14);
    strcpy((char *) ARCP_REGION2DATA(region1), ptrtest.string1);
    strcpy((char *) ARCP_REGION2DATA(region2), ptrtest.string2);
    arcp_region_init(region1, destroy_region1);
    arcp_region_init(region2, destroy_region2);
    r = aqueue_init(&aqueue);
    if(r != 0) {
	UNRESOLVED("aqueue_init failed");
    }
    test();
}

static void test_aqueue_destroy_empty() {
    CHECKPOINT();
    aqueue_destroy(&aqueue);
}

static void test_aqueue_enq() {
    int r;
    CHECKPOINT();
    r = aqueue_enq(&aqueue, region1);
    ASSERT(r == 0);
    r = aqueue_enq(&aqueue, region2);
    ASSERT(r == 0);
    arcp_release(region1);
    CHECKPOINT();
    arcp_release(region2);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    region1 = aqueue_deq(&aqueue);
    CHECKPOINT();
    region2 = aqueue_deq(&aqueue);
    CHECKPOINT();
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), ptrtest.string1) == 0);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), ptrtest.string2) == 0);
}

/****************************/

static void test_aqueue_full_fixture(void (*test)()) {
    int r;
    region1_destroyed = false;
    region2_destroyed = false;
    region1 = alloca(ARCP_REGION_OVERHEAD + 14);
    region2 = alloca(ARCP_REGION_OVERHEAD + 14);
    strcpy((char *) ARCP_REGION2DATA(region1), ptrtest.string1);
    strcpy((char *) ARCP_REGION2DATA(region2), ptrtest.string2);
    arcp_region_init(region1, destroy_region1);
    arcp_region_init(region2, destroy_region2);
    r = aqueue_init(&aqueue);
    if(r != 0) {
	UNRESOLVED("aqueue_init failed");
    }
    r = aqueue_enq(&aqueue, region1);
    if(r != 0) {
	UNRESOLVED("aqueue_enq failed");
    }
    r = aqueue_enq(&aqueue, region2);
    if(r != 0) {
	UNRESOLVED("aqueue_enq failed");
    }
    test();
}

static void test_aqueue_destroy_full() {
    CHECKPOINT();
    arcp_release(region1);
    CHECKPOINT();
    aqueue_destroy(&aqueue);
    ASSERT(region1_destroyed);
    ASSERT(!region2_destroyed);
}

static void test_aqueue_deq() {
    void *nope;
    arcp_release(region1);
    CHECKPOINT();
    arcp_release(region2);
    CHECKPOINT();
    region1 = aqueue_deq(&aqueue);
    ASSERT(!region1_destroyed);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), ptrtest.string1) == 0);
    region2 = aqueue_deq(&aqueue);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), ptrtest.string2) == 0);
    ASSERT(!region2_destroyed);
    nope = aqueue_deq(&aqueue);
    ASSERT(nope == NULL);
}

static void test_aqueue_compare_deq() {
    CHECKPOINT();
    arcp_release(region2);
    ASSERT(aqueue_compare_deq(&aqueue, region1));
    ASSERT(!region1_destroyed);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), ptrtest.string1) == 0);
    region2 = aqueue_peek(&aqueue);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), ptrtest.string2) == 0);
    arcp_release(region2);
    ASSERT(!aqueue_compare_deq(&aqueue, region1));
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    region2 = aqueue_peek(&aqueue);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), ptrtest.string2) == 0);
    arcp_release(region2);
    ASSERT(!region2_destroyed);
    ASSERT(aqueue_compare_deq(&aqueue, region2));
    ASSERT(region2_destroyed);
    ASSERT(aqueue_peek(&aqueue) == NULL);
    ASSERT(!aqueue_compare_deq(&aqueue, NULL));
}

static void test_aqueue_peek() {
    struct arcp_region *region1_2;
    arcp_release(region1);
    CHECKPOINT();
    arcp_release(region2);
    CHECKPOINT();
    region1 = aqueue_peek(&aqueue);
    ASSERT(!region1_destroyed);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), ptrtest.string1) == 0);
    arcp_release(region1);
    CHECKPOINT();
    region1_2 = aqueue_peek(&aqueue);
    ASSERT(!region1_destroyed);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1_2), ptrtest.string1) == 0);
    arcp_release(region1_2);
    CHECKPOINT();
    if(!aqueue_compare_deq(&aqueue, region1)) {
	UNRESOLVED("aqueue_compare_deq failed");
    }
    CHECKPOINT();
    ASSERT(region1_destroyed);
}

int run_atomic_queue_h_test_suite() {
    int r;
    void (*void_tests[])() = { test_aqueue_init, NULL };
    char *void_test_names[] = { "aqueue_init", NULL };

    void (*aqueue_init_tests[])() = { test_aqueue_destroy_empty, test_aqueue_enq,
				      NULL };
    char *aqueue_init_test_names[] = { "aqueue_destroy_empty", "aqueue_enq",
				       NULL };

    void (*aqueue_full_tests[])() = { test_aqueue_destroy_full,
				      test_aqueue_deq,
				      test_aqueue_compare_deq,
				      test_aqueue_peek, NULL };
    char *aqueue_full_test_names[] = { "aqueue_destroy_full",
				       "aqueue_deq",
				       "aqueue_compare_deq",
				       "aqueue_peek", NULL };

    r = run_test_suite(NULL, void_test_names, void_tests);
    if(r != 0) {
	return r;
    }

    r = run_test_suite(test_aqueue_init_fixture, aqueue_init_test_names, aqueue_init_tests);
    if(r != 0) {
	return r;
    }

    r = run_test_suite(test_aqueue_full_fixture, aqueue_full_test_names, aqueue_full_tests);
    if(r != 0) {
	return r;
    }

    return 0;
}
