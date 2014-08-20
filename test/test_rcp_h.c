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
#include <atomickit/rcp.h>
#include <atomickit/malloc.h>
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
    afree(region, sizeof(struct arcp_weakref));
    weakref1_destroyed = true;
}

static void destroy_weakref2(struct arcp_region *region __attribute__((unused))) {
    afree(region, sizeof(struct arcp_weakref));
    weakref2_destroyed = true;
}

static arcp_t arcp;

struct arcp_test_region {
	struct arcp_region;
	char data[];
};

static struct arcp_test_region *region1;
static struct arcp_test_region *region2;

/*************************/

static void test_arcp_uninit_fixture(void (*test)()) {
    region1_destroyed = false;
    region2_destroyed = false;
    region1 = alloca(sizeof(struct arcp_test_region) + strlen(strtest.string1) + 1);
    region2 = alloca(sizeof(struct arcp_test_region) + strlen(strtest.string1) + 1);
    strcpy(region1->data, strtest.string1);
    strcpy(region2->data, strtest.string2);
    test();
}

static void test_arcp_region_init() {
    CHECKPOINT();
    arcp_region_init(region1, destroy_region1);
    ASSERT(region1->destroy == destroy_region1);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(arcp_usecount(region1) == 1);
    ASSERT(arcp_storecount(region1) == 0);
    ASSERT(!region1_destroyed);
}

/****************************/

static void test_arcp_init_region_fixture(void (*test)()) {
    region1_destroyed = false;
    region2_destroyed = false;
    region1 = alloca(sizeof(struct arcp_test_region) + strlen(strtest.string1) + 1);
    region2 = alloca(sizeof(struct arcp_test_region) + strlen(strtest.string1) + 1);
    strcpy(region1->data, strtest.string1);
    strcpy(region2->data, strtest.string2);
    CHECKPOINT();
    arcp_region_init(region1, destroy_region1);
    CHECKPOINT();
    arcp_region_init(region2, destroy_region2);
    test();
}

static void test_arcp_init() {
    struct arcp_test_region *rg;
    CHECKPOINT();
    arcp_init(&arcp, region1);
    ASSERT(arcp_usecount(region1) == 1);
    ASSERT(arcp_storecount(region1) == 1);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(!region1_destroyed);
    rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
    ASSERT(rg == region1);
}

static void test_arcp_acquire() {
    struct arcp_test_region *rg;
    CHECKPOINT();
    rg = arcp_acquire(region1);
    ASSERT(rg == region1);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(arcp_usecount(region1) == 2);
    ASSERT(arcp_storecount(region1) == 0);
    ASSERT(!region1_destroyed);
}

static void test_arcp_region_init_weakref() {
    int r;
    struct arcp_weakref *weakref;
    struct arcp_test_region *rg;
    CHECKPOINT();
    r = arcp_region_init_weakref(region1);
    ASSERT(r == 0);
    weakref = arcp_weakref_phantom(region1);
    ASSERT(weakref != NULL);
    ASSERT(arcp_usecount(weakref) == 0);
    ASSERT(arcp_storecount(weakref) == 1);
    rg = (struct arcp_test_region *) arcp_weakref_load(weakref);
    ASSERT(rg == region1);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
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
    region1 = alloca(sizeof(struct arcp_test_region) + strlen(strtest.string1) + 1);
    region2 = alloca(sizeof(struct arcp_test_region) + strlen(strtest.string1) + 1);
    strcpy(region1->data, strtest.string1);
    strcpy(region2->data, strtest.string2);
    CHECKPOINT();
    arcp_region_init(region1, destroy_region1);
    CHECKPOINT();
    arcp_region_init(region2, destroy_region2);
    CHECKPOINT();
    arcp_region_init_weakref(region1);
    CHECKPOINT();
    arcp_region_init_weakref(region2);
    CHECKPOINT();
    weakref1 = (struct arcp_weakref *) arcp_load_phantom(&region1->weakref);
    weakref2 = (struct arcp_weakref *) arcp_load_phantom(&region2->weakref);
    weakref1->destroy = (arcp_destroy_f) destroy_weakref1;
    weakref2->destroy = (arcp_destroy_f) destroy_weakref2;
    CHECKPOINT();
    test();
    return;

    weakref1 = alloca(sizeof(struct arcp_weakref));
    weakref2 = alloca(sizeof(struct arcp_weakref));
    CHECKPOINT();
    ak_init(&weakref1->target, region1);
    CHECKPOINT();
    ak_init(&weakref2->target, region2);
    CHECKPOINT();
    ak_init(&weakref1->refcount, __ARCP_REFCOUNT_INIT(1, 0));
    CHECKPOINT();
    ak_init(&weakref2->refcount, __ARCP_REFCOUNT_INIT(1, 0));
    CHECKPOINT();
    weakref1->destroy = (arcp_destroy_f) destroy_weakref1;
    CHECKPOINT();
    weakref2->destroy = (arcp_destroy_f) destroy_weakref2;
    CHECKPOINT();
    ak_init(&weakref1->weakref, NULL);
    CHECKPOINT();
    ak_init(&weakref2->weakref, NULL);
    CHECKPOINT();
    ak_init(&region1->weakref, weakref1);
    CHECKPOINT();
    ak_init(&region2->weakref, weakref2);
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
    struct arcp_test_region *rg;
    CHECKPOINT();
    weakref = arcp_weakref(region1);
    ASSERT(weakref != NULL);
    ASSERT(arcp_usecount(weakref) == 1);
    ASSERT(arcp_storecount(weakref) == 1);
    ASSERT(!weakref1_destroyed);
    rg = (struct arcp_test_region *) arcp_weakref_load(weakref);
    ASSERT(rg == region1);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
}

