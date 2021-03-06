/*
 * test_array_h.c
 *
 * Copyright 2014 Evan Buswell
 *
 * This file is part of Atomic Kit.
 * 
 * Atomic Kit is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 2.
 * 
 * Atomic Kit is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <string.h>
#include <alloca.h>
#include <atomickit/array.h>
#include "alltests.h"
#include "test.h"

#define TEST_STRLEN 14

static struct {
	char string1[TEST_STRLEN];
	char string2[TEST_STRLEN];
	char string3[TEST_STRLEN];
} ptrtest = { "Test String 1", "Test String 2", "Test String 3" };

static bool region1_destroyed;
static bool region2_destroyed;
static bool region3_destroyed;

static void destroy_region1(struct arcp_region *region
			    __attribute__((unused))) {
	CHECKPOINT();
	region1_destroyed = true;
}

static void destroy_region2(struct arcp_region *region
			    __attribute__((unused))) {
	CHECKPOINT();
	region2_destroyed = true;
}

static void destroy_region3(struct arcp_region *region
			    __attribute__((unused))) {
	CHECKPOINT();
	region3_destroyed = true;
}

struct arcp_test_region {
	struct arcp_region;
	char data[];
};

static struct arcp_test_region *region1;
static struct arcp_test_region *region2;
static struct arcp_test_region *region3;

static struct aary *array;

/*************************/
static void test_aary_create() {
	struct aary *ary;
	CHECKPOINT();
	ary = aary_create(0);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 0);
	arcp_release(ary);
	CHECKPOINT();
	ary = aary_create(1024);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 1024);
	CHECKPOINT();
	ASSERT(aary_load_phantom(ary, 1023) == NULL);
	arcp_release(ary);
}

/*************************/
static void test_aary_init_fixture(void (*test)()) {
	region1_destroyed = false;
	region2_destroyed = false;
	region3_destroyed = false;
	region1 = alloca(sizeof(struct arcp_test_region) + TEST_STRLEN);
	region2 = alloca(sizeof(struct arcp_test_region) + TEST_STRLEN);
	region3 = alloca(sizeof(struct arcp_test_region) + TEST_STRLEN);
	strcpy(region1->data, ptrtest.string1);
	strcpy(region2->data, ptrtest.string2);
	strcpy(region3->data, ptrtest.string3);
	arcp_region_init(region1, destroy_region1);
	arcp_region_init(region2, destroy_region2);
	arcp_region_init(region3, destroy_region3);
	CHECKPOINT();
	array = aary_create(2);
	if (array == NULL) {
		UNRESOLVED("Could not create array");
	}
	test();
}

static void test_aary_len() {
	CHECKPOINT();
	ASSERT(aary_len(array) == 2);
}

