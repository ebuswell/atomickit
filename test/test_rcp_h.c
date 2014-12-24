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

#define NTHREADS 16
#define NREPEATS 1000

static bool region1_destroyed;
static bool region2_destroyed;

static bool weakref1_destroyed;
static bool weakref2_destroyed;

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

static void destroy_weakref1(struct arcp_region *region
			     __attribute__((unused))) {
	CHECKPOINT();
	afree(region, sizeof(struct arcp_weakref));
	weakref1_destroyed = true;
}

static void destroy_weakref2(struct arcp_region *region
			     __attribute__((unused))) {
	CHECKPOINT();
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
	region1 = alloca(sizeof(struct arcp_test_region)
			 + strlen(strtest.string1) + 1);
	region2 = alloca(sizeof(struct arcp_test_region)
			 + strlen(strtest.string2) + 1);
	strcpy(region1->data, strtest.string1);
	strcpy(region2->data, strtest.string2);
	test();
}

static void test_arcp_region_init() {
	CHECKPOINT();
	arcp_region_init(region1, destroy_region1);
	ASSERT(region1->destroy == destroy_region1);
	ASSERT(strcmp(region1->data, strtest.string1) == 0);
	CHECKPOINT();
	ASSERT(arcp_usecount(region1) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(region1) == 0);
	ASSERT(!region1_destroyed);
}

/****************************/

static void test_arcp_init_region_fixture(void (*test)()) {
	region1_destroyed = false;
	region2_destroyed = false;
	region1 = alloca(sizeof(struct arcp_test_region)
			 + strlen(strtest.string1) + 1);
	region2 = alloca(sizeof(struct arcp_test_region)
			 + strlen(strtest.string2) + 1);
	strcpy(region1->data, strtest.string1);
	strcpy(region2->data, strtest.string2);
	CHECKPOINT();
	arcp_region_init(region1, destroy_region1);
	CHECKPOINT();
	arcp_region_init(region2, destroy_region2);
	test();
}

static void test_arcp_init() {
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		arcp_t myarcp;
		struct arcp_test_region *rg;
		int i;
		for (i = 0; i < NREPEATS; i++) {
			arcp_init(&myarcp, region1);
			if (i % 50 == 0) {
				COORDINATE_THREADS(NTHREADS) {
					CHECKPOINT();
					ASSERT(arcp_usecount(region1) == 1);
					ASSERT(arcp_storecount(region1) == NTHREADS);
					ASSERT(strcmp(region1->data, strtest.string1) == 0);
					ASSERT(!region1_destroyed);
				} END_COORDINATE_THREADS(NTHREADS);
			}
			rg = (struct arcp_test_region *)
				arcp_load_phantom(&myarcp);
			ASSERT(rg == region1);
			arcp_store(&myarcp, NULL);
		}
	} END_WITH_THREADS(NTHREADS);
}

static void test_arcp_acquire() {
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		int i;
		for (i = 0; i < NREPEATS; i++) {
			struct arcp_test_region *rg;
			rg = arcp_acquire(region1);
			ASSERT(rg == region1);
			if (i % 50 == 0) {
				COORDINATE_THREADS(NTHREADS) {
					CHECKPOINT();
					ASSERT(arcp_usecount(region1) == 1 + NTHREADS);
					ASSERT(arcp_storecount(region1) == 0);
					ASSERT(!region1_destroyed);
					ASSERT(strcmp(region1->data, strtest.string1) == 0);
				} END_COORDINATE_THREADS(NTHREADS);
			}
			arcp_release(region1);
		}
	} END_WITH_THREADS(NTHREADS);
}

static void test_arcp_region_init_weakref() {
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		int r;
		struct arcp_weakref *weakref;
		struct arcp_test_region *rg;
		REPEAT(NREPEATS) {
			r = arcp_region_init_weakref(region1);
			ASSERT(r == 0);
			weakref = arcp_weakref_phantom(region1);
			ASSERT(weakref != NULL);
			ASSERT(arcp_usecount(weakref) == 0);
			ASSERT(arcp_storecount(weakref) == 1);
			rg = (struct arcp_test_region *)
				arcp_weakref_load(weakref);
			ASSERT(rg == region1);
			ASSERT(strcmp(region1->data, strtest.string1) == 0);
			COORDINATE_THREADS(NTHREADS) {
				arcp_region_destroy_weakref(region1);
			} END_COORDINATE_THREADS(NTHREADS);
		} END_REPEAT(NREPEATS);
	} END_WITH_THREADS(NTHREADS);
}

