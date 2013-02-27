/*
 * test.c
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
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

#include "atomickit/atomic.h"
#include "atomickit/atomic-float.h"
#include "atomickit/atomic-pointer.h"
#include "atomickit/atomic-txn.h"
#include "atomickit/atomic-queue.h"

struct {
    char __attribute__((aligned(8))) string1[14];
    char __attribute__((aligned(8))) string2[14];
    char __attribute__((aligned(8))) string3[14];
} test = {"Test String 1", "Test String 2", "Test String 3"};

#define CHECKING(function)			\
    printf("Checking " #function "...")		\
    
#define OK()					\
    printf("OK\n")

#define CONDFAIL(cond)					\
    if(cond) {						\
	printf("FAIL at %s:%d\n", __FILE__, __LINE__);	\
	exit(1);					\
    }

volatile atomic_bool abool = ATOMIC_VAR_INIT(false);
volatile atomic_char achar = ATOMIC_VAR_INIT('\0');
volatile atomic_schar aschar = ATOMIC_VAR_INIT(0);
volatile atomic_uchar auchar = ATOMIC_VAR_INIT(0);
volatile atomic_short ashort = ATOMIC_VAR_INIT(0);
volatile atomic_ushort aushort = ATOMIC_VAR_INIT(0);
volatile atomic_int aint = ATOMIC_VAR_INIT(0);
volatile atomic_uint auint = ATOMIC_VAR_INIT(0);
volatile atomic_long along = ATOMIC_VAR_INIT(0);
volatile atomic_ulong aulong = ATOMIC_VAR_INIT(0);
volatile atomic_wchar_t awchar = ATOMIC_VAR_INIT('\0');
volatile atomic_intptr_t aintptr = ATOMIC_VAR_INIT(0);
volatile atomic_uintptr_t auintptr = ATOMIC_VAR_INIT(0);
volatile atomic_size_t asize = ATOMIC_VAR_INIT(0);
volatile atomic_ptrdiff_t aptrdiff = ATOMIC_VAR_INIT(0);
volatile atomic_intmax_t aintmax = ATOMIC_VAR_INIT(0);
volatile atomic_uintmax_t auintmax = ATOMIC_VAR_INIT(0);
volatile atomic_flag aflag = ATOMIC_FLAG_INIT;

volatile atomic_float afloat;
volatile atomic_double adouble;
volatile atomic_ptr aptr = ATOMIC_PTR_VAR_INIT(NULL);

bool item1_destroyed = false;
bool item2_destroyed = false;

void destroy_item1(struct atxn_item *item __attribute__((unused))) {
    item1_destroyed = true;
}

void destroy_item2(struct atxn_item *item __attribute__((unused))) {
    item2_destroyed = true;
}

atxn_t atxn;

bool node1_destroyed = false;
bool node2_destroyed = false;
bool dummy_destroyed = false;

void destroy_node1(struct aqueue_node *node __attribute__((unused))) {
    node1_destroyed = true;
}

void destroy_node2(struct aqueue_node *node __attribute__((unused))) {
    node2_destroyed = true;
}

void destroy_dummy(struct aqueue_node *node __attribute__((unused))) {
    dummy_destroyed = true;
}

aqueue_t aqueue;

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
    int r;
    float fr;
    /* double dr; */
    void *pr;

    printf("Testing single-threaded...\n");

    CHECKING(atomic_init);
    atomic_init(&aint, 21);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 21);
    OK();

    CHECKING(atomic_thread_fence);
    atomic_thread_fence(memory_order_relaxed);
    atomic_thread_fence(memory_order_consume);
    atomic_thread_fence(memory_order_acquire);
    atomic_thread_fence(memory_order_release);
    atomic_thread_fence(memory_order_acq_rel);
    atomic_thread_fence(memory_order_seq_cst);
    OK();

    CHECKING(atomic_signal_fence);
    atomic_signal_fence(memory_order_relaxed);
    atomic_signal_fence(memory_order_consume);
    atomic_signal_fence(memory_order_acquire);
    atomic_signal_fence(memory_order_release);
    atomic_signal_fence(memory_order_acq_rel);
    atomic_signal_fence(memory_order_seq_cst);
    OK();

    CHECKING(atomic_is_lock_free);
    printf("\n");
    printf("atomic_bool: ");
    if(atomic_is_lock_free(&abool)) {
	CONDFAIL(!ATOMIC_BOOL_LOCK_FREE);
	printf("true\n");
    } else {
	CONDFAIL(ATOMIC_BOOL_LOCK_FREE);
	printf("false\n");
    }
    printf("atomic_char: ");
    if(atomic_is_lock_free(&achar)) {
	CONDFAIL(!ATOMIC_CHAR_LOCK_FREE);
	printf("true\n");
    } else {
	CONDFAIL(ATOMIC_CHAR_LOCK_FREE);
	printf("false\n");
    }
    printf("atomic_schar: ");
    printf(atomic_is_lock_free(&aschar) ? "true\n" : "false\n");
    printf("atomic_uchar: ");
    printf(atomic_is_lock_free(&auchar) ? "true\n" : "false\n");
    printf("atomic_short: ");
    if(atomic_is_lock_free(&ashort)) {
	CONDFAIL(!ATOMIC_SHORT_LOCK_FREE);
	printf("true\n");
    } else {
	CONDFAIL(ATOMIC_SHORT_LOCK_FREE);
	printf("false\n");
    }
    printf("atomic_ushort: ");
    printf(atomic_is_lock_free(&aushort) ? "true\n" : "false\n");
    printf("atomic_int: ");
    if(atomic_is_lock_free(&aint)) {
	CONDFAIL(!ATOMIC_INT_LOCK_FREE);
	printf("true\n");
    } else {
	CONDFAIL(ATOMIC_INT_LOCK_FREE);
	printf("false\n");
    }
    printf("atomic_uint: ");
    printf(atomic_is_lock_free(&auint) ? "true\n" : "false\n");
    printf("atomic_long: ");
    if(atomic_is_lock_free(&along)) {
	CONDFAIL(!ATOMIC_LONG_LOCK_FREE);
	printf("true\n");
    } else {
	CONDFAIL(ATOMIC_LONG_LOCK_FREE);
	printf("false\n");
    }
    printf("atomic_ulong: ");
    printf(atomic_is_lock_free(&aulong) ? "true\n" : "false\n");
    printf("atomic_wchar_t: ");
    if(atomic_is_lock_free(&awchar)) {
	CONDFAIL(!ATOMIC_WCHAR_T_LOCK_FREE);
	printf("true\n");
    } else {
	CONDFAIL(ATOMIC_WCHAR_T_LOCK_FREE);
	printf("false\n");
    }
    printf("atomic_intptr_t: ");
    if(atomic_is_lock_free(&aintptr)) {
	CONDFAIL(!ATOMIC_POINTER_LOCK_FREE);
	printf("true\n");
    } else {
	CONDFAIL(ATOMIC_POINTER_LOCK_FREE);
	printf("false\n");
    }
    printf("atomic_uintptr_t: ");
    printf(atomic_is_lock_free(&auintptr) ? "true\n" : "false\n");
    printf("atomic_size_t: ");
    printf(atomic_is_lock_free(&asize) ? "true\n" : "false\n");
    printf("atomic_ptrdiff_t: ");
    printf(atomic_is_lock_free(&aptrdiff) ? "true\n" : "false\n");
    printf("atomic_intmax_t: ");
    printf(atomic_is_lock_free(&aintmax) ? "true\n" : "false\n");
    printf("atomic_uintmax_t: ");
    printf(atomic_is_lock_free(&auintmax) ? "true\n" : "false\n");
    printf("atomic_flag: ");
    printf(atomic_is_lock_free(&aflag) ? "true\n" : "false\n");
    OK();

    CHECKING(atomic_store_explicit);
    atomic_store_explicit(&aint, 31, memory_order_relaxed);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 31);
    OK();

    CHECKING(atomic_store);
    atomic_store(&aint, 41);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 41);
    OK();

    CHECKING(atomic_load_explicit);
    atomic_store(&aint, 51);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 51);
    OK();

    CHECKING(atomic_load);
    atomic_store(&aint, 61);
    CONDFAIL(atomic_load(&aint) != 61);
    OK();

    CHECKING(atomic_exchange_explicit);
    r = atomic_exchange_explicit(&aint, 71, memory_order_relaxed);
    CONDFAIL(r != 61);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 71);
    OK();

    CHECKING(atomic_exchange);
    r = atomic_exchange(&aint, 81);
    CONDFAIL(r != 71);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 81);
    OK();

    CHECKING(atomic_compare_exchange_strong_explicit);
    CONDFAIL(atomic_compare_exchange_strong_explicit(&aint, &r, 91, memory_order_relaxed, memory_order_relaxed));
    CONDFAIL(r != 81);
    CONDFAIL(!atomic_compare_exchange_strong_explicit(&aint, &r, 91, memory_order_relaxed, memory_order_relaxed));
    CONDFAIL(r != 81);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 91);
    OK();

    CHECKING(atomic_compare_exchange_strong);
    CONDFAIL(atomic_compare_exchange_strong(&aint, &r, 101));
    CONDFAIL(r != 91);
    CONDFAIL(!atomic_compare_exchange_strong(&aint, &r, 101));
    CONDFAIL(r != 91);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 101);
    OK();

    CHECKING(atomic_compare_exchange_weak_explicit);
    CONDFAIL(atomic_compare_exchange_weak_explicit(&aint, &r, 111, memory_order_relaxed, memory_order_relaxed));
    CONDFAIL(r != 101);
    CONDFAIL(!atomic_compare_exchange_weak_explicit(&aint, &r, 111, memory_order_relaxed, memory_order_relaxed));
    CONDFAIL(r != 101);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 111);
    OK();

    CHECKING(atomic_compare_exchange_weak);
    CONDFAIL(atomic_compare_exchange_weak(&aint, &r, 121));
    CONDFAIL(r != 111);
    CONDFAIL(!atomic_compare_exchange_weak(&aint, &r, 121));
    CONDFAIL(r != 111);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 121);
    OK();

    CHECKING(atomic_fetch_add_explicit);
    CONDFAIL(atomic_fetch_add_explicit(&aint, 10, memory_order_relaxed) != 121);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 131);
    OK();

    CHECKING(atomic_fetch_add);
    CONDFAIL(atomic_fetch_add(&aint, 10) != 131);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 141);
    OK();

    CHECKING(atomic_fetch_sub_explicit);
    CONDFAIL(atomic_fetch_sub_explicit(&aint, 10, memory_order_relaxed) != 141);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 131);
    OK();

    CHECKING(atomic_fetch_sub);
    CONDFAIL(atomic_fetch_sub(&aint, 10) != 131);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 121);
    OK();

    CHECKING(atomic_fetch_or_explicit);
    atomic_store_explicit(&aint, 0x2, memory_order_relaxed);
    CONDFAIL(atomic_fetch_or_explicit(&aint, 0x1, memory_order_relaxed) != 0x2);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 0x3);
    OK();

    CHECKING(atomic_fetch_or);
    atomic_store_explicit(&aint, 0x2, memory_order_relaxed);
    CONDFAIL(atomic_fetch_or(&aint, 0x1) != 0x2);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 0x3);
    OK();

    CHECKING(atomic_fetch_xor_explicit);
    atomic_store_explicit(&aint, 0x2, memory_order_relaxed);
    CONDFAIL(atomic_fetch_xor_explicit(&aint, 0x3, memory_order_relaxed) != 0x2);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 0x1);
    OK();

    CHECKING(atomic_fetch_xor);
    atomic_store_explicit(&aint, 0x2, memory_order_relaxed);
    CONDFAIL(atomic_fetch_xor(&aint, 0x3) != 0x2);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 0x1);
    OK();

    CHECKING(atomic_fetch_and_explicit);
    atomic_store_explicit(&aint, 0x2, memory_order_relaxed);
    CONDFAIL(atomic_fetch_and_explicit(&aint, 0x3, memory_order_relaxed) != 0x2);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 0x2);
    OK();

    CHECKING(atomic_fetch_and);
    atomic_store_explicit(&aint, 0x2, memory_order_relaxed);
    CONDFAIL(atomic_fetch_and(&aint, 0x3) != 0x2);
    CONDFAIL(atomic_load_explicit(&aint, memory_order_relaxed) != 0x2);
    OK();

    CHECKING(atomic_flag_test_and_set_explicit);
    CONDFAIL(atomic_flag_test_and_set_explicit(&aflag, memory_order_relaxed));
    CONDFAIL(!atomic_flag_test_and_set_explicit(&aflag, memory_order_relaxed));
    OK();

    CHECKING(atomic_flag_clear_explicit);
    atomic_flag_clear_explicit(&aflag, memory_order_relaxed);
    CONDFAIL(atomic_flag_test_and_set_explicit(&aflag, memory_order_relaxed));
    atomic_flag_clear_explicit(&aflag, memory_order_relaxed);
    OK();

    CHECKING(atomic_flag_test_and_set);
    CONDFAIL(atomic_flag_test_and_set(&aflag));
    CONDFAIL(!atomic_flag_test_and_set(&aflag));
    OK();

    CHECKING(atomic_flag_clear);
    atomic_flag_clear(&aflag);
    CONDFAIL(atomic_flag_test_and_set_explicit(&aflag, memory_order_relaxed));
    OK();


    CHECKING(atomic_float_init);
    atomic_float_init(&afloat, 2.1f);
    CONDFAIL(atomic_float_load_explicit(&afloat, memory_order_relaxed) != 2.1f);
    OK();

    CHECKING(atomic_float_is_lock_free);
    printf("\n");
    printf("atomic_float: ");
    printf(atomic_float_is_lock_free(&afloat) ? "true\n" : "false\n");
    OK();

    CHECKING(atomic_float_store_explicit);
    atomic_float_store_explicit(&afloat, 3.1f, memory_order_relaxed);
    CONDFAIL(atomic_float_load_explicit(&afloat, memory_order_relaxed) != 3.1f);
    OK();

    CHECKING(atomic_float_store);
    atomic_float_store(&afloat, 4.1f);
    CONDFAIL(atomic_float_load_explicit(&afloat, memory_order_relaxed) != 4.1f);
    OK();

    CHECKING(atomic_float_load_explicit);
    atomic_float_store(&afloat, 5.1f);
    CONDFAIL(atomic_float_load_explicit(&afloat, memory_order_relaxed) != 5.1f);
    OK();

    CHECKING(atomic_float_load);
    atomic_float_store(&afloat, 6.1f);
    CONDFAIL(atomic_float_load(&afloat) != 6.1f);
    OK();

    CHECKING(atomic_float_exchange_explicit);
    fr = atomic_float_exchange_explicit(&afloat, 7.1f, memory_order_relaxed);
    CONDFAIL(fr != 6.1f);
    CONDFAIL(atomic_float_load_explicit(&afloat, memory_order_relaxed) != 7.1f);
    OK();

    CHECKING(atomic_float_exchange);
    fr = atomic_float_exchange(&afloat, 8.1f);
    CONDFAIL(fr != 7.1f);
    CONDFAIL(atomic_float_load_explicit(&afloat, memory_order_relaxed) != 8.1f);
    OK();

    CHECKING(atomic_float_compare_exchange_strong_explicit);
    CONDFAIL(atomic_float_compare_exchange_strong_explicit(&afloat, &fr, 9.1f, memory_order_relaxed, memory_order_relaxed));
    CONDFAIL(fr != 8.1f);
    CONDFAIL(!atomic_float_compare_exchange_strong_explicit(&afloat, &fr, 9.1f, memory_order_relaxed, memory_order_relaxed));
    CONDFAIL(fr != 8.1f);
    CONDFAIL(atomic_float_load_explicit(&afloat, memory_order_relaxed) != 9.1f);
    OK();

    CHECKING(atomic_float_compare_exchange_strong);
    CONDFAIL(atomic_float_compare_exchange_strong(&afloat, &fr, 10.1f));
    CONDFAIL(fr != 9.1f);
    CONDFAIL(!atomic_float_compare_exchange_strong(&afloat, &fr, 10.1f));
    CONDFAIL(fr != 9.1f);
    CONDFAIL(atomic_float_load_explicit(&afloat, memory_order_relaxed) != 10.1f);
    OK();

    CHECKING(atomic_float_compare_exchange_weak_explicit);
    CONDFAIL(atomic_float_compare_exchange_weak_explicit(&afloat, &fr, 11.1f, memory_order_relaxed, memory_order_relaxed));
    CONDFAIL(fr != 10.1f);
    CONDFAIL(!atomic_float_compare_exchange_weak_explicit(&afloat, &fr, 11.1f, memory_order_relaxed, memory_order_relaxed));
    CONDFAIL(fr != 10.1f);
    CONDFAIL(atomic_float_load_explicit(&afloat, memory_order_relaxed) != 11.1f);
    OK();

    CHECKING(atomic_float_compare_exchange_weak);
    CONDFAIL(atomic_float_compare_exchange_weak(&afloat, &fr, 12.1f));
    CONDFAIL(fr != 11.1f);
    CONDFAIL(!atomic_float_compare_exchange_weak(&afloat, &fr, 12.1f));
    CONDFAIL(fr != 11.1f);
    CONDFAIL(atomic_float_load_explicit(&afloat, memory_order_relaxed) != 12.1f);
    OK();

    /* CHECKING(atomic_double_init); */
    /* atomic_double_init(&adouble, 2.1f); */
    /* CONDFAIL(atomic_double_load_explicit(&adouble, memory_order_relaxed) != 2.1f); */
    /* OK(); */

    /* CHECKING(atomic_double_is_lock_free); */
    /* printf("\n"); */
    /* printf("atomic_double: "); */
    /* printf(atomic_double_is_lock_free(&adouble) ? "true\n" : "false\n"); */
    /* OK(); */

    /* CHECKING(atomic_double_store_explicit); */
    /* atomic_double_store_explicit(&adouble, 3.1f, memory_order_relaxed); */
    /* CONDFAIL(atomic_double_load_explicit(&adouble, memory_order_relaxed) != 3.1f); */
    /* OK(); */

    /* CHECKING(atomic_double_store); */
    /* atomic_double_store(&adouble, 4.1f); */
    /* CONDFAIL(atomic_double_load_explicit(&adouble, memory_order_relaxed) != 4.1f); */
    /* OK(); */

    /* CHECKING(atomic_double_load_explicit); */
    /* atomic_double_store(&adouble, 5.1f); */
    /* CONDFAIL(atomic_double_load_explicit(&adouble, memory_order_relaxed) != 5.1f); */
    /* OK(); */

    /* CHECKING(atomic_double_load); */
    /* atomic_double_store(&adouble, 6.1f); */
    /* CONDFAIL(atomic_double_load(&adouble) != 6.1f); */
    /* OK(); */

    /* CHECKING(atomic_double_exchange_explicit); */
    /* dr = atomic_double_exchange_explicit(&adouble, 7.1f, memory_order_relaxed); */
    /* CONDFAIL(dr != 6.1f); */
    /* CONDFAIL(atomic_double_load_explicit(&adouble, memory_order_relaxed) != 7.1f); */
    /* OK(); */

    /* CHECKING(atomic_double_exchange); */
    /* dr = atomic_double_exchange(&adouble, 8.1f); */
    /* CONDFAIL(dr != 7.1f); */
    /* CONDFAIL(atomic_double_load_explicit(&adouble, memory_order_relaxed) != 8.1f); */
    /* OK(); */

    /* CHECKING(atomic_double_compare_exchange_strong_explicit); */
    /* CONDFAIL(atomic_double_compare_exchange_strong_explicit(&adouble, &dr, 9.1f, memory_order_relaxed, memory_order_relaxed)); */
    /* CONDFAIL(dr != 8.1f); */
    /* CONDFAIL(!atomic_double_compare_exchange_strong_explicit(&adouble, &dr, 9.1f, memory_order_relaxed, memory_order_relaxed)); */
    /* CONDFAIL(dr != 8.1f); */
    /* CONDFAIL(atomic_double_load_explicit(&adouble, memory_order_relaxed) != 9.1f); */
    /* OK(); */

    /* CHECKING(atomic_double_compare_exchange_strong); */
    /* CONDFAIL(atomic_double_compare_exchange_strong(&adouble, &dr, 10.1f)); */
    /* CONDFAIL(dr != 9.1f); */
    /* CONDFAIL(!atomic_double_compare_exchange_strong(&adouble, &dr, 10.1f)); */
    /* CONDFAIL(dr != 9.1f); */
    /* CONDFAIL(atomic_double_load_explicit(&adouble, memory_order_relaxed) != 10.1f); */
    /* OK(); */

    /* CHECKING(atomic_double_compare_exchange_weak_explicit); */
    /* CONDFAIL(atomic_double_compare_exchange_weak_explicit(&adouble, &dr, 11.1f, memory_order_relaxed, memory_order_relaxed)); */
    /* CONDFAIL(dr != 10.1f); */
    /* CONDFAIL(!atomic_double_compare_exchange_weak_explicit(&adouble, &dr, 11.1f, memory_order_relaxed, memory_order_relaxed)); */
    /* CONDFAIL(dr != 10.1f); */
    /* CONDFAIL(atomic_double_load_explicit(&adouble, memory_order_relaxed) != 11.1f); */
    /* OK(); */

    /* CHECKING(atomic_double_compare_exchange_weak); */
    /* CONDFAIL(atomic_double_compare_exchange_weak(&adouble, &dr, 12.1f)); */
    /* CONDFAIL(dr != 11.1f); */
    /* CONDFAIL(!atomic_double_compare_exchange_weak(&adouble, &dr, 12.1f)); */
    /* CONDFAIL(dr != 11.1f); */
    /* CONDFAIL(atomic_double_load_explicit(&adouble, memory_order_relaxed) != 12.1f); */
    /* OK(); */

    CHECKING(atomic_ptr_init);
    atomic_ptr_init(&aptr, test.string1);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string1);
    OK();

    CHECKING(atomic_ptr_is_lock_free);
    printf("\n");
    printf("atomic_ptr: ");
    printf(atomic_ptr_is_lock_free(&aptr) ? "true\n" : "false\n");
    OK();

    CHECKING(atomic_ptr_store_explicit);
    atomic_ptr_store_explicit(&aptr, test.string2, memory_order_relaxed);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string2);
    OK();

    CHECKING(atomic_ptr_store);
    atomic_ptr_store(&aptr, test.string3);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string3);
    OK();

    CHECKING(atomic_ptr_load_explicit);
    atomic_ptr_store(&aptr, test.string1);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string1);
    OK();

    CHECKING(atomic_ptr_load);
    atomic_ptr_store(&aptr, test.string2);
    CONDFAIL(atomic_ptr_load(&aptr) != test.string2);
    OK();

    CHECKING(atomic_ptr_exchange_explicit);
    pr = atomic_ptr_exchange_explicit(&aptr, test.string3, memory_order_relaxed);
    CONDFAIL(pr != test.string2);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string3);
    OK();

    CHECKING(atomic_ptr_exchange);
    pr = atomic_ptr_exchange(&aptr, test.string1);
    CONDFAIL(pr != test.string3);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string1);
    OK();

    CHECKING(atomic_ptr_compare_exchange_strong_explicit);
    CONDFAIL(atomic_ptr_compare_exchange_strong_explicit(&aptr, &pr, test.string2, memory_order_relaxed, memory_order_relaxed));
    CONDFAIL(pr != test.string1);
    CONDFAIL(!atomic_ptr_compare_exchange_strong_explicit(&aptr, &pr, test.string2, memory_order_relaxed, memory_order_relaxed));
    CONDFAIL(pr != test.string1);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string2);
    OK();

    CHECKING(atomic_ptr_compare_exchange_strong);
    CONDFAIL(atomic_ptr_compare_exchange_strong(&aptr, &pr, test.string3));
    CONDFAIL(pr != test.string2);
    CONDFAIL(!atomic_ptr_compare_exchange_strong(&aptr, &pr, test.string3));
    CONDFAIL(pr != test.string2);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string3);
    OK();

    CHECKING(atomic_ptr_compare_exchange_weak_explicit);
    CONDFAIL(atomic_ptr_compare_exchange_weak_explicit(&aptr, &pr, test.string1, memory_order_relaxed, memory_order_relaxed));
    CONDFAIL(pr != test.string3);
    CONDFAIL(!atomic_ptr_compare_exchange_weak_explicit(&aptr, &pr, test.string1, memory_order_relaxed, memory_order_relaxed));
    CONDFAIL(pr != test.string3);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string1);
    OK();

    CHECKING(atomic_ptr_compare_exchange_weak);
    CONDFAIL(atomic_ptr_compare_exchange_weak(&aptr, &pr, test.string2));
    CONDFAIL(pr != test.string1);
    CONDFAIL(!atomic_ptr_compare_exchange_weak(&aptr, &pr, test.string2));
    CONDFAIL(pr != test.string1);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string2);
    OK();

    CHECKING(atomic_ptr_fetch_add_explicit);
    CONDFAIL(atomic_ptr_fetch_add_explicit(&aptr, 4, memory_order_relaxed) != test.string2);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != &test.string2[4]);
    OK();

    CHECKING(atomic_ptr_fetch_add);
    CONDFAIL(atomic_ptr_fetch_add(&aptr, 4) != &test.string2[4]);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != &test.string2[8]);
    OK();

    CHECKING(atomic_ptr_fetch_sub_explicit);
    CONDFAIL(atomic_ptr_fetch_sub_explicit(&aptr, 4, memory_order_relaxed) != &test.string2[8]);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != &test.string2[4]);
    OK();

    CHECKING(atomic_ptr_fetch_sub);
    CONDFAIL(atomic_ptr_fetch_sub(&aptr, 4) != &test.string2[4]);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string2);
    OK();

    CHECKING(atomic_ptr_fetch_or_explicit);
    CONDFAIL(atomic_ptr_fetch_or_explicit(&aptr, 0x1, memory_order_relaxed) != test.string2);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != &test.string2[1]);
    OK();

    CHECKING(atomic_ptr_fetch_or);
    atomic_ptr_store_explicit(&aptr, test.string2, memory_order_relaxed);
    CONDFAIL(atomic_ptr_fetch_or(&aptr, 0x1) != test.string2);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != &test.string2[1]);
    OK();

    CHECKING(atomic_ptr_fetch_xor_explicit);
    atomic_ptr_store_explicit(&aptr, &test.string2[2], memory_order_relaxed);
    CONDFAIL(atomic_ptr_fetch_xor_explicit(&aptr, 0x3, memory_order_relaxed) != &test.string2[2]);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != &test.string2[1]);
    OK();

    CHECKING(atomic_ptr_fetch_xor);
    atomic_ptr_store_explicit(&aptr, &test.string2[2], memory_order_relaxed);
    CONDFAIL(atomic_ptr_fetch_xor(&aptr, 0x3) != &test.string2[2]);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != &test.string2[1]);
    OK();

    CHECKING(atomic_ptr_fetch_and_explicit);
    atomic_ptr_store_explicit(&aptr, &test.string2[2], memory_order_relaxed);
    CONDFAIL(atomic_ptr_fetch_and_explicit(&aptr, ~((uintptr_t) 7), memory_order_relaxed) != &test.string2[2]);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string2);
    OK();

    CHECKING(atomic_ptr_fetch_and);
    atomic_ptr_store_explicit(&aptr, &test.string2[2], memory_order_relaxed);
    CONDFAIL(atomic_ptr_fetch_and(&aptr, ~((uintptr_t) 7)) != &test.string2[2]);
    CONDFAIL(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) != test.string2);
    OK();

    printf("Preparing atxn tests...");
    struct atxn_item *item1 = alloca(sizeof(struct atxn_item) + 14 - 1);
    strcpy((char *) item1->data, test.string1);
    struct atxn_item *item2 = alloca(sizeof(struct atxn_item) + 14 - 1);
    strcpy((char *) item2->data, test.string2);
    void *item1_ptr;
    void *item2_ptr;
    void *ptr1;
    void *ptr2;
    OK();

    CHECKING(atxn_item_init);
    item1_ptr = atxn_item_init(item1, destroy_item1);
    CONDFAIL(item1->destroy != destroy_item1);
    CONDFAIL(&item1->data != item1_ptr);
    item2_ptr = atxn_item_init(item2, destroy_item2);
    CONDFAIL(item2->destroy != destroy_item2);
    CONDFAIL(&item2->data != item2_ptr);
    CONDFAIL(item1_destroyed);
    CONDFAIL(item2_destroyed);
    OK();

    CHECKING(atxn_init);
    atxn_init(&atxn, item1_ptr);
    CONDFAIL(item1_destroyed);
    CONDFAIL(item2_destroyed);
    ptr1 = atxn_acquire(&atxn);
    CONDFAIL(ptr1 != item1_ptr);
    CONDFAIL(strcmp(ptr1, test.string1) != 0);
    atxn_release(&atxn, ptr1);
    OK();

    CHECKING(atxn_destroy);
    atxn_destroy(&atxn);
    CONDFAIL(item1_destroyed);
    atxn_init(&atxn, item1_ptr);
    atxn_item_release(item1_ptr);
    atxn_destroy(&atxn);
    CONDFAIL(!item1_destroyed);
    item1_destroyed = false;
    item1_ptr = atxn_item_init(item1, destroy_item1);
    atxn_init(&atxn, item1_ptr);
    OK();

    CHECKING(atxn_acquire);
    ptr1 = atxn_acquire(&atxn);
    ptr2 = atxn_acquire(&atxn);
    CONDFAIL(item1_destroyed);
    CONDFAIL(ptr1 != item1_ptr);
    CONDFAIL(ptr2 != item1_ptr);
    CONDFAIL(strcmp(ptr1, ptr2) != 0);
    CONDFAIL(strcmp(ptr1, test.string1) != 0);
    atxn_release(&atxn, ptr1);
    atxn_release(&atxn, ptr2);
    CONDFAIL(item1_destroyed);
    OK();

    CHECKING(atxn_item_release);
    atxn_item_release(item1_ptr);
    CONDFAIL(item1_destroyed);
    item1_ptr = atxn_acquire(&atxn);
    atxn_destroy(&atxn);
    atxn_item_release(item1_ptr);
    CONDFAIL(!item1_destroyed);
    item1_destroyed = false;
    item1_ptr = atxn_item_init(item1, destroy_item1);
    atxn_init(&atxn, item1_ptr);
    OK();

    CHECKING(atxn_commit);
    /* succeed */
    CONDFAIL(!atxn_commit(&atxn, item1_ptr, item2_ptr));
    CONDFAIL(item1_destroyed);
    CONDFAIL(item2_destroyed);
    ptr1 = atxn_acquire(&atxn);
    CONDFAIL(ptr1 != item2_ptr);
    CONDFAIL(strcmp(ptr1, test.string2) != 0);
    atxn_release(&atxn, ptr1);
    /* fail */
    CONDFAIL(atxn_commit(&atxn, item1_ptr, item1_ptr));
    CONDFAIL(item1_destroyed);
    CONDFAIL(item2_destroyed);
    ptr1 = atxn_acquire(&atxn);
    CONDFAIL(ptr1 != item2_ptr);
    CONDFAIL(strcmp(ptr1, test.string2) != 0);
    atxn_release(&atxn, ptr1);
    atxn_commit(&atxn, item2_ptr, item1_ptr);
    OK();

    CHECKING(atxn_check);
    CONDFAIL(!atxn_check(&atxn, item1_ptr));
    atxn_commit(&atxn, item1_ptr, item2_ptr);
    CONDFAIL(atxn_check(&atxn, item1_ptr));
    atxn_commit(&atxn, item2_ptr, item1_ptr);
    OK();

    CHECKING(atxn_release);
    atxn_item_release(item1_ptr);
    /* current */
    ptr1 = atxn_acquire(&atxn);
    ptr2 = atxn_acquire(&atxn);
    atxn_release(&atxn, ptr1);
    CONDFAIL(item1_destroyed);
    CONDFAIL(item2_destroyed);
    atxn_release(&atxn, ptr2);
    CONDFAIL(item1_destroyed);
    CONDFAIL(item2_destroyed);
    /* not current */
    ptr1 = atxn_acquire(&atxn);
    ptr2 = atxn_acquire(&atxn);
    atxn_commit(&atxn, item1_ptr, item2_ptr);
    atxn_release(&atxn, ptr1);
    CONDFAIL(item1_destroyed);
    CONDFAIL(item2_destroyed);
    atxn_release(&atxn, ptr2);
    CONDFAIL(!item1_destroyed);
    CONDFAIL(item2_destroyed);
    item1_destroyed = false;
    item1_ptr = atxn_item_init(item1, destroy_item1);
    atxn_commit(&atxn, item2_ptr, item1_ptr);
    OK();

    printf("Cleaning up atxn tests...");
    atxn_item_release(item1_ptr);
    atxn_item_release(item2_ptr);
    atxn_destroy(&atxn);
    OK();

    printf("Preparing aqueue tests...");
    struct aqueue_node *node1 = alloca(sizeof(struct aqueue_node) + 14 - 1);
    strcpy((char *) node1->nodeptr.data, test.string1);
    struct aqueue_node *node2 = alloca(sizeof(struct aqueue_node) + 14 - 1);
    strcpy((char *) node2->nodeptr.data, test.string2);
    struct aqueue_node *dummy = alloca(sizeof(struct aqueue_node) - 1);
    void *node1_ptr;
    void *node2_ptr;
    void *dummy_ptr;
    OK();

    CHECKING(aqueue_node_init);
    dummy_ptr = 
    OK();

    exit(0);
}