static void test_aary_store() {
	CHECKPOINT();
	aary_store(array, 0, region1);
	CHECKPOINT();
	ASSERT((struct arcp_test_region *) aary_load_phantom(array, 0) == region1);
	ASSERT(!region1_destroyed);
	CHECKPOINT();
	aary_store(array, 1, region2);
	CHECKPOINT();
	ASSERT((struct arcp_test_region *) aary_load_phantom(array, 1) == region2);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	arcp_release(region1);
	arcp_release(region2);
	CHECKPOINT();
	aary_store(array, 1, region3);
	CHECKPOINT();
	ASSERT((struct arcp_test_region *) aary_load_phantom(array, 1) == region3);
	ASSERT(!region1_destroyed);
	ASSERT(region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
}

static void test_aary_storefirst() {
	CHECKPOINT();
	aary_storefirst(array, region1);
	CHECKPOINT();
	ASSERT((struct arcp_test_region *) aary_load_phantom(array, 0) == region1);
	ASSERT(!region1_destroyed);
	arcp_release(region1);
	CHECKPOINT();
	aary_storefirst(array, region2);
	CHECKPOINT();
	ASSERT((struct arcp_test_region *) aary_load_phantom(array, 0) == region2);
	ASSERT(region1_destroyed);
	ASSERT(!region2_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
}

static void test_aary_storelast() {
	CHECKPOINT();
	aary_storelast(array, region1);
	CHECKPOINT();
	ASSERT((struct arcp_test_region *) aary_load_phantom(array, 1) == region1);
	ASSERT(!region1_destroyed);
	arcp_release(region1);
	CHECKPOINT();
	aary_storelast(array, region2);
	CHECKPOINT();
	ASSERT((struct arcp_test_region *) aary_load_phantom(array, 1) == region2);
	ASSERT(region1_destroyed);
	ASSERT(!region2_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
}

/*************************/
static void test_aary_populate_part_fixture(void (*test)()) {
	region1_destroyed = false;
	region2_destroyed = false;
	region3_destroyed = false;
	region1 = alloca(sizeof(struct arcp_test_region) + TEST_STRLEN);
	region2 = alloca(sizeof(struct arcp_test_region) + TEST_STRLEN);
	region3 = alloca(sizeof(struct arcp_test_region) + TEST_STRLEN);
	strcpy(region1->data, ptrtest.string1);
	strcpy(region2->data, ptrtest.string2);
	strcpy(region3->data, ptrtest.string3);
	arcp_region_init(region1, destroy_region1);
	arcp_region_init(region2, destroy_region2);
	arcp_region_init(region3, destroy_region3);
	CHECKPOINT();
	array = aary_create(2);
	if (array == NULL) {
		UNRESOLVED("Could not create array");
	}
	CHECKPOINT();
	aary_store(array, 0, region1);
	CHECKPOINT();
	aary_store(array, 1, region2);
	test();
}

static void test_aary_load() {
	struct arcp_test_region *rg;
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load(array, 0);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(array);
	ASSERT(!region1_destroyed);
	arcp_release(rg);
	ASSERT(region1_destroyed);
}

static void test_aary_last() {
	struct arcp_test_region *rg;
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_last(array);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(array);
	ASSERT(!region2_destroyed);
	arcp_release(rg);
	ASSERT(region2_destroyed);
}

static void test_aary_first() {
	struct arcp_test_region *rg;
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_first(array);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(array);
	ASSERT(!region1_destroyed);
	arcp_release(rg);
	ASSERT(region1_destroyed);
}

static void test_aary_load_phantom() {
	struct arcp_test_region *rg;
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(array, 0);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(array);
	ASSERT(region1_destroyed);
}

static void test_aary_first_phantom() {
	struct arcp_test_region *rg;
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_first_phantom(array);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(array);
	ASSERT(region1_destroyed);
}

static void test_aary_last_phantom() {
	struct arcp_test_region *rg;
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_last_phantom(array);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(array);
	ASSERT(region2_destroyed);
}

static void test_aary_dup() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_dup(array);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	ASSERT(!region1_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	ASSERT(!region2_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(array);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
}

static void test_aary_insert() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_insert(array, 1, region3);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 3);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	ASSERT(!region1_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region3);
	ASSERT(strcmp(rg->data, ptrtest.string3) == 0);
	ASSERT(!region3_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 2);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	ASSERT(!region2_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
	ASSERT(region3_destroyed);
}

static void test_aary_append() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_append(array, region3);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 3);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	ASSERT(!region1_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	ASSERT(!region2_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 2);
	ASSERT(rg == region3);
	ASSERT(strcmp(rg->data, ptrtest.string3) == 0);
	ASSERT(!region3_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
	ASSERT(region3_destroyed);
}

static void test_aary_prepend() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_prepend(array, region3);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 3);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region3);
	ASSERT(strcmp(rg->data, ptrtest.string3) == 0);
	ASSERT(!region3_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	ASSERT(!region1_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 2);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	ASSERT(!region2_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
	ASSERT(region3_destroyed);
}

static void test_aary_dup_insert() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_dup_insert(array, 1, region3);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 3);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	ASSERT(!region1_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region3);
	ASSERT(strcmp(rg->data, ptrtest.string3) == 0);
	ASSERT(!region3_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 2);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	ASSERT(!region2_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(region3_destroyed);
	arcp_release(array);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
}

static void test_aary_dup_append() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_dup_append(array, region3);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 3);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	ASSERT(!region1_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	ASSERT(!region2_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 2);
	ASSERT(rg == region3);
	ASSERT(strcmp(rg->data, ptrtest.string3) == 0);
	ASSERT(!region3_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(region3_destroyed);
	arcp_release(array);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
}

static void test_aary_dup_prepend() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_dup_prepend(array, region3);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 3);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region3);
	ASSERT(strcmp(rg->data, ptrtest.string3) == 0);
	ASSERT(!region3_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	ASSERT(!region1_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 2);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	ASSERT(!region2_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(region3_destroyed);
	arcp_release(array);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
}

static void test_aary_set_add1() {
	struct aary *ary;
	CHECKPOINT();
	aary_sortx(array);
	CHECKPOINT();
	ary = aary_set_add(array, region2);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
}

static void test_aary_set_add2() {
	struct aary *ary;
	CHECKPOINT();
	aary_sortx(array);
	CHECKPOINT();
	ary = aary_set_add(array, region3);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 3);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
	ASSERT(region3_destroyed);
}