/****************************/
static void test_arcp_init_weakref_fixture(void (*test)()) {
	struct arcp_weakref *weakref1;
	struct arcp_weakref *weakref2;
	region1_destroyed = false;
	region2_destroyed = false;
	weakref1_destroyed = false;
	weakref2_destroyed = false;
	region1 = alloca(sizeof(struct arcp_test_region)
			 + strlen(strtest.string1) + 1);
	region2 = alloca(sizeof(struct arcp_test_region)
			 + strlen(strtest.string2) + 1);
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
	weakref1 = (struct arcp_weakref *)
		arcp_load_phantom(&region1->weakref);
	CHECKPOINT();
	weakref2 = (struct arcp_weakref *)
		arcp_load_phantom(&region2->weakref);
	weakref1->destroy = (arcp_destroy_f) destroy_weakref1;
	weakref2->destroy = (arcp_destroy_f) destroy_weakref2;
	test();
}

static void test_arcp_region_destroy_weakref() {
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		struct arcp_weakref *weakref1;
		REPEAT(NREPEATS) {
			arcp_region_destroy_weakref(region1);
			COORDINATE_THREADS(NTHREADS) {
				ASSERT(arcp_load_phantom(&region1->weakref) == NULL);
				ASSERT(weakref1_destroyed);
				arcp_region_init_weakref(region1);
				weakref1 = (struct arcp_weakref *)
					arcp_load_phantom(&region1->weakref);
				weakref1->destroy = (arcp_destroy_f) destroy_weakref1;
				weakref1_destroyed = false;
			} END_COORDINATE_THREADS(NTHREADS);
		} END_REPEAT(NREPEATS);
	} END_WITH_THREADS(NTHREADS);
}

static void test_arcp_weakref() {
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		struct arcp_weakref *weakref;
		struct arcp_test_region *rg;
		int i;
		for (i = 0; i < NREPEATS; i++) {
			weakref = arcp_weakref(region1);
			ASSERT(weakref != NULL);
			if (i % 50 == 0) {
				COORDINATE_THREADS(NTHREADS) {
					CHECKPOINT();
					ASSERT(arcp_usecount(weakref) == NTHREADS);
					ASSERT(arcp_storecount(weakref) == 1);
					ASSERT(!weakref1_destroyed);
					ASSERT(strcmp(region1->data, strtest.string1) == 0);
				} END_COORDINATE_THREADS(NTHREADS);
			}
			rg = (struct arcp_test_region *)
				arcp_weakref_load(weakref);
			ASSERT(rg == region1);
			arcp_release(rg);
			arcp_release(weakref);
		}
	} END_WITH_THREADS(NTHREADS);
}

static void test_arcp_weakref_phantom() {
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		REPEAT(NREPEATS) {
			struct arcp_weakref *weakref;
			struct arcp_test_region *rg;
			weakref = arcp_weakref_phantom(region1);
			ASSERT(weakref != NULL);
			ASSERT(arcp_usecount(weakref) == 0);
			ASSERT(arcp_storecount(weakref) == 1);
			ASSERT(!weakref1_destroyed);
			rg = (struct arcp_test_region *)
				arcp_weakref_load(weakref);
			ASSERT(rg == region1);
			ASSERT(strcmp(region1->data, strtest.string1) == 0);
			arcp_release(rg);
		} END_REPEAT(NREPEATS);
	} END_WITH_THREADS(NTHREADS);
}

static void test_arcp_weakref_load() {
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		struct arcp_test_region *rg;
		int i;
		for (i = 0; i < NREPEATS; i++) {
			rg = (struct arcp_test_region *)
				arcp_weakref_load(arcp_weakref_phantom(region1));
			ASSERT(rg == region1);
			if (i % 50 == 0) {
				COORDINATE_THREADS(NTHREADS) {
					CHECKPOINT();
					ASSERT(strcmp(region1->data, strtest.string1) == 0);
					ASSERT(arcp_usecount(region1) == NTHREADS + 1);
					ASSERT(arcp_storecount(region1) == 0);
					ASSERT(!region1_destroyed);
					ASSERT(!weakref1_destroyed);
				} END_COORDINATE_THREADS(NTHREADS);
			}
			arcp_release(rg);
		}
	} END_WITH_THREADS(NTHREADS);
}

