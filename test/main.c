/*
 * main.c
 *
 * Copyright 2014 Evan Buswell
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
#include <stdio.h>
#include "test.h"
#include "alltests.h"

int
main(int argc __attribute__((unused)),
     char **argv __attribute__((unused))) {
	int r;

	// test_config_nofork();

	r = run_atomic_h_test_suite();
	if (r != 0) {
		fprintf(stderr, "Failed to run tests");
		exit(EXIT_FAILURE);
	}
	r = run_float_h_test_suite();
	if (r != 0) {
		fprintf(stderr, "Failed to run tests");
		exit(EXIT_FAILURE);
	}
	r = run_rcp_h_test_suite();
	if (r != 0) {
		fprintf(stderr, "Failed to run tests");
		exit(EXIT_FAILURE);
	}
	r = run_queue_h_test_suite();
	if (r != 0) {
		fprintf(stderr, "Failed to run tests");
		exit(EXIT_FAILURE);
	}
	r = run_malloc_h_test_suite();
	if (r != 0) {
		fprintf(stderr, "Failed to run tests");
		exit(EXIT_FAILURE);
	}
	r = run_array_h_test_suite();
	if (r != 0) {
		fprintf(stderr, "Failed to run tests");
		exit(EXIT_FAILURE);
	}
	r = print_test_results();
	if (r != 0) {
		fprintf(stderr, "Failed to print test results");
		exit(EXIT_FAILURE);
	}
	if (tests_succeeded()) {
		exit(EXIT_SUCCESS);
	} else {
		exit(EXIT_FAILURE);
	}
}
