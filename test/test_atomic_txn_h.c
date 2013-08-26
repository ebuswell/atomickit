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

#include <atomickit/atomic-txn.h>
#include "alltests.h"
#include "test.h"

static struct {
    char __attribute__((aligned(8))) string1[14];
    char __attribute__((aligned(8))) string2[14];
    char __attribute__((aligned(8))) string3[14];
} ptrtest = { "Test String 1", "Test String 2", "Test String 3" };

static bool region1_destroyed;
static bool region2_destroyed;
static bool region3_destroyed;

static void destroy_region1(struct arcp_region *region __attribute__((unused))) {
    CHECKPOINT();
    region1_destroyed = true;
}

static void destroy_region2(struct arcp_region *region __attribute__((unused))) {
    CHECKPOINT();
    region2_destroyed = true;
}

static void destroy_region3(struct arcp_region *region __attribute__((unused))) {
    CHECKPOINT();
    region3_destroyed = true;
}

static atxn_t atxn1;
static atxn_t atxn2;

static struct arcp_region *region1;
static struct arcp_region *region2;
static struct arcp_region *region3;

atxn_handle_t *handle1;
atxn_handle_t *handle2;

/*************************/

static void test_atxn_init_region_fixture(void (*test)()) {
    region1_destroyed = false;
    region2_destroyed = false;
    region3_destroyed = false;
    region1 = alloca(ARCP_REGION_OVERHEAD + 14);
    region2 = alloca(ARCP_REGION_OVERHEAD + 14);
    region3 = alloca(ARCP_REGION_OVERHEAD + 14);
    strcpy((char *) region1->data, ptrtest.string1);
    strcpy((char *) region2->data, ptrtest.string2);
    strcpy((char *) region3->data, ptrtest.string3);
    arcp_region_init(region1, destroy_region1);
    arcp_region_init(region2, destroy_region2);
    arcp_region_init(region3, destroy_region3);
    test();
}

static void test_atxn_init() {
    ASSERT(atxn_init(&atxn1, region1) == 0);
    ASSERT(!region1_destroyed);
    struct arcp_region *rg1 = atxn_load1(&atxn1);
    ASSERT(rg1 == region1);
    ASSERT(strcmp((char *) rg1->data, ptrtest.string1) == 0);
    atxn_destroy(&atxn1);
}

/*************************/

static void test_atxn_init_fixture(void (*test)()) {
    region1_destroyed = false;
    region2_destroyed = false;
    region3_destroyed = false;
    region1 = alloca(ARCP_REGION_OVERHEAD + 14);
    region2 = alloca(ARCP_REGION_OVERHEAD + 14);
    region3 = alloca(ARCP_REGION_OVERHEAD + 14);
    strcpy((char *) region1->data, ptrtest.string1);
    strcpy((char *) region2->data, ptrtest.string2);
    strcpy((char *) region3->data, ptrtest.string3);
    arcp_region_init(region1, destroy_region1);
    arcp_region_init(region2, destroy_region2);
    arcp_region_init(region3, destroy_region3);
    int r;
    r = atxn_init(&atxn1, region1);
    if(r != 0) {
	UNRESOLVED("atxn_init failed");
    }
    r = atxn_init(&atxn2, region2);
    if(r != 0) {
	UNRESOLVED("atxn_init failed");
    }
    test();
}

static void test_atxn_destroy() {
    CHECKPOINT();
    atxn_destroy(&atxn1);
    ASSERT(!region1_destroyed);
    atxn_init(&atxn1, region1);
    CHECKPOINT();
    atxn_release1(region1);
    CHECKPOINT();
    atxn_destroy(&atxn1);
    ASSERT(region1_destroyed);
}