static void test_aary_dup_set_add1() {
	struct aary *ary;
	CHECKPOINT();
	aary_sortx(array);
	CHECKPOINT();
	ary = aary_dup_set_add(array, region2);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(ary);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	arcp_release(array);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
}

static void test_aary_dup_set_add2() {
	struct aary *ary;
	CHECKPOINT();
	aary_sortx(array);
	CHECKPOINT();
	ary = aary_dup_set_add(array, region3);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 3);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(region3_destroyed);
	arcp_release(array);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
}

static void test_aary_set_remove1() {
	struct aary *ary;
	CHECKPOINT();
	aary_sortx(array);
	CHECKPOINT();
	ary = aary_set_remove(array, region3);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
}

static void test_aary_dup_set_remove1() {
	struct aary *ary;
	CHECKPOINT();
	aary_sortx(array);
	CHECKPOINT();
	ary = aary_dup_set_remove(array, region3);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(array);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
}

static void test_aary_set_contains() {
	CHECKPOINT();
	aary_sortx(array);
	CHECKPOINT();
	ASSERT(aary_len(array) == 2);
	CHECKPOINT();
	ASSERT(aary_set_contains(array, region1));
	CHECKPOINT();
	ASSERT(aary_set_contains(array, region2));
	CHECKPOINT();
	ASSERT(!aary_set_contains(array, region3));
}

/*************************/
static void test_aary_populate_full_fixture(void (*test)()) {
	region1_destroyed = false;
	region2_destroyed = false;
	region3_destroyed = false;
	region1 = alloca(sizeof(struct arcp_test_region) + TEST_STRLEN);
	region2 = alloca(sizeof(struct arcp_test_region) + TEST_STRLEN);
	region3 = alloca(sizeof(struct arcp_test_region) + TEST_STRLEN);
	strcpy(region1->data, ptrtest.string1);
	strcpy(region2->data, ptrtest.string2);
	strcpy(region3->data, ptrtest.string3);
	arcp_region_init(region1, destroy_region1);
	arcp_region_init(region2, destroy_region2);
	arcp_region_init(region3, destroy_region3);
	CHECKPOINT();
	array = aary_create(3);
	if (array == NULL) {
		UNRESOLVED("Could not create array");
	}
	CHECKPOINT();
	aary_store(array, 0, region1);
	CHECKPOINT();
	aary_store(array, 1, region2);
	CHECKPOINT();
	aary_store(array, 2, region3);
	test();
}

static void test_aary_remove() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_remove(array, 1);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	ASSERT(!region1_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region3);
	ASSERT(strcmp(rg->data, ptrtest.string3) == 0);
	ASSERT(!region3_destroyed);
	ASSERT(!region2_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region3_destroyed);
}

