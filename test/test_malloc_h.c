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
#define NTHREADS 16
#define NREPEATS 1000

static void *regions[NSIZES + 1][NTHREADS];

#define REGION_SIZE(i) (16 * (2 << (i)) - 8)

#define CHECK_MATRIX() do {						\
	int __i;							\
	for (__i = 0; __i < NSIZES; __i++) {				\
		int __j;						\
		for (__j = 0; __j < NTHREADS; __j++) {			\
			int __k;					\
			ASSERT(regions[__i][__j] != NULL);		\
			for (__k = 0; __k < NSIZES; __k++) {		\
				int __l;				\
				for (__l = 0; __l < NTHREADS; __l++) {	\
					if (__i != __k && __j != __l) {	\
						ASSERT(regions[__i][__j] != regions[__k][__l]); \
					}				\
				}					\
			}						\
		}							\
	}								\
} while (0)

#define CHECK_MATRIX0() do {						\
	int __i;							\
	for (__i = 0; __i < NSIZES; __i++) {				\
		int __k;						\
		ASSERT(regions[__i][0] != NULL);			\
		for (__k = 0; __k < NSIZES; __k++) {			\
			if (__i != __k) {				\
				ASSERT(regions[__i][0] != regions[__k][0]); \
			}						\
		}							\
	}								\
} while (0)
/*************************/
static void test_amalloc() {
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		REPEAT(NREPEATS) {
			int i;
			for (i = 0; i < NSIZES; i++) {
				regions[i][thread_number]
					= amalloc(REGION_SIZE(i));
			}
			COORDINATE_THREADS(NTHREADS) {
				CHECK_MATRIX();
				for (i = 0; i < NSIZES; i++) {
					int j;
					for (j = 0; j < NTHREADS; j++) {
						afree(regions[i][j],
						      REGION_SIZE(i));
					}
				}
			} END_COORDINATE_THREADS(NTHREADS);
		} END_REPEAT(NREPEATS);
	} END_WITH_THREADS(NTHREADS);
	CHECKPOINT();
	{
		int i;
		for (i = 0; i < NSIZES; i++) {
			int j;
			for (j = 0; j < NTHREADS; j++) {
				regions[i][j] = amalloc(REGION_SIZE(i));
			}
		}
	}
	CHECKPOINT();
	WITH_THREADS(NTHREADS) {
		REPEAT(NREPEATS) {
			int i;
			for (i = 0; i < NSIZES; i++) {
				afree(regions[i][thread_number],
				      REGION_SIZE(i));
				regions[i][thread_number]
					= amalloc(REGION_SIZE(i));
			}
		} END_REPEAT(NREPEATS);
	} END_WITH_THREADS(NTHREADS);
	CHECK_MATRIX();
	CHECKPOINT();
	{
		int i;
		for (i = 0; i < NSIZES; i++) {
			int j;
			for (j = 0; j < NTHREADS; j++) {
				afree(regions[i][j], REGION_SIZE(i));
			}
		}
	}
}

/*************************/
static void test_mallocd_fixture(void (*test)()) {
	int i;
	CHECKPOINT();
	for (i = 0; i < NSIZES; i++) {
		regions[i][0] = amalloc(REGION_SIZE(i));
	}
	test();
}

static void test_afree() {
	int i;
	CHECKPOINT();
	for (i = 0; i < NSIZES; i++) {
		 afree(regions[i][0], REGION_SIZE(i));
	}
}

static void test_arealloc() {
	int i;
	CHECKPOINT();
	/* realloc within the same chunk */
	for (i = 0; i < NSIZES; i++) {
		regions[i][0] = arealloc(regions[i][0],
					 REGION_SIZE(i), REGION_SIZE(i) + 4);
	}
	CHECK_MATRIX0();
	CHECKPOINT();
	/* realloc to a different chunk */
	for (i = 0; i < NSIZES; i++) {
		regions[i][0] = arealloc(regions[i][0], REGION_SIZE(i) + 4,
					 REGION_SIZE(i + 1) + 4);
	}
	CHECK_MATRIX0();
	CHECKPOINT();
	/* realloc as free */
	for (i = 0; i < NSIZES; i++) {
		regions[i][0] = arealloc(regions[i][0],
					 REGION_SIZE(i + 1) + 4, 0);
	}
	CHECKPOINT();
	/* realloc as alloc */
	for (i = 0; i < NSIZES; i++) {
		regions[i][0] = arealloc(regions[i][0], 0, REGION_SIZE(i));
	}
	CHECK_MATRIX0();
}

static void test_atryrealloc() {
	int i;
	CHECKPOINT();
	for (i = 0; i < NSIZES; i++) {
		ASSERT(!atryrealloc(regions[i][0], REGION_SIZE(i),
				    REGION_SIZE(i + 1)));
	}
}

/*************************/
int run_malloc_h_test_suite() {
	int r;
	void (*void_tests[])() = { test_amalloc, NULL };
	char *void_test_names[] = { "amalloc", NULL };

	void (*mallocd_tests[])() = { test_afree, test_arealloc,
				      test_atryrealloc, NULL };
	char *mallocd_test_names[] = { "afree", "arealloc", "atryrealloc",
				       NULL };

	r = run_test_suite(NULL, void_test_names, void_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_mallocd_fixture, mallocd_test_names,
			   mallocd_tests);
	if (r != 0) {
		return r;
	}

	return 0;
}
