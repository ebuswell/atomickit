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
#include <alloca.h>
#include <atomickit/atomic-rcp.h>
#include "alltests.h"
#include "test.h"

static struct {
    char string1[14];
    char string2[14];
} strtest = { "Test String 1", "Test String 2" };

static bool region1_destroyed;
static bool region2_destroyed;

static bool weakref1_destroyed;
static bool weakref2_destroyed;

static void destroy_region1(struct arcp_region *region __attribute__((unused))) {
    region1_destroyed = true;
}

static void destroy_region2(struct arcp_region *region __attribute__((unused))) {
    region2_destroyed = true;
}

static void destroy_weakref1(struct arcp_region *region __attribute__((unused))) {
    weakref1_destroyed = true;
}

static void destroy_weakref2(struct arcp_region *region __attribute__((unused))) {
    weakref2_destroyed = true;
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
    struct arcp_region *rg;
    CHECKPOINT();
    arcp_init(&arcp, region1);
    ASSERT(arcp_refcount(region1) == 1);
    ASSERT(arcp_ptrcount(region1) == 1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(!region1_destroyed);
    rg = arcp_load_phantom(&arcp);
    ASSERT(rg == region1);
}

static void test_arcp_acquire() {
    struct arcp_region *rg;
    CHECKPOINT();
    rg = arcp_acquire(region1);
    ASSERT(rg == region1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(arcp_refcount(region1) == 2);
    ASSERT(arcp_ptrcount(region1) == 0);
    ASSERT(!region1_destroyed);
}

static void test_arcp_region_init_weakref() {
    int r;
    struct arcp_weakref *weakref;
    struct arcp_region *rg;
    CHECKPOINT();
    r = arcp_region_init_weakref(region1);
    ASSERT(r == 0);
    weakref = arcp_weakref_phantom(region1);
    ASSERT(weakref != NULL);
    ASSERT(arcp_refcount(weakref) == 0);
    ASSERT(arcp_ptrcount(weakref) == 1);
    rg = arcp_weakref_load(weakref);
    ASSERT(rg == region1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    arcp_region_destroy_weakref(region1);
}

/****************************/
static void test_arcp_init_weakref_fixture(void (*test)()) {
    struct arcp_weakref *weakref1;
    struct arcp_weakref *weakref2;
    region1_destroyed = false;
    region2_destroyed = false;
    weakref1_destroyed = false;
    weakref2_destroyed = false;
    region1 = alloca(ARCP_REGION_OVERHEAD + strlen(strtest.string1) + 1);
    region2 = alloca(ARCP_REGION_OVERHEAD + strlen(strtest.string1) + 1);
    strcpy((char *) ARCP_REGION2DATA(region1), strtest.string1);
    strcpy((char *) ARCP_REGION2DATA(region2), strtest.string2);
    CHECKPOINT();
    arcp_region_init(region1, destroy_region1);
    CHECKPOINT();
    arcp_region_init(region2, destroy_region2);
    CHECKPOINT();
    weakref1 = alloca(sizeof(struct arcp_weakref));
    weakref2 = alloca(sizeof(struct arcp_weakref));
    CHECKPOINT();
    atomic_ptr_init(&weakref1->ptr, region1);
    CHECKPOINT();
    atomic_ptr_init(&weakref2->ptr, region2);
    CHECKPOINT();
    arcp_region_init(weakref1, destroy_weakref1);
    CHECKPOINT();
    arcp_region_init(weakref2, destroy_weakref2);
    CHECKPOINT();
    arcp_store(&region1->weakref, weakref1);
    CHECKPOINT();
    arcp_store(&region2->weakref, weakref2);
    CHECKPOINT();
    arcp_release(weakref1);
    CHECKPOINT();
    arcp_release(weakref2);
    CHECKPOINT();
    test();
}

static void test_arcp_region_destroy_weakref() {
    CHECKPOINT();
    arcp_region_destroy_weakref(region1);
    ASSERT(weakref1_destroyed);
}

static void test_arcp_weakref() {
    struct arcp_weakref *weakref;
    struct arcp_region *rg;
    CHECKPOINT();
    weakref = arcp_weakref(region1);
    ASSERT(weakref != NULL);
    ASSERT(arcp_refcount(weakref) == 1);
    ASSERT(arcp_ptrcount(weakref) == 1);
    ASSERT(!weakref1_destroyed);
    rg = arcp_weakref_load(weakref);
    ASSERT(rg == region1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
}

static void test_arcp_weakref_phantom() {
    struct arcp_weakref *weakref;
    struct arcp_region *rg;
    CHECKPOINT();
    weakref = arcp_weakref_phantom(region1);
    ASSERT(weakref != NULL);
    ASSERT(arcp_refcount(weakref) == 0);
    ASSERT(arcp_ptrcount(weakref) == 1);
    ASSERT(!weakref1_destroyed);
    rg = arcp_weakref_load(weakref);
    ASSERT(rg == region1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
}

static void test_arcp_weakref_load() {
    struct arcp_region *rg;
    CHECKPOINT();
    rg = arcp_weakref_load(arcp_weakref_phantom(region1));
    ASSERT(rg == region1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(arcp_refcount(region1) == 2);
    ASSERT(arcp_ptrcount(region1) == 0);
    ASSERT(!region1_destroyed);
    ASSERT(!weakref1_destroyed);
}

static void test_arcp_release_with_weakref() {
    CHECKPOINT();
    arcp_release(region1);
    ASSERT(region1_destroyed);
    ASSERT(weakref1_destroyed);
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
    struct arcp_region *rg;
    CHECKPOINT();
    rg = arcp_load(&arcp);
    ASSERT(rg == region1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(arcp_refcount(region1) == 2);
    ASSERT(arcp_ptrcount(region1) == 1);
    ASSERT(!region1_destroyed);
}

static void test_arcp_load_phantom() {
    struct arcp_region *rg;
    CHECKPOINT();
    rg = arcp_load_phantom(&arcp);
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
    struct arcp_region *rg;
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
    rg = arcp_load_phantom(&arcp);
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
    rg = arcp_load_phantom(&arcp);
    ASSERT(rg == region2);
}

static void test_arcp_compare_store_release_succeed() {
    struct arcp_region *rg;
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
    rg = arcp_load(&arcp);
    ASSERT(rg == region2);
}

static void test_arcp_compare_store_release_fail() {
    struct arcp_region *rg;
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
    rg = arcp_load_phantom(&arcp);
    ASSERT(rg == region1);
}

static void test_arcp_store() {
    struct arcp_region *rg;
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
    rg = arcp_load_phantom(&arcp);
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
    rg = arcp_load_phantom(&arcp);
    ASSERT(rg == region1);
}

static void test_arcp_exchange() {
    struct arcp_region *rg;
    CHECKPOINT();
    rg = arcp_exchange(&arcp, region2);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(arcp_refcount(region1) == 2);
    ASSERT(arcp_ptrcount(region1) == 0);
    ASSERT(arcp_refcount(region2) == 1);
    ASSERT(arcp_ptrcount(region2) == 1);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region1), strtest.string1) == 0);
    ASSERT(strcmp((char *) ARCP_REGION2DATA(region2), strtest.string2) == 0);
    ASSERT(rg == region1);
    rg = arcp_load_phantom(&arcp);
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
    rg = arcp_load_phantom(&arcp);
    ASSERT(rg == region1);
}

int run_atomic_rcp_h_test_suite() {
    int r;
    void (*arcp_uninit_tests[])() = { test_arcp_region_init, NULL };
    char *arcp_uninit_test_names[] = { "arcp_region_init", NULL };

    void (*arcp_init_region_tests[])() = { test_arcp_init, test_arcp_acquire,
					   test_arcp_region_init_weakref, NULL };
    char *arcp_init_region_test_names[] = { "arcp_init", "arcp_acquire",
					    "arcp_region_init_weakref", NULL };

    void (*arcp_init_weakref_tests[])() = { test_arcp_region_destroy_weakref, test_arcp_weakref,
					    test_arcp_weakref_phantom, test_arcp_weakref_load,
					    test_arcp_release_with_weakref, NULL };
    char *arcp_init_weakref_test_names[] = { "arcp_region_destroy_weakref", "arcp_weakref",
					     "arcp_weakref_phantom", "arcp_weakref_load",
					     "arcp_release_with_weakref", NULL };

    void (*arcp_init_tests[])() = { test_arcp_load, test_arcp_load_phantom,
				    test_arcp_compare_store, test_arcp_compare_store_release_succeed,
				    test_arcp_compare_store_release_fail,
				    test_arcp_store, test_arcp_release, test_arcp_exchange, NULL };

    char *arcp_init_test_names[] = { "arcp_load", "arcp_load_phantom",
				     "arcp_compare_store", "arcp_compare_store_release_succeed",
				     "arcp_compare_store_release_fail",
				     "arcp_store", "arcp_release", "arcp_exchange", NULL };

    r = run_test_suite(test_arcp_uninit_fixture, arcp_uninit_test_names, arcp_uninit_tests);
    if(r != 0) {
	return r;
    }

    r = run_test_suite(test_arcp_init_region_fixture, arcp_init_region_test_names, arcp_init_region_tests);
    if(r != 0) {
	return r;
    }

    r = run_test_suite(test_arcp_init_weakref_fixture, arcp_init_weakref_test_names, arcp_init_weakref_tests);
    if(r != 0) {
	return r;
    }

    r = run_test_suite(test_arcp_init_fixture, arcp_init_test_names, arcp_init_tests);
    if(r != 0) {
	return r;
    }

    return 0;
}
