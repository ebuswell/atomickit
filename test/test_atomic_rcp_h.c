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

#include <atomickit/atomic-rcp.h>
#include "test.h"

static struct {
    char __attribute__((aligned(8))) string1[14];
    char __attribute__((aligned(8))) string2[14];
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

static arcp_t arcp;

static struct arcp_region *region1;
static struct arcp_region *region2;

/*************************/

static void test_arcp_uninit_fixture(void (*test)()) {
    region1_destroyed = false;
    region2_destroyed = false;
    region1 = alloca(ARCP_REGION_OVERHEAD + 14);
    region2 = alloca(ARCP_REGION_OVERHEAD + 14);
    strcpy((char *) region1->data, ptrtest.string1);
    strcpy((char *) region2->data, ptrtest.string2);
    CHECKPOINT();
    test();
}

static void test_arcp_region_init() {
    arcp_region_init(region1, destroy_region1);
    ASSERT(region1->header.destroy == destroy_region1);
    ASSERT(strcmp((char *) region1->data, ptrtest.string1) == 0);
    ASSERT(!region1_destroyed);
}

/****************************/

static void test_arcp_init_region_fixture(void (*test)()) {
    region1_destroyed = false;
    region2_destroyed = false;
    region1 = alloca(ARCP_REGION_OVERHEAD + 14);
    region2 = alloca(ARCP_REGION_OVERHEAD + 14);
    strcpy((char *) region1->data, ptrtest.string1);
    strcpy((char *) region2->data, ptrtest.string2);
    CHECKPOINT();
    arcp_region_init(region1, destroy_region1);
    CHECKPOINT();
    arcp_region_init(region2, destroy_region2);
    CHECKPOINT();
    test();
}

static void test_arcp_init() {
    CHECKPOINT();
    arcp_init(&arcp, region1);
    ASSERT(!region1_destroyed);
    struct arcp_region *rg = arcp_load_weak(&arcp);
    ASSERT(rg == region1);
    ASSERT(strcmp((char *) rg->data, ptrtest.string1) == 0);
}

static void test_arcp_incref() {
    CHECKPOINT();
    arcp_incref(region1);
    CHECKPOINT();
    arcp_release(region1);
    ASSERT(!region1_destroyed);
    arcp_release(region1);
    ASSERT(region1_destroyed);
}

/****************************/

static void test_arcp_init_fixture(void (*test)()) {
    region1_destroyed = false;
    region2_destroyed = false;
    region1 = alloca(ARCP_REGION_OVERHEAD + 14);
    region2 = alloca(ARCP_REGION_OVERHEAD + 14);
    strcpy((char *) region1->data, ptrtest.string1);
    strcpy((char *) region2->data, ptrtest.string2);
    CHECKPOINT();
    arcp_region_init(region1, destroy_region1);
    CHECKPOINT();
    arcp_region_init(region2, destroy_region2);
    CHECKPOINT();
    arcp_init(&arcp, region1);
    test();
}

static void test_arcp_load() {
    CHECKPOINT();
    struct arcp_region *rg1 = arcp_load(&arcp);
    CHECKPOINT();
    struct arcp_region *rg2 = arcp_load(&arcp);
    ASSERT(!region1_destroyed);
    ASSERT(rg1 == region1);
    ASSERT(rg2 == region1);
    ASSERT(strcmp((char *) rg1->data, ptrtest.string1) == 0);
    arcp_release(rg1);
    CHECKPOINT();
    arcp_release(rg2);
    ASSERT(!region1_destroyed);
}

#include <stdio.h>

static void test_arcp_load_weak() {
    CHECKPOINT();
    struct arcp_region *rg1 = arcp_load_weak(&arcp);
    ASSERT(!region1_destroyed);
    ASSERT(rg1 == region1);
    ASSERT(strcmp((char *) rg1->data, ptrtest.string1) == 0);
    arcp_release(region1);
    ASSERT(!region1_destroyed);
    arcp_store(&arcp, NULL);
    ASSERT(region1_destroyed);
}

static void test_arcp_release() {
    arcp_release(region1);
    ASSERT(!region1_destroyed);
    region1 = arcp_load(&arcp);
    CHECKPOINT();
    arcp_store(&arcp, NULL);
    CHECKPOINT();
    arcp_release(region1);
    ASSERT(region1_destroyed);
}

static void test_arcp_compare_exchange() {
    /* succeed */
    ASSERT(arcp_compare_exchange(&arcp, region1, region2));
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    struct arcp_region *rg1 = arcp_load_weak(&arcp);
    ASSERT(rg1 == region2);
    ASSERT(strcmp((char *) rg1->data, ptrtest.string2) == 0);
    /* fail */
    ASSERT(!arcp_compare_exchange(&arcp, region1, region1));
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    rg1 = arcp_load_weak(&arcp);
    ASSERT(rg1 == region2);
    ASSERT(strcmp((char *) rg1->data, ptrtest.string2) == 0);
}

static void test_arcp_store() {
    CHECKPOINT();
    arcp_store(&arcp, region2);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    arcp_release(region2);
    CHECKPOINT();
    arcp_store(&arcp, region1);
    ASSERT(!region1_destroyed);
    ASSERT(region2_destroyed);
}

int run_atomic_rcp_h_test_suite() {
    int r;
    void (*arcp_uninit_tests[])() = { test_arcp_region_init, NULL };
    char *arcp_uninit_test_names[] = { "arcp_region_init", NULL };
    r = run_test_suite(test_arcp_uninit_fixture, arcp_uninit_test_names, arcp_uninit_tests);
    if(r != 0) {
	return r;
    }

    void (*arcp_init_region_tests[])() = { test_arcp_init, test_arcp_incref, NULL };
    char *arcp_init_region_test_names[] = { "arcp_init", "arcp_incref", NULL };
    r = run_test_suite(test_arcp_init_region_fixture, arcp_init_region_test_names, arcp_init_region_tests);
    if(r != 0) {
	return r;
    }

    void (*arcp_init_tests[])() = { test_arcp_load, test_arcp_load_weak,
				    test_arcp_compare_exchange, NULL,
				    test_arcp_store, test_arcp_release, NULL };
    char *arcp_init_test_names[] = { "arcp_load", "arcp_load_weak",
				     "arcp_compare_exchange", "arcp_compare_exchange_release",
				     "arcp_store", "arcp_release", NULL };
    r = run_test_suite(test_arcp_init_fixture, arcp_init_test_names, arcp_init_tests);
    if(r != 0) {
	return r;
    }

    return 0;
}
