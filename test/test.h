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

/**
 * The resulting status of a given test.
 *
 * `PASS` - the test passed.
 * `FAIL` - the test failed.
 * `UNRESOLVED` - the status of the test could not be determined,
 * usually because of an unexpected error somewhere.
 * `UNTESTED` - the test was not run for some reason, usually because
 * the test has not yet been written.
 * `UNSUPPORTED` - the test is not supported on this system
 * configuraiton.
 */
enum test_status { TEST_PASS, TEST_FAIL, TEST_UNRESOLVED, TEST_UNTESTED, TEST_UNSUPPORTED };

/**
 * The result of a given test.
 */
struct test_result {
    char *test_name; /** The name of the test */
    enum test_status status; /** The resulting status of the test */
    char *explanation; /** An explanation for the test's status */
    char *file; /** The file in which the test failed, NULL on success */
    char *line; /** The line in which the test failed, NULL on success */
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
 * The `FILE *` for communicating between the test and the test
 * framework.
 */
extern FILE *test_writer;

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
struct test_result *test_result_create(char *test_name, enum test_status result, char *explanation, char *file, char *line);

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
int run_test(void (*fixture)(void (*)()), char *test_name, void (*test)());

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
int run_test_suite(void (*fixture)(void (*)()), char **test_name, void (**test)());
/**
 * Print a summary of the test results to the standard output.
 *
 * @returns zero on success, nonzero on failure.
 */
int print_test_results();
/**
 * Determine whether all the tests have succeeded or not.
 *
 * @returns true if all tests have succeeded, false otherwise.
 */
bool tests_succeeded();

/**
 * Checkpoint the test in case of unexpected error (e.g. SEGFAULT),
 * recording the current file and line number.
 */
#define CHECKPOINT()							\
    do {								\
	if((fprintf(test_writer, "CHECKPOINT:%s:%d\n", __FILE__, __LINE__) < 0) \
	   || (fflush(test_writer) != 0)) {				\
	    fclose(test_writer);					\
	    exit(EXIT_FAILURE);						\
	}								\
    } while(0)								\

/**
 * Assert that some expression is true, failing the test otherwise.
 */
#define ASSERT(assertion)						\
    do {								\
	CHECKPOINT();							\
	if(!(assertion)) {						\
	    if(fprintf(test_writer, "FAIL:Assertion failed: %s\n", #assertion) < 0) { \
		fclose(test_writer);					\
		exit(EXIT_FAILURE);					\
	    }								\
	    if(fclose(test_writer) != 0) {				\
		exit(EXIT_FAILURE);					\
	    }								\
	    exit(EXIT_SUCCESS);						\
	}								\
    } while(0)

/**
 * Unconditionally fail the test.
 */
#define FAIL(reason)							\
    do {								\
	CHECKPOINT();							\
	if(fprintf(test_writer, "FAIL:%s\n", (reason)) < 0) {		\
	    fclose(test_writer);					\
	    exit(EXIT_FAILURE);						\
	}								\
	if(fclose(test_writer) != 0) {					\
	    exit(EXIT_FAILURE);						\
	}								\
	exit(EXIT_SUCCESS);						\
    } while(0)

/**
 * Unconditionally abort the test, setting the status to UNRESOLVED.
 */
#define UNRESOLVED(reason)						\
    do {								\
	CHECKPOINT();							\
	if(fprintf(test_writer, "UNRESOLVED:%s\n", (reason)) < 0) {	\
	    fclose(test_writer);					\
	    exit(EXIT_FAILURE);						\
	}								\
	if(fclose(test_writer) != 0) {					\
	    exit(EXIT_FAILURE);						\
	}								\
	exit(EXIT_SUCCESS);						\
    } while(0)

/**
 * Unconditionally abort the test, setting the status to UNTESTED.
 */
#define UNTESTED(reason)						\
    do {								\
	CHECKPOINT();							\
	if(fprintf(test_writer, "UNTESTED:%s\n", (reason)) < 0) {	\
	    fclose(test_writer);					\
	    exit(EXIT_FAILURE);						\
	}								\
	if(fclose(test_writer) != 0) {					\
	    exit(EXIT_FAILURE);						\
	}								\
	exit(EXIT_SUCCESS);						\
    } while(0)

/**
 * Unconditionally abort the test, setting the status to UNSUPPORTED.
 */
#define UNSUPPORTED(reason)						\
    do {								\
	CHECKPOINT();							\
	if(fprintf(test_writer, "UNSUPPORTED:%s\n", (reason)) < 0) {	\
	    fclose(test_writer);					\
	    exit(EXIT_FAILURE);						\
	}								\
	if(fclose(test_writer) != 0) {					\
	    exit(EXIT_FAILURE);						\
	}								\
	exit(EXIT_SUCCESS);						\
    } while(0)

/**
 * Abort the test, setting the status to PASS.
 *
 * It's recommended that you don't use this, instead allowing the test
 * to complete through the normal means, by returning from the test
 * function without aborting or setting another result status.
 */
#define PASS(reason)							\
    do {								\
	CHECKPOINT();							\
	if(fprintf(test_writer, "PASS:%s\n", (reason)) < 0) {	\
	    fclose(test_writer);					\
	    exit(EXIT_FAILURE);						\
	}								\
	if(fclose(test_writer) != 0) {					\
	    exit(EXIT_FAILURE);						\
	}								\
	exit(EXIT_SUCCESS);						\
    } while(0)

#endif /* ! TEST_H */
