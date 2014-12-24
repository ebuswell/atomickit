/*
 * test_float_h.c
 *
 * Copyright 2013 Evan Buswell
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
#include <atomickit/float.h>
#include "alltests.h"
#include "test.h"

static volatile atomic_float afloat;
static volatile atomic_double adouble;

static void test_ak_float_init() {
	CHECKPOINT();
	ak_float_init(&afloat, 2.1f);
	CHECKPOINT();
	ASSERT(ak_float_load(&afloat, mo_seq_cst) == 2.1f);
}

static void test_ak_float_is_nb() {
	CHECKPOINT();
	(void) ak_float_is_nb(&afloat);
}

/******************************************************/

static void test_ak_float_fixture(void (*test)()) {
	CHECKPOINT();
	ak_float_init(&afloat, 2.1f);
	test();
}

static void test_ak_float_store() {
	CHECKPOINT();
	ak_float_store(&afloat, 3.1f, mo_seq_cst);
	CHECKPOINT();
	ASSERT(ak_float_load(&afloat, mo_seq_cst) == 3.1f);
}

static void test_ak_float_load() {
	CHECKPOINT();
	ASSERT(ak_float_load(&afloat, mo_seq_cst) == 2.1f);
}

static void test_ak_float_swap() {
	float fr;
	CHECKPOINT();
	fr = ak_float_swap(&afloat, 3.1f, mo_seq_cst);
	ASSERT(fr == 2.1f);
	CHECKPOINT();
	ASSERT(ak_float_load(&afloat, mo_seq_cst) == 3.1f);
}

static void test_ak_float_cas_strong() {
	float fr = 1.1f;
	CHECKPOINT();
	ASSERT(!ak_float_cas_strong(&afloat, &fr, 3.1f,
	                            mo_seq_cst, mo_seq_cst));
	ASSERT(fr == 2.1f);
	CHECKPOINT();
	ASSERT(ak_float_cas_strong(&afloat, &fr, 3.1f,
	                           mo_seq_cst, mo_seq_cst));
	ASSERT(fr == 2.1f);
	CHECKPOINT();
	ASSERT(ak_float_load(&afloat, mo_seq_cst) == 3.1f);
}

static void test_ak_float_cas() {
	float fr = 1.1f;
	CHECKPOINT();
	ASSERT(!ak_float_cas(&afloat, &fr, 3.1f, mo_seq_cst, mo_seq_cst));
	ASSERT(fr == 2.1f);
	CHECKPOINT();
	ASSERT(ak_float_cas(&afloat, &fr, 3.1f, mo_seq_cst, mo_seq_cst));
	ASSERT(fr == 2.1f);
	CHECKPOINT();
	ASSERT(ak_float_load(&afloat, mo_seq_cst) == 3.1f);
}

/******************************************************/

static void test_ak_double_init() {
	CHECKPOINT();
	ak_double_init(&adouble, 2.1);
	CHECKPOINT();
	ASSERT(ak_double_load(&adouble, mo_seq_cst) == 2.1);
}

static void test_ak_double_is_nb() {
	CHECKPOINT();
	(void) ak_double_is_nb(&adouble);
}

/******************************************************/

static void test_ak_double_fixture(void (*test)()) {
	CHECKPOINT();
	ak_double_init(&adouble, 2.1);
	test();
}

static void test_ak_double_store() {
	CHECKPOINT();
	ak_double_store(&adouble, 3.1, mo_seq_cst);
	CHECKPOINT();
	ASSERT(ak_double_load(&adouble, mo_seq_cst) == 3.1);
}

static void test_ak_double_load() {
	CHECKPOINT();
	ASSERT(ak_double_load(&adouble, mo_seq_cst) == 2.1);
}

static void test_ak_double_swap() {
	double dr;
	CHECKPOINT();
	dr = ak_double_swap(&adouble, 3.1, mo_seq_cst);
	ASSERT(dr == 2.1);
	CHECKPOINT();
	ASSERT(ak_double_load(&adouble, mo_seq_cst) == 3.1);
}

static void test_ak_double_cas_strong() {
	double dr = 1.1;
	CHECKPOINT();
	ASSERT(!ak_double_cas_strong(&adouble, &dr, 3.1,
	                             mo_seq_cst, mo_seq_cst));
	ASSERT(dr == 2.1);
	CHECKPOINT();
	ASSERT(ak_double_cas_strong(&adouble, &dr, 3.1,
	                            mo_seq_cst, mo_seq_cst));
	ASSERT(dr == 2.1);
	CHECKPOINT();
	ASSERT(ak_double_load(&adouble, mo_seq_cst) == 3.1);
}

static void test_ak_double_cas() {
	double dr = 1.1;
	CHECKPOINT();
	ASSERT(!ak_double_cas(&adouble, &dr, 3.1, mo_seq_cst, mo_seq_cst));
	ASSERT(dr == 2.1);
	CHECKPOINT();
	ASSERT(ak_double_cas(&adouble, &dr, 3.1, mo_seq_cst, mo_seq_cst));
	ASSERT(dr == 2.1);
	CHECKPOINT();
	ASSERT(ak_double_load(&adouble, mo_seq_cst) == 3.1);
}


int run_float_h_test_suite() {
	int r;
	void (*ak_float_blank_tests[])()
		= { test_ak_float_init, test_ak_float_is_nb,
		    NULL };
	char *ak_float_blank_test_names[]
		= { "ak_float_init", "ak_float_is_nb", NULL };

	void (*ak_float_tests[])()
		= { test_ak_float_store, test_ak_float_load,
		    test_ak_float_swap, test_ak_float_cas_strong,
		    test_ak_float_cas, NULL };
	char *ak_float_test_names[]
		= { "ak_float_store", "ak_float_load",
		    "ak_float_swap", "ak_float_cas_strong",
		    "ak_float_cas", NULL };
	void (*ak_double_blank_tests[])()
		= { test_ak_double_init, test_ak_double_is_nb,
		    NULL };
	char *ak_double_blank_test_names[]
 		= { "ak_double_init", "ak_double_is_nb", NULL };

	void (*ak_double_tests[])()
		= { test_ak_double_store, test_ak_double_load,
		    test_ak_double_swap, test_ak_double_cas_strong,
		    test_ak_double_cas, NULL };
	char *ak_double_test_names[]
		= { "ak_double_store", "ak_double_load",
		    "ak_double_swap", "ak_double_cas_strong",
		    "ak_double_cas", NULL };

	r = run_test_suite(NULL,
	                   ak_float_blank_test_names, ak_float_blank_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_ak_float_fixture,
	                   ak_float_test_names, ak_float_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(NULL,
	                   ak_double_blank_test_names, ak_double_blank_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_ak_double_fixture,
	                   ak_double_test_names, ak_double_tests);
	if (r != 0) {
		return r;
	}

	return 0;
}