static void test_arcp_release_with_weakref() {
	arcp_refcount_t rc;
	CHECKPOINT();
	rc.p = ak_load(&region1->refcount, mo_relaxed);
	ASSERT(rc.v.usecount == 1);
	ASSERT(rc.v.destroy_lock == 0);
	ASSERT(rc.v.storecount == 0);
	WITH_THREADS(NTHREADS) {
		REPEAT(NREPEATS) {
			arcp_acquire(region1);
			arcp_release(region1);
			ASSERT(!region1_destroyed);
			ASSERT(!weakref1_destroyed);
		} END_REPEAT(NREPEATS);
	} END_WITH_THREADS(NTHREADS);
	CHECKPOINT();
	arcp_release(region1);
	ASSERT(region1_destroyed);
	ASSERT(weakref1_destroyed);
}

/****************************/

static void test_arcp_init_fixture(void (*test)()) {
	region1_destroyed = false;
	region2_destroyed = false;
	region1 = alloca(sizeof(struct arcp_test_region)
			 + strlen(strtest.string1) + 1);
	region2 = alloca(sizeof(struct arcp_test_region)
			 + strlen(strtest.string1) + 1);
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
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		struct arcp_test_region *rg;
		int i;
		for (i = 0; i < NREPEATS; i++) {
			rg = (struct arcp_test_region *) arcp_load(&arcp);
			ASSERT(rg == region1);
			if (i % 50 == 0) {
				COORDINATE_THREADS(NTHREADS) {
					ASSERT(strcmp(region1->data, strtest.string1) == 0);
					ASSERT(arcp_usecount(region1) == NTHREADS + 1);
					ASSERT(arcp_storecount(region1) == 1);
					ASSERT(!region1_destroyed);
				} END_COORDINATE_THREADS(NTHREADS);
			}
			arcp_release(rg);
		}
	} END_WITH_THREADS(NTHREADS);
}

static void test_arcp_load_phantom() {
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		REPEAT(NREPEATS) {
			struct arcp_test_region *rg;
			rg = (struct arcp_test_region *)
				arcp_load_phantom(&arcp);
			ASSERT(rg == region1);
			ASSERT(strcmp(region1->data, strtest.string1) == 0);
			ASSERT(arcp_usecount(region1) == 1);
			ASSERT(arcp_storecount(region1) == 1);
			ASSERT(!region1_destroyed);
		} END_REPEAT(NREPEATS);
	} END_WITH_THREADS(NTHREADS);
}

static void test_arcp_release() {
	arcp_release(region1);
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		REPEAT(NREPEATS) {
			arcp_acquire(region1);
			arcp_release(region1);
		} END_REPEAT(NREPEATS);
	} END_WITH_THREADS(NTHREADS);
	CHECKPOINT();
	ASSERT(arcp_usecount(region1) == 0);
	CHECKPOINT();
	ASSERT(arcp_storecount(region1) == 1);
	ASSERT(!region1_destroyed);
	ASSERT(strcmp(region1->data, strtest.string1) == 0);
	CHECKPOINT();
	region1 = (struct arcp_test_region *) arcp_load(&arcp);
	CHECKPOINT();
	arcp_store(&arcp, NULL);
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		REPEAT(NREPEATS) {
			arcp_acquire(region1);
			arcp_release(region1);
		} END_REPEAT(NREPEATS);
	} END_WITH_THREADS(NTHREADS);
	CHECKPOINT();
	arcp_release(region1);
	CHECKPOINT();
	ASSERT(arcp_usecount(region1) == 0);
	CHECKPOINT();
	ASSERT(arcp_storecount(region1) == 0);
	ASSERT(region1_destroyed);
	ASSERT(strcmp(region1->data, strtest.string1) == 0);
}

