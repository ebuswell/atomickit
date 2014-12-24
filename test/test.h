/** @file test.h
 * Unit testing framework.
 */
/*
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
#ifndef TEST_H
#define TEST_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <atomickit/atomic.h>
#include "test.h"

/**
 * The resulting status of a given test.
 *
 * `TEST_PASS` - the test passed.
 * `TEST_FAIL` - the test failed.
 * `TEST_UNRESOLVED` - the status of the test could not be determined,
 * usually because of an unexpected error somewhere.
 * `TEST_UNTESTED` - the test was not run for some reason, usually
 * because the test has not yet been written.
 * `TEST_UNSUPPORTED` - the test is not supported on this system
 * configuraiton.
 */
enum test_status { TEST_PASS, TEST_FAIL, TEST_UNRESOLVED, TEST_UNTESTED,
		   TEST_UNSUPPORTED };

/**
 * The result of a given test.
 */
struct test_result {
	char *test_name; /** The name of the test */
	enum test_status status; /** The resulting status of the test */
	char *explanation; /** An explanation for the test's status, can be
			    *  empty but not NULL */
	char *file; /** The file in which the test failed, ignored on
		     *  success */
	int line; /** The line in which the test failed, ignored on success */
};

/**
 * The list of test results.
 */
struct test_result_list {
	size_t nresults;
	size_t results_capacity;
	struct test_result *results[1];
};

/**
 * The list of test results.
 */
extern struct test_result_list *test_results;

/**
 * Add a result to the test_results list.
 *
 * Should only be used by the test framework unless you know what you
 * are doing.
 *
 * @param result the result to add.
 *
 * @returns zero on success, nonzero on failure.
 */
int test_results_push(struct test_result *result);

/**
 * Create a test result from the associated data.
 */
struct test_result *test_result_create(char *test_name,
				       enum test_status result,
				       char *explanation,
				       char *file, int line);

/**
 * Free a previously allocated test result and all associated strings.
 */
void test_result_free(struct test_result *result);

/**
 * Record a test result but don't actually run a test.
 *
 * @param test_name the name of the test.
 * @param result the resulting status of the test.
 * @param explanation the explanation for the resulting status.
 *
 * @returns zero on success, nonzero on failure.
 */
int no_test(char *test_name, enum test_status result, char *explanation);

/**
 * Run a test and record the result.
 *
 * @param fixture the fixture with which to run the test.
 * @param test_name the name of the test.
 * @param test the test to run.
 *
 * @returns zero on success, nonzero on failure.
 */
extern int (*run_test)(void (*fixture)(void (*)()),
		       char *test_name, void (*test)());

/**
 * Run a test suite and record the results.
 *
 * @param fixture the fixture with which to run each test.
 * @param test_name a NULL-terminated array of the names of each test
 * to run.
 * @param test an array of the tests corresponding to each test name.
 * NULL if there is no test for that test name (yet).
 *
 * @returns zero on success, nonzero on failure.
 */
int run_test_suite(void (*fixture)(void (*)()),
		   char **test_name, void (**test)());

/**
 * Print a summary of the test results to the standard output.
 *
 * @returns zero on success, nonzero on failure.
 */
int print_test_results(void);

/**
 * Determine whether all the tests have succeeded or not.
 *
 * @returns true if all tests have succeeded, false otherwise.
 */
bool tests_succeeded(void);

/**
 * Set the test framework to run the tests in the calling thread.
 */
void test_config_nofork(void);

/**
 * Checkpoint the test in case of unexpected error.
 *
 * Unless you know what you're doing, use the macro version instead.
 */
extern void (*checkpoint_test)(char *file, int line);

/**
 * End the test with the given status and explanation.
 *
 * Unless you know what you're doing, use the macro versions instead.
 */
extern void (*end_test)(enum test_status status, char *explanation);

/**
 * Checkpoint the test in case of unexpected error (e.g. SEGFAULT),
 * recording the current file and line number.
 */
#define CHECKPOINT() do {						\
	checkpoint_test(__FILE__, __LINE__);				\
} while (0)