static void test_aary_pop() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_pop(array);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	ASSERT(!region1_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	ASSERT(!region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(region3_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
}

static void test_aary_shift() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_shift(array);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	ASSERT(!region2_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region3);
	ASSERT(strcmp(rg->data, ptrtest.string3) == 0);
	ASSERT(!region3_destroyed);
	ASSERT(!region1_destroyed);
	arcp_release(region1);
	ASSERT(region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(region2_destroyed);
	ASSERT(region3_destroyed);
}

static void test_aary_dup_remove() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_dup_remove(array, 1);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	ASSERT(!region1_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region3);
	ASSERT(strcmp(rg->data, ptrtest.string3) == 0);
	ASSERT(!region3_destroyed);
	ASSERT(!region2_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(array);
	ASSERT(!region1_destroyed);
	ASSERT(region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region3_destroyed);
}

static void test_aary_dup_pop() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_dup_pop(array);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	ASSERT(!region1_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	ASSERT(!region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(array);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(region3_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
}

static void test_aary_dup_shift() {
	struct aary *ary;
	struct arcp_test_region *rg;
	CHECKPOINT();
	ary = aary_dup_shift(array);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 0);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	ASSERT(!region2_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(ary, 1);
	ASSERT(rg == region3);
	ASSERT(strcmp(rg->data, ptrtest.string3) == 0);
	ASSERT(!region3_destroyed);
	ASSERT(!region1_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(array);
	ASSERT(region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(region2_destroyed);
	ASSERT(region3_destroyed);
}

static void test_aary_sortx() {
	CHECKPOINT();
	aary_sortx(array);
	CHECKPOINT();
	ASSERT(aary_len(array) == 3);
	CHECKPOINT();
	ASSERT((uintptr_t) aary_load_phantom(array, 0) < (uintptr_t) aary_load_phantom(array, 1));
	CHECKPOINT();
	ASSERT((uintptr_t) aary_load_phantom(array, 1) < (uintptr_t) aary_load_phantom(array, 2));
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(array);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
	ASSERT(region3_destroyed);
}

static int aary_strcompar(const struct arcp_test_region *a,
			  const struct arcp_test_region *b) {
	return strcmp(a->data, b->data);
}

static void test_aary_sort() {
	struct arcp_test_region *rg1;
	struct arcp_test_region *rg2;
	struct arcp_test_region *rg3;
	CHECKPOINT();
	aary_sort(array,
		  (int (*)(const struct arcp_region *,
			   const struct arcp_region *)) aary_strcompar);
	CHECKPOINT();
	ASSERT(aary_len(array) == 3);
	CHECKPOINT();
	rg1 = (struct arcp_test_region *) aary_load_phantom(array, 0);
	CHECKPOINT();
	rg2 = (struct arcp_test_region *) aary_load_phantom(array, 1);
	CHECKPOINT();
	rg3 = (struct arcp_test_region *) aary_load_phantom(array, 2);
	ASSERT(strcmp(rg1->data, rg2->data) <= 0);
	ASSERT(strcmp(rg2->data, rg3->data) <= 0);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(array);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
	ASSERT(region3_destroyed);
}

static int aary_strcompar_r(const struct arcp_test_region *a,
			    const struct arcp_test_region *b,
			    int *direction) {
	return *direction * strcmp(a->data, b->data);
}

static void test_aary_sort_r() {
	int direction = 1;
	struct arcp_test_region *rg1;
	struct arcp_test_region *rg2;
	struct arcp_test_region *rg3;
	CHECKPOINT();
	aary_sort_r(array,
		    (int (*)(const struct arcp_region *,
			     const struct arcp_region *,
			     void *)) aary_strcompar_r,
		    &direction);
	CHECKPOINT();
	ASSERT(aary_len(array) == 3);
	CHECKPOINT();
	rg1 = (struct arcp_test_region *) aary_load_phantom(array, 0);
	CHECKPOINT();
	rg2 = (struct arcp_test_region *) aary_load_phantom(array, 1);
	CHECKPOINT();
	rg3 = (struct arcp_test_region *) aary_load_phantom(array, 2);
	ASSERT(strcmp(rg1->data, rg2->data) <= 0);
	ASSERT(strcmp(rg2->data, rg3->data) <= 0);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(!region3_destroyed);
	direction = -1;
	CHECKPOINT();
	aary_sort_r(array,
		    (int (*)(const struct arcp_region *,
			     const struct arcp_region *,
			     void *)) aary_strcompar_r,
		    &direction);
	CHECKPOINT();
	ASSERT(aary_len(array) == 3);
	CHECKPOINT();
	rg1 = (struct arcp_test_region *) aary_load_phantom(array, 0);
	CHECKPOINT();
	rg2 = (struct arcp_test_region *) aary_load_phantom(array, 1);
	CHECKPOINT();
	rg3 = (struct arcp_test_region *) aary_load_phantom(array, 2);
	ASSERT(strcmp(rg1->data, rg2->data) >= 0);
	ASSERT(strcmp(rg2->data, rg3->data) >= 0);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(array);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
	ASSERT(region3_destroyed);
}

static void test_aary_reverse() {
	struct arcp_test_region *rg;
	CHECKPOINT();
	aary_reverse(array);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(array, 0);
	ASSERT(rg == region3);
	ASSERT(strcmp(rg->data, ptrtest.string3) == 0);
	ASSERT(!region3_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(array, 1);
	ASSERT(rg == region2);
	ASSERT(strcmp(rg->data, ptrtest.string2) == 0);
	ASSERT(!region2_destroyed);
	CHECKPOINT();
	rg = (struct arcp_test_region *) aary_load_phantom(array, 2);
	ASSERT(rg == region1);
	ASSERT(strcmp(rg->data, ptrtest.string1) == 0);
	ASSERT(!region1_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(array);
	ASSERT(region1_destroyed);
	ASSERT(region2_destroyed);
	ASSERT(region3_destroyed);
}

static void test_aary_set_remove2() {
	struct aary *ary;
	CHECKPOINT();
	aary_sortx(array);
	CHECKPOINT();
	ary = aary_set_remove(array, region2);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region3_destroyed);
}

static void test_aary_dup_set_remove2() {
	struct aary *ary;
	CHECKPOINT();
	aary_sortx(array);
	CHECKPOINT();
	ary = aary_dup_set_remove(array, region2);
	ASSERT(ary != NULL);
	CHECKPOINT();
	ASSERT(aary_len(ary) == 2);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(region1);
	ASSERT(!region1_destroyed);
	arcp_release(region2);
	ASSERT(!region2_destroyed);
	arcp_release(region3);
	ASSERT(!region3_destroyed);
	arcp_release(array);
	ASSERT(!region1_destroyed);
	ASSERT(region2_destroyed);
	ASSERT(!region3_destroyed);
	arcp_release(ary);
	ASSERT(region1_destroyed);
	ASSERT(region3_destroyed);
}

/*************************/
int run_array_h_test_suite() {
	int r;
	void (*void_tests[])() = { test_aary_create, NULL };
	char *void_test_names[] = { "aary_create", NULL };

	void (*init_tests[])() = { test_aary_len, test_aary_store,
				   test_aary_storefirst, test_aary_storelast,
				   NULL };
	char *init_test_names[] = { "aary_len", "aary_store",
				    "aary_storefirst", "aary_storelast",
				    NULL };

	void (*populate_part_tests[])() = { test_aary_load, test_aary_last,
					    test_aary_first,
					    test_aary_load_phantom,
					    test_aary_first_phantom,
					    test_aary_last_phantom,
					    test_aary_dup, test_aary_insert,
					    test_aary_append,
					    test_aary_prepend,
					    test_aary_dup_insert,
					    test_aary_dup_append,
					    test_aary_dup_prepend,
					    test_aary_set_add1,
					    test_aary_set_add2,
					    test_aary_dup_set_add1,
					    test_aary_dup_set_add2,
					    test_aary_set_remove1,
					    test_aary_dup_set_remove1,
					    test_aary_set_contains, NULL };
	char *populate_part_test_names[] = { "aary_load", "aary_last",
					     "aary_first",
					     "aary_load_phantom",
					     "aary_first_phantom",
					     "aary_last_phantom", "aary_dup",
					     "aary_insert", "aary_append",
					     "array_prepend",
					     "aary_dup_insert",
					     "aary_dup_append",
					     "aary_dup_prepend",
					     "aary_set_add1", "aary_set_add2",
					     "aary_dup_set_add1",
					     "aary_dup_set_add2",
					     "aary_set_remove1",
					     "aary_dup_set_remove1",
					     "aary_set_contains", NULL };

	void (*populate_full_tests[])() = { test_aary_remove, test_aary_pop,
					    test_aary_shift,
					    test_aary_dup_remove,
					    test_aary_dup_pop,
					    test_aary_dup_shift,
					    test_aary_sortx, test_aary_sort,
					    test_aary_sort_r,
					    test_aary_reverse,
					    test_aary_set_remove2,
					    test_aary_dup_set_remove2, NULL };
	char *populate_full_test_names[] = { "aary_remove", "aary_pop",
					     "aary_shift", "aary_dup_remove",
					     "aary_dup_pop", "aary_dup_shift",
					     "aary_sortx", "aary_sort",
					     "aary_sort_r", "aary_reverse",
					     "aary_set_remove2",
					     "aary_dup_set_remove2", NULL };

	r = run_test_suite(NULL, void_test_names, void_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_aary_init_fixture, init_test_names,
			   init_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_aary_populate_part_fixture,
			   populate_part_test_names, populate_part_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_aary_populate_full_fixture,
			   populate_full_test_names, populate_full_tests);
	if (r != 0) {
		return r;
	}

	return 0;
}