static void test_arcp_cas() {
	struct arcp_test_region *rg, *other;
	/* succeed */
	CHECKPOINT();
	ASSERT(arcp_cas(&arcp, region1, region2));
	CHECKPOINT();
	ASSERT(arcp_usecount(region1) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(region1) == 0);
	CHECKPOINT();
	ASSERT(arcp_usecount(region2) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(region2) == 1);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(strcmp(region1->data, strtest.string1) == 0);
	ASSERT(strcmp(region2->data, strtest.string2) == 0);
	CHECKPOINT();
	rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
	ASSERT(rg == region2);
	/* fail */
	CHECKPOINT();
	ASSERT(!arcp_cas(&arcp, region1, region1));
	CHECKPOINT();
	ASSERT(arcp_usecount(region1) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(region1) == 0);
	CHECKPOINT();
	ASSERT(arcp_usecount(region2) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(region2) == 1);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(strcmp(region1->data, strtest.string1) == 0);
	ASSERT(strcmp(region2->data, strtest.string2) == 0);
	CHECKPOINT();
	rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
	ASSERT(rg == region2);
	/* multithread */
	WITH_THREADS(NTHREADS) {
		struct arcp_test_region *rg;
		REPEAT(NREPEATS) {
			rg = (struct arcp_test_region *)
				arcp_load_phantom(&arcp);
			arcp_cas(&arcp, rg,
				 rg == region1 ? region2 : region1);
		} END_REPEAT(NREPEATS);
	} END_WITH_THREADS(NTHREADS);
	CHECKPOINT();
	rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
	if (rg == region1) {
		other = region2;
	} else {
		ASSERT(rg == region2);
		other = region1;
	}
	CHECKPOINT();
	ASSERT(arcp_usecount(rg) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(rg) == 1);
	CHECKPOINT();
	ASSERT(arcp_usecount(other) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(other) == 0);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(strcmp(region1->data, strtest.string1) == 0);
	ASSERT(strcmp(region2->data, strtest.string2) == 0);
}

static void test_arcp_cas_release_succeed() {
	struct arcp_test_region *rg;
	/* succeed */
	CHECKPOINT();
	ASSERT(arcp_cas_release(&arcp, region1, region2));
	CHECKPOINT();
	ASSERT(arcp_usecount(region1) == 0);
	CHECKPOINT();
	ASSERT(arcp_storecount(region1) == 0);
	CHECKPOINT();
	ASSERT(arcp_usecount(region2) == 0);
	CHECKPOINT();
	ASSERT(arcp_storecount(region2) == 1);
	ASSERT(region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(strcmp(region1->data, strtest.string1) == 0);
	ASSERT(strcmp(region2->data, strtest.string2) == 0);
	CHECKPOINT();
	rg = (struct arcp_test_region *) arcp_load(&arcp);
	ASSERT(rg == region2);
}

static void test_arcp_cas_release_fail() {
	struct arcp_test_region *rg;
	/* fail */
	CHECKPOINT();
	ASSERT(!arcp_cas_release(&arcp, region2, region1));
	CHECKPOINT();
	ASSERT(arcp_usecount(region1) == 0);
	CHECKPOINT();
	ASSERT(arcp_storecount(region1) == 1);
	CHECKPOINT();
	ASSERT(arcp_usecount(region2) == 0);
	CHECKPOINT();
	ASSERT(arcp_storecount(region2) == 0);
	ASSERT(!region1_destroyed);
	ASSERT(region2_destroyed);
	ASSERT(strcmp(region1->data, strtest.string1) == 0);
	ASSERT(strcmp(region2->data, strtest.string2) == 0);
	CHECKPOINT();
	rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
	ASSERT(rg == region1);
}

static void test_arcp_cas_release_multithread() {
	struct arcp_test_region *rg;
	struct arcp_test_region *other;
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		struct arcp_test_region *rg;
		struct arcp_test_region *other;
		REPEAT(NREPEATS) {
			do {
				rg = (struct arcp_test_region *)
					arcp_load(&arcp);
				other = rg == region1 ? arcp_acquire(region2)
					              : arcp_acquire(region1);
			} while (!arcp_cas_release(&arcp, rg, other));
		} END_REPEAT(NREPEATS);
	} END_WITH_THREADS(NTHREADS);
	CHECKPOINT();
	rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
	if (rg == region1) {
		other = region2;
	} else {
		ASSERT(rg == region2);
		other = region1;
	}
	CHECKPOINT();
	ASSERT(arcp_usecount(rg) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(rg) == 1);
	CHECKPOINT();
	ASSERT(arcp_usecount(other) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(other) == 0);
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	ASSERT(strcmp(region1->data, strtest.string1) == 0);
	ASSERT(strcmp(region2->data, strtest.string2) == 0);
}

