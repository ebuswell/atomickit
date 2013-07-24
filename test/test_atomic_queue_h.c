/*
 * test_atomic_queue_h.c
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
#include "config.h"

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
void *alloca(size_t);
#endif

#include <atomickit/atomic-queue.h>
#include "test.h"

static struct {
    char __attribute__((aligned(8))) string1[14];
    char __attribute__((aligned(8))) string2[14];
} ptrtest = { "Test String 1", "Test String 2" };

static bool sentinel_destroyed = false;
static bool node1_destroyed = false;
static bool node2_destroyed = false;

static void destroy_node1(struct aqueue_node *node __attribute__((unused))) {
    CHECKPOINT();
    node1_destroyed = true;
}

static void destroy_node2(struct aqueue_node *node __attribute__((unused))) {
    CHECKPOINT();
    node2_destroyed = true;
}

static void destroy_sentinel(struct aqueue_node *node __attribute__((unused))) {
    CHECKPOINT();
    sentinel_destroyed = true;
}

static aqueue_t aqueue;

static struct aqueue_node *node1;
static struct aqueue_node *node2;
static struct aqueue_node *sentinel;

static void *node1_ptr;
static void *node2_ptr;
static void *sentinel_ptr;

/*************************/
static void test_aqueue_uninit_fixture(void (*test)()) {
    node1 = alloca(AQUEUE_NODE_OVERHEAD + 14);
    node2 = alloca(sizeof(struct aqueue_node) + 14 - 1);
    sentinel = alloca(sizeof(struct aqueue_node) - 1);
    strcpy((char *) node1->data, ptrtest.string1);
    strcpy((char *) node2->data, ptrtest.string2);
    test();
}

static void test_aqueue_node_init() {
    node1_ptr = aqueue_node_init(node1, destroy_node1);
    ASSERT(node1->header.destroy == destroy_node1);
    ASSERT(&node1->data == node1_ptr);
    ASSERT(!node1_destroyed);
}

/****************************/
static void test_aqueue_init_node_fixture(void (*test)()) {
    node1 = alloca(sizeof(struct aqueue_node) + 14 - 1);
    node2 = alloca(sizeof(struct aqueue_node) + 14 - 1);
    sentinel = alloca(sizeof(struct aqueue_node) - 1);
    strcpy((char *) node1->data, ptrtest.string1);
    strcpy((char *) node2->data, ptrtest.string2);
    
    node1_ptr = aqueue_node_init(node1, destroy_node1);
    node2_ptr = aqueue_node_init(node2, destroy_node2);
    sentinel_ptr = aqueue_node_init(sentinel, destroy_sentinel);
    test();
}

static void test_aqueue_init() {
    CHECKPOINT();
    aqueue_init(&aqueue, sentinel_ptr);
    ASSERT(!sentinel_destroyed);
    void *ptr1 = aqueue_deq(&aqueue);
    ASSERT(ptr1 == NULL);
    ASSERT(!sentinel_destroyed);
}

static void test_aqueue_node_release_unenq() {
    CHECKPOINT();
    aqueue_node_release(node1_ptr);
    ASSERT(node1_destroyed);
}

/****************************/

static void test_aqueue_init_fixture(void (*test)()) {
    node1 = alloca(sizeof(struct aqueue_node) + 14 - 1);
    node2 = alloca(sizeof(struct aqueue_node) + 14 - 1);
    sentinel = alloca(sizeof(struct aqueue_node) - 1);
    strcpy((char *) node1->data, ptrtest.string1);
    strcpy((char *) node2->data, ptrtest.string2);
    node1_ptr = aqueue_node_init(node1, destroy_node1);
    node2_ptr = aqueue_node_init(node2, destroy_node2);
    sentinel_ptr = aqueue_node_init(sentinel, destroy_sentinel);
    aqueue_init(&aqueue, sentinel_ptr);
    aqueue_node_release(sentinel_ptr);
    test();
}


static void test_aqueue_destroy_empty() {
    CHECKPOINT();
    aqueue_destroy(&aqueue);
    ASSERT(sentinel_destroyed);
}

static void test_aqueue_enq() {
    CHECKPOINT();
    aqueue_enq(&aqueue, node1_ptr);
    CHECKPOINT();
    aqueue_enq(&aqueue, node2_ptr);
    CHECKPOINT();
    aqueue_node_release(node1_ptr);
    CHECKPOINT();
    aqueue_node_release(node2_ptr);
    ASSERT(!node1_destroyed);
    ASSERT(!node2_destroyed);
    node1_ptr = aqueue_deq(&aqueue);
    CHECKPOINT();
    node2_ptr = aqueue_deq(&aqueue);
    CHECKPOINT();
    ASSERT(strcmp(node1_ptr, ptrtest.string1) == 0);
    ASSERT(strcmp(node2_ptr, ptrtest.string2) == 0);
}

