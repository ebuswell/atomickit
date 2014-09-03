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
#include <atomickit/malloc.h>
#include "alltests.h"
#include "test.h"

#define NSIZES 10
#define OS_THRESH 8192

static void *regions[NSIZES + 1];

#define REGION_SIZE(i) (16 * (2 << (i)) - 8)

/*************************/
static void test_amalloc() {
    int i;
    CHECKPOINT();
    for(i = 0; i < NSIZES; i++) {
	regions[i] = amalloc(REGION_SIZE(i));
	ASSERT(regions[i] != NULL);
    }
    for(i = 0; i < NSIZES; i++) {
	int j;
	for(j = 0; j < NSIZES; j++) {
	    if(j != i) {
		ASSERT(regions[i] != regions[j]);
	    }
	}
    }
    CHECKPOINT();
    for(i = 0; i < NSIZES; i++) {
	afree(regions[i], REGION_SIZE(i));
    }
    CHECKPOINT();
    for(i = 0; i < NSIZES; i++) {
	regions[i] = amalloc(REGION_SIZE(i));
	ASSERT(regions[i] != NULL);
    }
    for(i = 0; i < NSIZES; i++) {
	int j;
	for(j = 0; j < NSIZES; j++) {
	    if(j != i) {
		ASSERT(regions[i] != regions[j]);
	    }
	}
    }
}

/*************************/
static void test_mallocd_fixture(void (*test)()) {
    int i;
    for(i = 0; i < NSIZES; i++) {
	regions[i] = amalloc(REGION_SIZE(i));
    }
    test();
}

static void test_afree() {
    int i;
    CHECKPOINT();
    for(i = 0; i < NSIZES; i++) {
	afree(regions[i], REGION_SIZE(i));
    }
}

static void test_arealloc() {
    int i;
    CHECKPOINT();
    /* realloc within the same chunk */
    for(i = 0; i < NSIZES; i++) {
	void *old, *new;
	old = regions[i];
	new = arealloc(old, REGION_SIZE(i), REGION_SIZE(i) + 4);
	ASSERT(new != NULL);
	regions[i] = new;
	/* ASSERT(new == old); */
    }
    CHECKPOINT();
    /* realloc to a different chunk */
    for(i = 0; i < NSIZES; i++) {
	void *old, *new;
	old = regions[i];
	new = arealloc(old, REGION_SIZE(i) + 4, REGION_SIZE(i + 1) + 4);
	ASSERT(new != NULL);
	regions[i] = new;
	/* ASSERT(new != old); */
    }
    CHECKPOINT();
    /* realloc as free */
    for(i = 0; i < NSIZES; i++) {
	regions[i] = arealloc(regions[i], REGION_SIZE(i + 1) + 4, 0);
    }
    CHECKPOINT();
    /* realloc as alloc */
    for(i = 0; i < NSIZES; i++) {
	void *old, *new;
	old = regions[i];
	new = arealloc(old, 0, REGION_SIZE(i));
	ASSERT(new != NULL);
	regions[i] = new;
    }
}

static void test_atryrealloc() {
    int i;
    /* for(i = 0; i < NSIZES; i++) { */
    /* 	ASSERT(atryrealloc(regions[i], REGION_SIZE(i), REGION_SIZE(i) + 4)); */
    /* } */
    for(i = 0; i < NSIZES; i++) {
    	ASSERT(!atryrealloc(regions[i], REGION_SIZE(i) + 4, REGION_SIZE(i + 1) + 4));
    }
}

/*************************/
int run_malloc_h_test_suite() {
    int r;
    void (*void_tests[])() = { test_amalloc, NULL };
    char *void_test_names[] = { "amalloc", NULL };

    void (*mallocd_tests[])() = { test_afree, test_arealloc, test_atryrealloc, NULL };
    char *mallocd_test_names[] = { "afree", "arealloc", "atryrealloc", NULL };

    r = run_test_suite(NULL, void_test_names, void_tests);
    if(r != 0) {
	return r;
    }

    r = run_test_suite(test_mallocd_fixture, mallocd_test_names, mallocd_tests);
    if(r != 0) {
	return r;
    }

    return 0;
}