static void test_atxn_load1() {
    CHECKPOINT();
    struct arcp_region *rg1 = atxn_load1(&atxn1);
    CHECKPOINT();
    struct arcp_region *rg2 = atxn_load1(&atxn1);
    ASSERT(!region1_destroyed);
    ASSERT(rg1 == region1);
    ASSERT(rg2 == region1);
    ASSERT(strcmp((char *) rg1->data, (char *) rg2->data) == 0);
    ASSERT(strcmp((char *) rg1->data, ptrtest.string1) == 0);
    atxn_release1(rg1);
    CHECKPOINT();
    atxn_release1(rg2);
    ASSERT(!region1_destroyed);
}

static void test_atxn_load_weak1() {
    CHECKPOINT();
    struct arcp_region *rg1 = atxn_load_weak1(&atxn1);
    ASSERT(!region1_destroyed);
    ASSERT(rg1 == region1);
    ASSERT(strcmp((char *) region1->data, ptrtest.string1) == 0);
    atxn_release1(region1);
    ASSERT(!region1_destroyed);
    atxn_destroy(&atxn1);
    ASSERT(region1_destroyed);
}

static void test_atxn_release1() {
    atxn_release1(region1);
    ASSERT(!region1_destroyed);
    region1 = atxn_load1(&atxn1);
    CHECKPOINT();
    atxn_destroy(&atxn1);
    CHECKPOINT();
    atxn_release1(region1);
    ASSERT(region1_destroyed);
}

static void test_atxn_start() {
    CHECKPOINT();
    handle1 = atxn_start();
    ASSERT(handle1 != NULL);
}

/*************************/
#include <stdio.h>
static void test_atxn_started_fixture(void (*test)()) {
    region1_destroyed = false;
    region2_destroyed = false;
    region3_destroyed = false;
    region1 = alloca(ARCP_REGION_OVERHEAD + 14);
    region2 = alloca(ARCP_REGION_OVERHEAD + 14);
    region3 = alloca(ARCP_REGION_OVERHEAD + 14);
    strcpy((char *) region1->data, ptrtest.string1);
    strcpy((char *) region2->data, ptrtest.string2);
    strcpy((char *) region3->data, ptrtest.string3);
    arcp_region_init(region1, destroy_region1);
    arcp_region_init(region2, destroy_region2);
    arcp_region_init(region3, destroy_region3);
    int r;
    r = atxn_init(&atxn1, region1);
    if(r != 0) {
	UNRESOLVED("atxn_init failed");
    }
    r = atxn_init(&atxn2, region2);
    if(r != 0) {
	UNRESOLVED("atxn_init failed");
    }
    handle1 = atxn_start();
    if(handle1 == NULL) {
	UNRESOLVED("atxn_start failed");
    }
    handle2 = atxn_start();
    if(handle2 == NULL) {
	UNRESOLVED("atxn_start failed");
    }
    test();
}

static void test_atxn_abort() {
    CHECKPOINT();
    struct arcp_region *rg;
    int r = atxn_load(handle1, &atxn1, &rg);
    if(r != 0) {
    	UNRESOLVED("atxn_load failed");
    }
    atxn_abort(handle1);
    handle1 = NULL;
    ASSERT(!region1_destroyed);
    atxn_destroy(&atxn1);
    ASSERT(!region1_destroyed);
    arcp_release(region1);
    ASSERT(region1_destroyed);
}

static void test_atxn_status() {
    ASSERT(atxn_status(handle1) == ATXN_PENDING);
    struct arcp_region *rg;
    enum atxn_status r = atxn_load(handle1, &atxn1, &rg);
    if(r != ATXN_SUCCESS) {
	UNRESOLVED("atxn_load failed");
    }
    ASSERT(atxn_status(handle1) == ATXN_PENDING);
    ASSERT(atxn_status(handle2) == ATXN_PENDING);
    r = atxn_store(handle2, &atxn1, region2);
    if(r != ATXN_SUCCESS) {
	UNRESOLVED("atxn_store failed");	
    }
    ASSERT(atxn_status(handle2) == ATXN_PENDING);
    if(atxn_commit(handle2) != ATXN_SUCCESS) {
	UNRESOLVED("atxn_commit failed");
    }
    struct arcp_region *rg2;
    r = atxn_load(handle1, &atxn2, &rg2);
    if(r != ATXN_FAILURE) {
	UNRESOLVED("atxn_load failed");
    }
    ASSERT(atxn_status(handle1) == ATXN_FAILURE);
}