static void test_arcp_weakref_phantom() {
    struct arcp_weakref *weakref;
    struct arcp_test_region *rg;
    CHECKPOINT();
    weakref = arcp_weakref_phantom(region1);
    ASSERT(weakref != NULL);
    ASSERT(arcp_usecount(weakref) == 0);
    ASSERT(arcp_storecount(weakref) == 1);
    ASSERT(!weakref1_destroyed);
    rg = (struct arcp_test_region *) arcp_weakref_load(weakref);
    ASSERT(rg == region1);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
}

static void test_arcp_weakref_load() {
    struct arcp_test_region *rg;
    CHECKPOINT();
    rg = (struct arcp_test_region *) arcp_weakref_load(arcp_weakref_phantom(region1));
    ASSERT(rg == region1);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(arcp_usecount(region1) == 2);
    ASSERT(arcp_storecount(region1) == 0);
    ASSERT(!region1_destroyed);
    ASSERT(!weakref1_destroyed);
}

static void test_arcp_release_with_weakref() {
    arcp_refcount_t rc;
    CHECKPOINT();
    rc.p = atomic_load(&region1->refcount);
    ASSERT(rc.v.usecount == 1);
    ASSERT(rc.v.destroy_lock == 0);
    ASSERT(rc.v.storecount == 0);
    arcp_release(region1);
    ASSERT(region1_destroyed);
    ASSERT(weakref1_destroyed);
}

/****************************/

static void test_arcp_init_fixture(void (*test)()) {
    region1_destroyed = false;
    region2_destroyed = false;
    region1 = alloca(sizeof(struct arcp_test_region) + strlen(strtest.string1) + 1);
    region2 = alloca(sizeof(struct arcp_test_region) + strlen(strtest.string1) + 1);
    strcpy(region1->data, strtest.string1);
    strcpy(region2->data, strtest.string2);
    CHECKPOINT();
    arcp_region_init(region1, destroy_region1);
    CHECKPOINT();
    arcp_region_init(region2, destroy_region2);
    CHECKPOINT();
    arcp_init(&arcp, region1);
    test();
}

static void test_arcp_load() {
    struct arcp_test_region *rg;
    CHECKPOINT();
    rg = (struct arcp_test_region *) arcp_load(&arcp);
    ASSERT(rg == region1);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(arcp_usecount(region1) == 2);
    ASSERT(arcp_storecount(region1) == 1);
    ASSERT(!region1_destroyed);
}

static void test_arcp_load_phantom() {
    struct arcp_test_region *rg;
    CHECKPOINT();
    rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
    ASSERT(rg == region1);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(arcp_usecount(region1) == 1);
    ASSERT(arcp_storecount(region1) == 1);
    ASSERT(!region1_destroyed);
}

static void test_arcp_release() {
    arcp_release(region1);
    ASSERT(arcp_usecount(region1) == 0);
    ASSERT(arcp_storecount(region1) == 1);
    ASSERT(!region1_destroyed);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    region1 = (struct arcp_test_region *) arcp_load(&arcp);
    CHECKPOINT();
    arcp_store(&arcp, NULL);
    CHECKPOINT();
    arcp_release(region1);
    ASSERT(arcp_usecount(region1) == 0);
    ASSERT(arcp_storecount(region1) == 0);
    ASSERT(region1_destroyed);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
}