static void test_arcp_store() {
	struct arcp_test_region *rg;
	struct arcp_test_region *other;
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		struct arcp_test_region *rg;
		struct arcp_test_region *other;
		REPEAT(NREPEATS) {
			rg = (struct arcp_test_region *)
				arcp_load_phantom(&arcp);
			other = rg == region1 ? region2
					      : region1;
			arcp_store(&arcp, other);
		} END_REPEAT(NREPEATS);
	} END_WITH_THREADS(NTHREADS);
	CHECKPOINT();
	rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
	if (rg == region1) {
		other = region2;
	} else {
		ASSERT(rg == region2);
		other = region1;
	}
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	CHECKPOINT();
	ASSERT(arcp_usecount(rg) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(rg) == 1);
	CHECKPOINT();
	ASSERT(arcp_usecount(other) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(other) == 0);
	ASSERT(strcmp(region1->data, strtest.string1) == 0);
	ASSERT(strcmp(region2->data, strtest.string2) == 0);
	CHECKPOINT();
	arcp_release(rg);
	CHECKPOINT();
	arcp_store(&arcp, other);
	ASSERT(other == region1 ? !region1_destroyed : !region2_destroyed);
	ASSERT(rg == region1 ? region1_destroyed : region2_destroyed);
	CHECKPOINT();
	ASSERT(arcp_usecount(other) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(other) == 1);
	CHECKPOINT();
	ASSERT(arcp_usecount(rg) == 0);
	CHECKPOINT();
	ASSERT(arcp_storecount(rg) == 0);
	ASSERT(strcmp(region1->data, strtest.string1) == 0);
	ASSERT(strcmp(region2->data, strtest.string2) == 0);
	CHECKPOINT();
	rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
	ASSERT(rg == other);
}

static void test_arcp_swap() {
	struct arcp_test_region *other;
	struct arcp_test_region *rg;
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		struct arcp_test_region *rg;
		struct arcp_test_region *other;
		other = arcp_acquire(region2);
		REPEAT(NREPEATS) {
			rg = (struct arcp_test_region *) arcp_swap(&arcp, other);
			arcp_release(other);
			other = rg == region1 ? arcp_acquire(region2)
					      : arcp_acquire(region1);
			arcp_release(rg);
		} END_REPEAT(NREPEATS);
		arcp_release(other);
	} END_WITH_THREADS(NTHREADS);
	CHECKPOINT();
	rg = (struct arcp_test_region *) arcp_load_phantom(&arcp);
	if (rg == region1) {
		other = region2;
	} else {
		ASSERT(rg == region2);
		other = region1;
	}
	ASSERT(!region1_destroyed);
	ASSERT(!region2_destroyed);
	CHECKPOINT();
	ASSERT(arcp_usecount(rg) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(rg) == 1);
	CHECKPOINT();
	ASSERT(arcp_usecount(other) == 1);
	CHECKPOINT();
	ASSERT(arcp_storecount(other) == 0);
	ASSERT(strcmp(region1->data, strtest.string1) == 0);
	ASSERT(strcmp(region2->data, strtest.string2) == 0);
}

int run_rcp_h_test_suite() {
	int r;
	void (*arcp_uninit_tests[])() = { test_arcp_region_init, NULL };
	char *arcp_uninit_test_names[] = { "arcp_region_init", NULL };

	void (*arcp_init_region_tests[])() = { test_arcp_init,
					       test_arcp_acquire,
					       test_arcp_region_init_weakref,
					       NULL };
	char *arcp_init_region_test_names[] = { "arcp_init", "arcp_acquire",
						"arcp_region_init_weakref",
						NULL };

	void (*arcp_init_weakref_tests[])()
		= { test_arcp_region_destroy_weakref, test_arcp_weakref,
		    test_arcp_weakref_phantom, test_arcp_weakref_load,
		    test_arcp_release_with_weakref, NULL };
	char *arcp_init_weakref_test_names[]
		= { "arcp_region_destroy_weakref", "arcp_weakref",
		    "arcp_weakref_phantom", "arcp_weakref_load",
		    "arcp_release_with_weakref", NULL };

	void (*arcp_init_tests[])() = { test_arcp_load,
					test_arcp_load_phantom, test_arcp_cas,
					test_arcp_cas_release_succeed,
					test_arcp_cas_release_fail,
					test_arcp_cas_release_multithread,
					test_arcp_store, test_arcp_release,
					test_arcp_swap, NULL };

	char *arcp_init_test_names[] = { "arcp_load", "arcp_load_phantom",
					 "arcp_cas",
					 "arcp_cas_release_succeed",
					 "arcp_cas_release_fail",
					 "arcp_cas_release_multithread",
					 "arcp_store", "arcp_release",
					 "arcp_swap", NULL };

	r = run_test_suite(test_arcp_uninit_fixture,
			   arcp_uninit_test_names, arcp_uninit_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_arcp_init_region_fixture,
			   arcp_init_region_test_names,
			   arcp_init_region_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_arcp_init_weakref_fixture,
			   arcp_init_weakref_test_names,
			   arcp_init_weakref_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_arcp_init_fixture,
			   arcp_init_test_names, arcp_init_tests);
	if (r != 0) {
		return r;
	}

	return 0;
}