/**
 * Assert that some expression is true, failing the test otherwise.
 */
#define ASSERT(assertion) do {						\
	if (!(assertion)) {						\
		char __buf[256];					\
		CHECKPOINT();						\
		if (snprintf(__buf, 256, "Assertion failed: %s",	\
			     #assertion) >= 256) {			\
			end_test(TEST_UNRESOLVED,			\
				 "Buffer overflow: assertion too long"); \
	    	}							\
		end_test(TEST_FAIL, __buf);				\
	}								\
} while (0)

/**
 * Unconditionally fail the test.
 */
#define FAIL(reason) do {						\
	CHECKPOINT();							\
	end_test(TEST_FAIL, reason);					\
} while (0)

/**
 * Unconditionally abort the test, setting the status to UNRESOLVED.
 */
#define UNRESOLVED(reason) do {						\
	CHECKPOINT();							\
	end_test(TEST_UNRESOLVED, reason);				\
} while (0)

/**
 * Unconditionally abort the test, setting the status to UNTESTED.
 */
#define UNTESTED(reason) do {						\
	CHECKPOINT();							\
	end_test(TEST_UNTESTED, reason);				\
} while (0)

/**
 * Unconditionally abort the test, setting the status to UNSUPPORTED.
 */
#define UNSUPPORTED(reason) do {					\
	CHECKPOINT();							\
	end_test(TEST_UNSUPPORTED, reason);				\
} while (0)

/**
 * Abort the test, setting the status to PASS.
 *
 * It's recommended that you don't use this, instead allowing the test
 * to complete through the normal means, by returning from the test
 * function without aborting or setting another result status.
 */
#define PASS(reason) do {						\
	CHECKPOINT();							\
	end_test(TEST_PASS, reason);					\
} while (0)

#define REPEAT(n) do {							\
	int __i;							\
	for (__i = 0; __i < n; __i++) {

#define END_REPEAT(n)							\
	}								\
} while (0)

#define COORDINATE_THREADS(x) do { 					\
	int __mycoordv = ak_load(&__coordv, mo_acquire);		\
	if ((ak_ldadd(&__coord, 1, mo_acq_rel) + 1) == __mycoordv) {	\

#define END_COORDINATE_THREADS(x)					\
		ak_store(&__coordv, __mycoordv + x, mo_release);	\
	} else {							\
		while (ak_load(&__coordv, mo_acquire) == __mycoordv) {	\
			cpu_yield();					\
		}							\
	}								\
} while (0)

#define ONE_THREAD(x) do { 						\
	if (thread_number == 0) {

#define END_ONE_THREAD(x)						\
	}								\
} while (0)

#define WITH_THREADS(x) do {						\
	pthread_t __threads[x];						\
	int __r;							\
	intptr_t __i;							\
	atomic_int __pistol = ATOMIC_VAR_INIT(0);			\
	atomic_int __coord = ATOMIC_VAR_INIT(0);			\
	atomic_int __coordv = ATOMIC_VAR_INIT(x);			\
	void *__start_routine(intptr_t thread_number) {			\
		(void)(thread_number);					\
		(void)(__coord);					\
		(void)(__coordv);					\
		while (ak_load(&__pistol, mo_acquire) < x) {		\
			cpu_yield();					\
		}

#define END_WITH_THREADS(x)						\
		return NULL;						\
	}								\
	for (__i = 0; __i < x; __i++) {					\
		__r = pthread_create(&__threads[__i], NULL,		\
				     (void *(*)(void *)) __start_routine, \
		                     (void *) __i);			\
		if (__r != 0) {						\
			FAIL("Failed to create threads");		\
		}							\
	}								\
	ak_store(&__pistol, x, mo_release);				\
	for (__i = 0; __i < x; __i++) {					\
		__r = pthread_join(__threads[__i], NULL);		\
		if (__r != 0) {						\
			FAIL("Failed to rejoin threads");		\
		}							\
	}								\
} while (0)
#endif /* ! TEST_H */
