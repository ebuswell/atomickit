/*
 * test_atomic_h.c
 * 
 * Copyright 2013 Evan Buswell
 * 
 * This file is part of Atomic Kit.
 * 
 * Atomic Kit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License,
 * or (at your option) any later version.
 * 
 * Atomic Kit is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "atomickit/atomic.h"
#include "alltests.h"
#include "test.h"

static volatile atomic_bool abool = ATOMIC_VAR_INIT(false);
static volatile atomic_char achar = ATOMIC_VAR_INIT('\0');
static volatile atomic_schar aschar = ATOMIC_VAR_INIT(0);
static volatile atomic_uchar auchar = ATOMIC_VAR_INIT(0);
static volatile atomic_short ashort = ATOMIC_VAR_INIT(0);
static volatile atomic_ushort aushort = ATOMIC_VAR_INIT(0);
static volatile atomic_int aint = ATOMIC_VAR_INIT(0);
static volatile atomic_uint auint = ATOMIC_VAR_INIT(0);
static volatile atomic_long along = ATOMIC_VAR_INIT(0);
static volatile atomic_ulong aulong = ATOMIC_VAR_INIT(0);
static volatile atomic_wchar_t awchar = ATOMIC_VAR_INIT('\0');
static volatile atomic_intptr_t aintptr = ATOMIC_VAR_INIT(0);
static volatile atomic_uintptr_t auintptr = ATOMIC_VAR_INIT(0);
static volatile atomic_size_t asize = ATOMIC_VAR_INIT(0);
static volatile atomic_ptrdiff_t aptrdiff = ATOMIC_VAR_INIT(0);
static volatile atomic_intmax_t aintmax = ATOMIC_VAR_INIT(0);
static volatile atomic_uintmax_t auintmax = ATOMIC_VAR_INIT(0);
static volatile atomic_flag aflag = ATOMIC_FLAG_INIT;

static void test_ak_init() {
	CHECKPOINT();
	ak_init(&aint, 21);
	CHECKPOINT();
	ASSERT(ak_load(&aint, mo_relaxed) == 21);
}

static void test_ak_fence() {
	CHECKPOINT();
	ak_fence(mo_relaxed);
	CHECKPOINT();
	ak_fence(mo_consume);
	CHECKPOINT();
	ak_fence(mo_acquire);
	CHECKPOINT();
	ak_fence(mo_release);
	CHECKPOINT();
	ak_fence(mo_acq_rel);
	CHECKPOINT();
	ak_fence(mo_seq_cst);
}

static void test_ak_sigfence() {
	CHECKPOINT();
	ak_sigfence(mo_relaxed);
	CHECKPOINT();
	ak_sigfence(mo_consume);
	CHECKPOINT();
	ak_sigfence(mo_acquire);
	CHECKPOINT();
	ak_sigfence(mo_release);
	CHECKPOINT();
	ak_sigfence(mo_acq_rel);
	CHECKPOINT();
	ak_sigfence(mo_seq_cst);
}

static void test_ak_is_nb() {
	_Bool b;
	CHECKPOINT();
	b = ak_is_nb(&abool);
	ASSERT(!b == !ATOMIC_BOOL_LOCK_FREE);
	CHECKPOINT();
	b = ak_is_nb(&achar);
	ASSERT(!b == !ATOMIC_CHAR_LOCK_FREE);
	CHECKPOINT();
	b = ak_is_nb(&aschar);
	CHECKPOINT();
	b = ak_is_nb(&auchar);
	CHECKPOINT();
	b = ak_is_nb(&ashort);
	ASSERT(!b == !ATOMIC_SHORT_LOCK_FREE);
	CHECKPOINT();
	b = ak_is_nb(&aushort);
	CHECKPOINT();
	b = ak_is_nb(&aint);
	ASSERT(!b == !ATOMIC_INT_LOCK_FREE);
	CHECKPOINT();
	b = ak_is_nb(&auint);
	CHECKPOINT();
	b = ak_is_nb(&along);
	ASSERT(!b == !ATOMIC_LONG_LOCK_FREE);
	CHECKPOINT();
	b = ak_is_nb(&aulong);
	CHECKPOINT();
	b = ak_is_nb(&awchar);
	ASSERT(!b == !ATOMIC_WCHAR_T_LOCK_FREE);
	CHECKPOINT();
	b = ak_is_nb(&aintptr);
	ASSERT(!b == !ATOMIC_POINTER_LOCK_FREE);
	CHECKPOINT();
	b = ak_is_nb(&auintptr);
	CHECKPOINT();
	b = ak_is_nb(&asize);
	CHECKPOINT();
	b = ak_is_nb(&aptrdiff);
	CHECKPOINT();
	b = ak_is_nb(&aintmax);
	CHECKPOINT();
	b = ak_is_nb(&auintmax);
}

/*****************************************************/

static void test_atomic_int_fixture(void (*test)()) {
	CHECKPOINT();
	ak_init(&aint, 21);
	test();
}