/****************************/

static void test_aqueue_full_fixture(void (*test)()) {
    node1 = alloca(sizeof(struct aqueue_node) + 14 - 1);
    node2 = alloca(sizeof(struct aqueue_node) + 14 - 1);
    sentinel = alloca(sizeof(struct aqueue_node) - 1);
    strcpy((char *) node1->data, ptrtest.string1);
    strcpy((char *) node2->data, ptrtest.string2);
    node1_ptr = aqueue_node_init(node1, destroy_node1);
    node2_ptr = aqueue_node_init(node2, destroy_node2);
    sentinel_ptr = aqueue_node_init(sentinel, destroy_sentinel);
    aqueue_init(&aqueue, sentinel_ptr);
    aqueue_node_release(sentinel_ptr);
    aqueue_enq(&aqueue, node1_ptr);
    aqueue_enq(&aqueue, node2_ptr);
    test();
}

static void test_aqueue_node_release_enq() {
    aqueue_node_release(node1_ptr);
    ASSERT(!node1_destroyed);
    aqueue_node_release(node2_ptr);
    ASSERT(!node2_destroyed);
    node1_ptr = aqueue_deq(&aqueue);
    CHECKPOINT();
    node2_ptr = aqueue_deq(&aqueue);
    CHECKPOINT();
    aqueue_node_release(node1_ptr);
    CHECKPOINT();
    aqueue_node_release(node2_ptr);
    ASSERT(node1_destroyed);
    ASSERT(!node2_destroyed);
    /* node2 has an implicit reference until the next dequeue */
    /* ASSERT(node2_destroyed); */
}

static void test_aqueue_destroy_full() {
    aqueue_node_release(node1_ptr);
    CHECKPOINT();
    aqueue_destroy(&aqueue);
    ASSERT(sentinel_destroyed);
    ASSERT(node1_destroyed);
    ASSERT(!node2_destroyed);
}

static void test_aqueue_deq() {
    aqueue_node_release(node1_ptr);
    CHECKPOINT();
    aqueue_node_release(node2_ptr);
    CHECKPOINT();
    node1_ptr = aqueue_deq(&aqueue);
    ASSERT(!node1_destroyed);
    ASSERT(strcmp(node1_ptr, ptrtest.string1) == 0);
    ASSERT(sentinel_destroyed);
    node2_ptr = aqueue_deq(&aqueue);
    ASSERT(strcmp(node2_ptr, ptrtest.string2) == 0);
    ASSERT(!node2_destroyed);
    void *nope = aqueue_deq(&aqueue);
    ASSERT(nope == NULL);
}

int run_atomic_queue_h_test_suite() {
    int r;
    void (*aqueue_uninit_tests[])() = { test_aqueue_node_init, NULL };
    char *aqueue_uninit_test_names[] = { "aqueue_node_init", NULL };
    r = run_test_suite(test_aqueue_uninit_fixture, aqueue_uninit_test_names, aqueue_uninit_tests);
    if(r != 0) {
	return r;
    }

    void (*aqueue_init_node_tests[])() = { test_aqueue_init, test_aqueue_node_release_unenq, NULL };
    char *aqueue_init_node_test_names[] = { "aqueue_init", "aqueue_node_release_unenq", NULL };
    r = run_test_suite(test_aqueue_init_node_fixture, aqueue_init_node_test_names, aqueue_init_node_tests);
    if(r != 0) {
	return r;
    }

    void (*aqueue_init_tests[])() = { test_aqueue_destroy_empty, test_aqueue_enq,
				      NULL };
    char *aqueue_init_test_names[] = { "aqueue_destroy_empty", "aqueue_enq",
				       NULL };
    r = run_test_suite(test_aqueue_init_fixture, aqueue_init_test_names, aqueue_init_tests);
    if(r != 0) {
	return r;
    }

    void (*aqueue_full_tests[])() = { test_aqueue_destroy_full,
				      test_aqueue_node_release_enq,
				      test_aqueue_deq, NULL };
    char *aqueue_full_test_names[] = { "aqueue_destroy_full",
				       "aqueue_node_release_enq",
				       "aqueue_deq", NULL };
    r = run_test_suite(test_aqueue_full_fixture, aqueue_full_test_names, aqueue_full_tests);
    if(r != 0) {
	return r;
    }

    return 0;
}