static void test_atxn_load() {
    CHECKPOINT();
    struct arcp_region *rg1;
    enum atxn_status r = atxn_load(handle1, &atxn1, &rg1);
    ASSERT(r == ATXN_SUCCESS);
    struct arcp_region *rg2;
    r = atxn_load(handle1, &atxn1, &rg2);
    ASSERT(r == ATXN_SUCCESS);
    ASSERT(!region1_destroyed);
    ASSERT(rg1 == region1);
    ASSERT(rg2 == region1);
    ASSERT(strcmp((char *) rg1->data, ptrtest.string1) == 0);
    struct arcp_region *rg3;
    r = atxn_load(handle1, &atxn2, &rg3);
    ASSERT(r == ATXN_SUCCESS);
    ASSERT(!region2_destroyed);
    ASSERT(rg3 == region2);
    ASSERT(strcmp((char *) rg3->data, ptrtest.string2) == 0);
    r = atxn_store(handle1, &atxn2, region3);
    if(r != ATXN_SUCCESS) {
	UNRESOLVED("atxn_store failed");
    }
    struct arcp_region *rg4;
    r = atxn_load(handle1, &atxn2, &rg4);
    ASSERT(r == ATXN_SUCCESS);
    ASSERT(!region3_destroyed);
    ASSERT(rg4 == region3);
    ASSERT(strcmp((char *) rg4->data, ptrtest.string3) == 0);
    atxn_release1(region1);
    atxn_abort(handle1);
    atxn_destroy(&atxn1);
    ASSERT(!region3_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(region1_destroyed);
    int rr = atxn_init(&atxn1, region2);
    if(rr != 0) {
	UNRESOLVED("atxn_init failed");
    }
    CHECKPOINT();
    handle1 = atxn_start();
    if(handle1 == NULL) {
    	UNRESOLVED("atxn_start failed");
    }
    CHECKPOINT();
    r = atxn_load(handle1, &atxn1, &rg1);
    ASSERT(r == ATXN_SUCCESS);
    r = atxn_store(handle2, &atxn1, region3);
    if(r != ATXN_SUCCESS) {
    	UNRESOLVED("atxn_store failed");
    }
    CHECKPOINT();
    atxn_commit(handle2);
    CHECKPOINT();
    r = atxn_load(handle1, &atxn2, &rg2);
    ASSERT(r == ATXN_FAILURE);
    ASSERT(atxn_status(handle1) == ATXN_FAILURE);
}

static void test_atxn_store() {
    CHECKPOINT();
    /* Store is visible to our handle */
    ASSERT(atxn_store(handle1, &atxn1, region2) == 0);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    struct arcp_region *rg1;
    enum atxn_status r = atxn_load(handle1, &atxn1, &rg1);
    if(r != ATXN_SUCCESS) {
	UNRESOLVED("atxn_load failed");
    }
    ASSERT(rg1 == region2);
    ASSERT(strcmp((char *) rg1->data, ptrtest.string2) == 0);
    /* A second store is also visible to our handle */
    ASSERT(atxn_store(handle1, &atxn1, region3) == 0);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(!region3_destroyed);
    struct arcp_region *rg2;
    r = atxn_load(handle1, &atxn1, &rg2);
    if(r != ATXN_SUCCESS) {
	UNRESOLVED("atxn_load failed");
    }
    ASSERT(rg2 == region3);
    ASSERT(strcmp((char *) rg2->data, ptrtest.string3) == 0);
    /* This store is not visible to other handles */
    struct arcp_region *rg3;
    r = atxn_load(handle2, &atxn1, &rg3);
    if(r != ATXN_SUCCESS) {
	UNRESOLVED("atxn_load failed");
    }
    ASSERT(rg3 == region1);
    ASSERT(strcmp((char *) rg3->data, ptrtest.string1) == 0);
    atxn_abort(handle2);
    /* A committed store is globally visible */
    if(atxn_commit(handle1) != ATXN_SUCCESS) {
    	UNRESOLVED("atxn_commit failed");
    }
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(!region3_destroyed);
    struct arcp_region *rg4 = atxn_load1(&atxn1);
    ASSERT(rg4 == region3);
    ASSERT(strcmp((char *) rg4->data, ptrtest.string3) == 0);
    atxn_release1(rg4);
    /* Now check that the commits cleaned up properly. */
    atxn_release1(region1);
    atxn_release1(region2);
    atxn_release1(region3);
    ASSERT(region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(!region3_destroyed);
    atxn_destroy(&atxn1);
    ASSERT(region3_destroyed);
    ASSERT(!region2_destroyed);
    atxn_destroy(&atxn2);
    ASSERT(region2_destroyed);
}

static void test_atxn_commit() {
    CHECKPOINT();
    struct arcp_region *rg1;
    enum atxn_status r = atxn_load(handle1, &atxn1, &rg1);
    if(r != ATXN_SUCCESS) {
	UNRESOLVED("atxn_load failed");
    }
    ASSERT(rg1 == region1);
    r = atxn_store(handle1, &atxn1, region3);
    if(r != ATXN_SUCCESS) {
	UNRESOLVED("atxn_store failed");
    }
    struct arcp_region *rg2;
    r = atxn_load(handle2, &atxn1, &rg2);
    if(r != ATXN_SUCCESS) {
	UNRESOLVED("atxn_load failed");
    }
    r = atxn_store(handle2, &atxn2, region3);
    if(r != ATXN_SUCCESS) {
	UNRESOLVED("atxn_store failed");
    }
    ASSERT(atxn_commit(handle1) == ATXN_SUCCESS);
    struct arcp_region *rg3 = atxn_load1(&atxn1);
    ASSERT(rg3 == region3);
    ASSERT(strcmp((char *) rg3->data, ptrtest.string3) == 0);
    atxn_release1(rg3);
    ASSERT(atxn_commit(handle2) == ATXN_FAILURE);
    rg3 = atxn_load1(&atxn1);
    ASSERT(rg3 == region3);
    ASSERT(strcmp((char *) rg3->data, ptrtest.string3) == 0);
    atxn_release1(rg3);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(!region3_destroyed);
    atxn_release1(region1);
    atxn_release1(region2);
    atxn_release1(region3);
    ASSERT(region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(!region3_destroyed);
    atxn_destroy(&atxn1);
    ASSERT(region3_destroyed);
    ASSERT(!region2_destroyed);
    atxn_destroy(&atxn2);
    ASSERT(region2_destroyed);
}

/*************************/
int run_atomic_txn_h_test_suite() {
    int r;
    void (*init_region_tests[])() = { test_atxn_init, NULL };
    char *init_region_test_names[] = { "atxn_init", NULL };
    r = run_test_suite(test_atxn_init_region_fixture, init_region_test_names, init_region_tests);
    if(r != 0) {
    	return r;
    }

    void (*init_tests[])() = { test_atxn_destroy, test_atxn_load1, test_atxn_load_weak1,
    			       test_atxn_release1, test_atxn_start,
    			       NULL };
    char *init_test_names[] = { "atxn_destroy", "atxn_load1", "atxn_load_weak1",
    				"atxn_release1", "atxn_start",
    				NULL };
    r = run_test_suite(test_atxn_init_fixture, init_test_names, init_tests);
    if(r != 0) {
    	return r;
    }

    void (*started_tests[])() = { test_atxn_abort, test_atxn_status, test_atxn_load,
    				  test_atxn_store, test_atxn_commit, NULL };
    char *started_test_names[] = { "atxn_abort", "atxn_status", "atxn_load",
    				   "atxn_store", "atxn_commit", NULL };
    r = run_test_suite(test_atxn_started_fixture, started_test_names, started_tests);
    if(r != 0) {
    	return r;
    }

    return 0;
}