static void test_arcp_cas() {
    struct arcp_test_region *rg;
    /* succeed */
    ASSERT(arcp_cas(&arcp, region1, region2));
    ASSERT(arcp_usecount(region1) == 1);
    ASSERT(arcp_storecount(region1) == 0);
    ASSERT(arcp_usecount(region2) == 1);
    ASSERT(arcp_storecount(region2) == 1);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(strcmp(region2->data, strtest.string2) == 0);
    rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
    ASSERT(rg == region2);
    /* fail */
    ASSERT(!arcp_cas(&arcp, region1, region1));
    ASSERT(arcp_usecount(region1) == 1);
    ASSERT(arcp_storecount(region1) == 0);
    ASSERT(arcp_usecount(region2) == 1);
    ASSERT(arcp_storecount(region2) == 1);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(strcmp(region2->data, strtest.string2) == 0);
    rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
    ASSERT(rg == region2);
}

static void test_arcp_cas_release_succeed() {
    struct arcp_test_region *rg;
    /* succeed */
    ASSERT(arcp_cas_release(&arcp, region1, region2));
    ASSERT(arcp_usecount(region1) == 0);
    ASSERT(arcp_storecount(region1) == 0);
    ASSERT(arcp_usecount(region2) == 0);
    ASSERT(arcp_storecount(region2) == 1);
    ASSERT(region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(strcmp(region2->data, strtest.string2) == 0);
    rg = (struct arcp_test_region *) arcp_load(&arcp);
    ASSERT(rg == region2);
}

static void test_arcp_cas_release_fail() {
    struct arcp_test_region *rg;
    /* fail */
    ASSERT(!arcp_cas_release(&arcp, region2, region1));
    ASSERT(arcp_usecount(region1) == 0);
    ASSERT(arcp_storecount(region1) == 1);
    ASSERT(arcp_usecount(region2) == 0);
    ASSERT(arcp_storecount(region2) == 0);
    ASSERT(!region1_destroyed);
    ASSERT(region2_destroyed);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(strcmp(region2->data, strtest.string2) == 0);
    rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
    ASSERT(rg == region1);
}

static void test_arcp_store() {
    struct arcp_test_region *rg;
    CHECKPOINT();
    arcp_store(&arcp, region2);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(arcp_usecount(region1) == 1);
    ASSERT(arcp_storecount(region1) == 0);
    ASSERT(arcp_usecount(region2) == 1);
    ASSERT(arcp_storecount(region2) == 1);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(strcmp(region2->data, strtest.string2) == 0);
    rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
    ASSERT(rg == region2);
    arcp_release(region2);
    CHECKPOINT();
    arcp_store(&arcp, region1);
    ASSERT(!region1_destroyed);
    ASSERT(region2_destroyed);
    ASSERT(arcp_usecount(region1) == 1);
    ASSERT(arcp_storecount(region1) == 1);
    ASSERT(arcp_usecount(region2) == 0);
    ASSERT(arcp_storecount(region2) == 0);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(strcmp(region2->data, strtest.string2) == 0);
    rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
    ASSERT(rg == region1);
}

static void test_arcp_swap() {
    struct arcp_test_region *rg;
    CHECKPOINT();
    rg = (struct arcp_test_region *) arcp_swap(&arcp, region2);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(arcp_usecount(region1) == 2);
    ASSERT(arcp_storecount(region1) == 0);
    ASSERT(arcp_usecount(region2) == 1);
    ASSERT(arcp_storecount(region2) == 1);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(strcmp(region2->data, strtest.string2) == 0);
    ASSERT(rg == region1);
    rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
    ASSERT(rg == region2);
    arcp_release(region2);
    CHECKPOINT();
    rg = (struct arcp_test_region *) arcp_swap(&arcp, region1);
    ASSERT(!region1_destroyed);
    ASSERT(!region2_destroyed);
    ASSERT(arcp_usecount(region1) == 2);
    ASSERT(arcp_storecount(region1) == 1);
    ASSERT(arcp_usecount(region2) == 1);
    ASSERT(arcp_storecount(region2) == 0);
    ASSERT(strcmp(region1->data, strtest.string1) == 0);
    ASSERT(strcmp(region2->data, strtest.string2) == 0);
    ASSERT(rg == region2);
    rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
    ASSERT(rg == region1);
}

int run_rcp_h_test_suite() {
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
				    test_arcp_cas, test_arcp_cas_release_succeed,
				    test_arcp_cas_release_fail,
				    test_arcp_store, test_arcp_release, test_arcp_swap, NULL };

    char *arcp_init_test_names[] = { "arcp_load", "arcp_load_phantom",
				     "arcp_cas", "arcp_cas_release_succeed",
				     "arcp_cas_release_fail",
				     "arcp_store", "arcp_release", "arcp_swap", NULL };

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
