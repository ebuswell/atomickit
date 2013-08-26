/*
 * test_atomic_rcp_h.c
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
#include "alltests.h"
#include "test.h"

static struct {
    char string1[14];
    char string2[14];
} strtest = { "Test String 1", "Test String 2" };

static bool region1_destroyed;
static bool region2_destroyed;

static void destroy_region1(struct arcp_region *region __attribute__((unused))) {
    region1_destroyed = true;
}

static void destroy_region2(struct arcp_region *region __attribute__((unused))) {
    region2_destroyed = true;
}

static arcp_t arcp;

static struct arcp_region *region1;
static struct arcp_region *region2;

/*************************/

static void test_arcp_uninit_fixture(void (*test)()) {
    region1_destroyed = false;
    region2_destroyed = false;
    region1 = alloca(ARCP_REGION_OVERHEAD + strlen(strtest.string1) + 1);
    region2 = alloca(ARCP_REGION_OVERHEAD + strlen(strtest.string1) + 1);
    strcpy((char *) ARCP_REGION2DATA(region1), strtest.string1);
    strcpy((char *) ARCP_REGION2DATA(region2), strtest.string2);
    test();
}

static void test_arcp_region_init() {
    CHECKPOINT();
    arcp_region_init(region1, destroy_region1);
    ASSERT(region1->destroy == destroy_region1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(arcp_refcount(region1) == 1);
    ASSERT(arcp_ptrcount(region1) == 0);
    ASSERT(!region1_destroyed);
}

/****************************/

static void test_arcp_init_region_fixture(void (*test)()) {
    region1_destroyed = false;
    region2_destroyed = false;
    region1 = alloca(ARCP_REGION_OVERHEAD + strlen(strtest.string1) + 1);
    region2 = alloca(ARCP_REGION_OVERHEAD + strlen(strtest.string1) + 1);
    strcpy((char *) ARCP_REGION2DATA(region1), strtest.string1);
    strcpy((char *) ARCP_REGION2DATA(region2), strtest.string2);
    CHECKPOINT();
    arcp_region_init(region1, destroy_region1);
    CHECKPOINT();
    arcp_region_init(region2, destroy_region2);
    test();
}

static void test_arcp_init() {
    CHECKPOINT();
    arcp_init(&arcp, region1);
    ASSERT(arcp_refcount(region1) == 1);
    ASSERT(arcp_ptrcount(region1) == 1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(!region1_destroyed);
    struct arcp_region *rg = arcp_load_weak(&arcp);
    ASSERT(rg == region1);
}

static void test_arcp_acquire() {
    CHECKPOINT();
    struct arcp_region *rg = arcp_acquire(region1);
    ASSERT(rg == region1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(arcp_refcount(region1) == 2);
    ASSERT(arcp_ptrcount(region1) == 0);
    ASSERT(!region1_destroyed);
}

/****************************/

static void test_arcp_init_fixture(void (*test)()) {
    region1_destroyed = false;
    region2_destroyed = false;
    region1 = alloca(ARCP_REGION_OVERHEAD + strlen(strtest.string1) + 1);
    region2 = alloca(ARCP_REGION_OVERHEAD + strlen(strtest.string1) + 1);
    strcpy((char *) ARCP_REGION2DATA(region1), strtest.string1);
    strcpy((char *) ARCP_REGION2DATA(region2), strtest.string2);
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
    struct arcp_region *rg = arcp_load(&arcp);
    ASSERT(rg == region1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(arcp_refcount(region1) == 2);
    ASSERT(arcp_ptrcount(region1) == 1);
    ASSERT(!region1_destroyed);
}

static void test_arcp_load_weak() {
    CHECKPOINT();
    struct arcp_region *rg = arcp_load_weak(&arcp);
    ASSERT(rg == region1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(arcp_refcount(region1) == 1);
    ASSERT(arcp_ptrcount(region1) == 1);
    ASSERT(!region1_destroyed);
}

static void test_arcp_release() {
    arcp_release(region1);
    ASSERT(arcp_refcount(region1) == 0);
    ASSERT(arcp_ptrcount(region1) == 1);
    ASSERT(!region1_destroyed);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    region1 = arcp_load(&arcp);
    CHECKPOINT();
    arcp_store(&arcp, NULL);
    CHECKPOINT();
    arcp_release(region1);
    ASSERT(arcp_refcount(region1) == 0);
    ASSERT(arcp_ptrcount(region1) == 0);
    ASSERT(region1_destroyed);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
}

static void test_arcp_compare_store() {
    /* succeed */
    ASSERT(arcp_compare_store(&arcp, region1, region2));
    ASSERT(arcp_refcount(region1) == 1);
    ASSERT(arcp_ptrcount(region1) == 0);
    ASSERT(arcp_refcount(region2) == 1);
    ASSERT(arcp_ptrcount(region2) == 1);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), strtest.string2) == 0);
    struct arcp_region *rg = arcp_load_weak(&arcp);
    ASSERT(rg == region2);
    /* fail */
    ASSERT(!arcp_compare_store(&arcp, region1, region1));
    ASSERT(arcp_refcount(region1) == 1);
    ASSERT(arcp_ptrcount(region1) == 0);
    ASSERT(arcp_refcount(region2) == 1);
    ASSERT(arcp_ptrcount(region2) == 1);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), strtest.string2) == 0);
    rg = arcp_load_weak(&arcp);
    ASSERT(rg == region2);
}