static void test_ak_store() {
	CHECKPOINT();
	ak_store(&aint, 31, mo_relaxed);
	CHECKPOINT();
	ASSERT(ak_load(&aint, mo_relaxed) == 31);
}

static void test_ak_load() {
	CHECKPOINT();
	ASSERT(ak_load(&aint, mo_relaxed) == 21);
}

static void test_ak_swap() {
	int r = 0;
	CHECKPOINT();
	r = ak_swap(&aint, 31, mo_relaxed);
	ASSERT(r == 21);
	CHECKPOINT();
	ASSERT(ak_load(&aint, mo_relaxed) == 31);
}

static void test_ak_cas_strong() {
	int r = 0;
	CHECKPOINT();
	ASSERT(!ak_cas_strong(&aint, &r, 31, mo_relaxed, mo_relaxed));
	ASSERT(r == 21);
	CHECKPOINT();
	ASSERT(ak_cas_strong(&aint, &r, 31, mo_relaxed, mo_relaxed));
	ASSERT(r == 21);
	CHECKPOINT();
	ASSERT(ak_load(&aint, mo_relaxed) == 31);
}

static void test_ak_cas() {
	int r = 0;
	CHECKPOINT();
	ASSERT(!ak_cas(&aint, &r, 31, mo_relaxed, mo_relaxed));
	ASSERT(r == 21);
	CHECKPOINT();
	ASSERT(ak_cas(&aint, &r, 31, mo_relaxed, mo_relaxed));
	ASSERT(r == 21);
	CHECKPOINT();
	ASSERT(ak_load(&aint, mo_relaxed) == 31);
}

static void test_ak_ldadd() {
	CHECKPOINT();
	ASSERT(ak_ldadd(&aint, 10, mo_relaxed) == 21);
	CHECKPOINT();
	ASSERT(ak_load(&aint, mo_relaxed) == 31);
}

static void test_ak_ldsub() {
	CHECKPOINT();
	ASSERT(ak_ldsub(&aint, 10, mo_relaxed) == 21);
	CHECKPOINT();
	ASSERT(ak_load(&aint, mo_relaxed) == 11);
}

static void test_ak_ldor() {
	CHECKPOINT();
	ASSERT(ak_ldor(&aint, 30, mo_relaxed) == 21);
	CHECKPOINT();
	ASSERT(ak_load(&aint, mo_relaxed) == 31);
}

static void test_ak_ldand() {
	CHECKPOINT();
	ASSERT(ak_ldand(&aint, 19, mo_relaxed) == 21);
	CHECKPOINT();
	ASSERT(ak_load(&aint, mo_relaxed) == 17);
}

/*******************************************/

static void test_atomic_flag_unset_fixture(void (*test)()) {
	aflag_clear(&aflag, mo_relaxed);
	test();
}

static void test_aflag_ts() {
	ASSERT(!aflag_ts(&aflag, mo_relaxed));
	ASSERT(aflag_ts(&aflag, mo_relaxed));
}

/*******************************************/

static void test_atomic_flag_set_fixture(void (*test)()) {
	aflag_ts(&aflag, mo_relaxed);
	test();
}

static void test_aflag_clear() {
	CHECKPOINT();
	aflag_clear(&aflag, mo_relaxed);
	ASSERT(!aflag_ts(&aflag, mo_relaxed));
}

int run_atomic_h_test_suite() {
	int r;
	void (*atomic_blank_tests[])() = { test_ak_init, test_ak_fence,
					   test_ak_sigfence, test_ak_is_nb,
					   NULL };
	char *atomic_blank_test_names[] = { "ak_init", "ak_fence",
					    "ak_sigfence", "ak_is_nb", NULL };
	void (*atomic_int_tests[])() = { test_ak_store, test_ak_load,
					 test_ak_swap, test_ak_cas_strong,
					 test_ak_cas, test_ak_ldadd,
					 test_ak_ldsub, test_ak_ldor,
					 test_ak_ldand, NULL};
	
	char *atomic_int_test_names[] = { "ak_store", "ak_load", "ak_swap",
					  "ak_cas_strong", "ak_cas",
					  "ak_ldadd", "ak_ldsub", "ak_ldor",
					  "ak_ldand", NULL };

	void (*atomic_flag_unset_tests[])() = { test_aflag_ts, NULL };
	
	char *atomic_flag_unset_test_names[] = { "aflag_ts", NULL };

	void (*atomic_flag_set_tests[])() = { test_aflag_clear, NULL };
	
	char *atomic_flag_set_test_names[] = { "aflag_clear", NULL };

	r = run_test_suite(NULL, atomic_blank_test_names, atomic_blank_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_atomic_int_fixture,
			   atomic_int_test_names, atomic_int_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_atomic_flag_unset_fixture,
			   atomic_flag_unset_test_names,
			   atomic_flag_unset_tests);
	if (r != 0) {
		return r;
	}

	r = run_test_suite(test_atomic_flag_set_fixture,
			   atomic_flag_set_test_names, atomic_flag_set_tests);
	if (r != 0) {
		return r;
	}

	return r;
}