static void test_arcp_compare_store_release_succeed() {
    /* succeed */
    ASSERT(arcp_compare_store_release(&arcp, region1, region2));
    ASSERT(arcp_refcount(region1) == 0);
    ASSERT(arcp_ptrcount(region1) == 0);
    ASSERT(arcp_refcount(region2) == 0);
    ASSERT(arcp_ptrcount(region2) == 1);
    ASSERT(region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), strtest.string2) == 0);
    struct arcp_region *rg = arcp_load(&arcp);
    ASSERT(rg == region2);
}

static void test_arcp_compare_store_release_fail() {
    /* fail */
    ASSERT(!arcp_compare_store_release(&arcp, region2, region1));
    ASSERT(arcp_refcount(region1) == 0);
    ASSERT(arcp_ptrcount(region1) == 1);
    ASSERT(arcp_refcount(region2) == 0);
    ASSERT(arcp_ptrcount(region2) == 0);
    ASSERT(!region1_destroyed);
    ASSERT(region2_destroyed);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), strtest.string2) == 0);
    struct arcp_region *rg = arcp_load_weak(&arcp);
    ASSERT(rg == region1);
}

static void test_arcp_store() {
    CHECKPOINT();
    arcp_store(&arcp, region2);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(arcp_refcount(region1) == 1);
    ASSERT(arcp_ptrcount(region1) == 0);
    ASSERT(arcp_refcount(region2) == 1);
    ASSERT(arcp_ptrcount(region2) == 1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), strtest.string2) == 0);
    struct arcp_region *rg = arcp_load_weak(&arcp);
    ASSERT(rg == region2);
    arcp_release(region2);
    CHECKPOINT();
    arcp_store(&arcp, region1);
    ASSERT(!region1_destroyed);
    ASSERT(region2_destroyed);
    ASSERT(arcp_refcount(region1) == 1);
    ASSERT(arcp_ptrcount(region1) == 1);
    ASSERT(arcp_refcount(region2) == 0);
    ASSERT(arcp_ptrcount(region2) == 0);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), strtest.string2) == 0);
    rg = arcp_load_weak(&arcp);
    ASSERT(rg == region1);
}

static void test_arcp_exchange() {
    CHECKPOINT();
    struct arcp_region *rg = arcp_exchange(&arcp, region2);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(arcp_refcount(region1) == 2);
    ASSERT(arcp_ptrcount(region1) == 0);
    ASSERT(arcp_refcount(region2) == 1);
    ASSERT(arcp_ptrcount(region2) == 1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), strtest.string2) == 0);
    ASSERT(rg == region1);
    rg = arcp_load_weak(&arcp);
    ASSERT(rg == region2);
    arcp_release(region2);
    CHECKPOINT();
    rg = arcp_exchange(&arcp, region1);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(arcp_refcount(region1) == 2);
    ASSERT(arcp_ptrcount(region1) == 1);
    ASSERT(arcp_refcount(region2) == 1);
    ASSERT(arcp_ptrcount(region2) == 0);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), strtest.string2) == 0);
    ASSERT(rg == region2);
    rg = arcp_load_weak(&arcp);
    ASSERT(rg == region1);
}

int run_atomic_rcp_h_test_suite() {
    int r;
    void (*arcp_uninit_tests[])() = { test_arcp_region_init, NULL };
    char *arcp_uninit_test_names[] = { "arcp_region_init", NULL };
    r = run_test_suite(test_arcp_uninit_fixture, arcp_uninit_test_names, arcp_uninit_tests);
    if(r != 0) {
	return r;
    }

    void (*arcp_init_region_tests[])() = { test_arcp_init, test_arcp_acquire, NULL };
    char *arcp_init_region_test_names[] = { "arcp_init", "arcp_acquire", NULL };
    r = run_test_suite(test_arcp_init_region_fixture, arcp_init_region_test_names, arcp_init_region_tests);
    if(r != 0) {
	return r;
    }

    void (*arcp_init_tests[])() = { test_arcp_load, test_arcp_load_weak,
				    test_arcp_compare_store, test_arcp_compare_store_release_succeed,
				    test_arcp_compare_store_release_fail,
				    test_arcp_store, test_arcp_release, test_arcp_exchange, NULL };
    char *arcp_init_test_names[] = { "arcp_load", "arcp_load_weak",
				     "arcp_compare_store", "arcp_compare_store_release_succeed",
				     "arcp_compare_store_release_fail",
				     "arcp_store", "arcp_release", "arcp_exchange", NULL };
    r = run_test_suite(test_arcp_init_fixture, arcp_init_test_names, arcp_init_tests);
    if(r != 0) {
	return r;
    }

    return 0;
}
