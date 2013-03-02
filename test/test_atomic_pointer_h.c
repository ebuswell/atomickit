#include <atomickit/atomic-pointer.h>
#include "test.h"

static struct {
    char *string1;
    char *string2;
    char *string3;
} ptrtest = {"Test String 1", "Test String 2", "Test String 3"};

static volatile atomic_ptr aptr = ATOMIC_PTR_VAR_INIT(NULL);

static void test_atomic_ptr_blank_fixture(void (*test)()) {
    test();
}

static void test_atomic_ptr_init() {
    CHECKPOINT();
    atomic_ptr_init(&aptr, ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string1);
}

static void test_atomic_ptr_is_lock_free() {
    CHECKPOINT();
    (void) atomic_ptr_is_lock_free(&aptr);
}

/******************************************************/

static void test_atomic_ptr_fixture(void (*test)()) {
    atomic_ptr_init(&aptr, ptrtest.string1);
    test();
}

static void test_atomic_ptr_store_explicit() {
    CHECKPOINT();
    atomic_ptr_store_explicit(&aptr, ptrtest.string2, memory_order_relaxed);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string2);
}

static void test_atomic_ptr_store() {
    CHECKPOINT();
    atomic_ptr_store(&aptr, ptrtest.string2);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string2);
}

static void test_atomic_ptr_load_explicit() {
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string1);
}

static void test_atomic_ptr_load() {
    ASSERT(atomic_ptr_load(&aptr) == ptrtest.string1);
}

static void test_atomic_ptr_exchange_explicit() {
    CHECKPOINT();
    void *ptr = atomic_ptr_exchange_explicit(&aptr, ptrtest.string2, memory_order_relaxed);
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string2);
}

static void test_atomic_ptr_exchange() {
    CHECKPOINT();
    void *ptr = atomic_ptr_exchange(&aptr, ptrtest.string2);
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string2);
}

static void test_atomic_ptr_compare_exchange_strong_explicit() {
    void *ptr = ptrtest.string2;
    ASSERT(!atomic_ptr_compare_exchange_strong_explicit(&aptr, &ptr, ptrtest.string3, memory_order_relaxed, memory_order_relaxed));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_compare_exchange_strong_explicit(&aptr, &ptr, ptrtest.string3, memory_order_relaxed, memory_order_relaxed));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string3);
}

static void test_atomic_ptr_compare_exchange_strong() {
    void *ptr = ptrtest.string2;
    ASSERT(!atomic_ptr_compare_exchange_strong(&aptr, &ptr, ptrtest.string3));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_compare_exchange_strong(&aptr, &ptr, ptrtest.string3));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string3);
}

static void test_atomic_ptr_compare_exchange_weak_explicit() {
    void *ptr = ptrtest.string2;
    ASSERT(!atomic_ptr_compare_exchange_weak_explicit(&aptr, &ptr, ptrtest.string3, memory_order_relaxed, memory_order_relaxed));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_compare_exchange_weak_explicit(&aptr, &ptr, ptrtest.string3, memory_order_relaxed, memory_order_relaxed));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string3);
}

static void test_atomic_ptr_compare_exchange_weak() {
    void *ptr = ptrtest.string2;
    ASSERT(!atomic_ptr_compare_exchange_weak(&aptr, &ptr, ptrtest.string3));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_compare_exchange_weak(&aptr, &ptr, ptrtest.string3));
    ASSERT(ptr == ptrtest.string1);
    ASSERT(atomic_ptr_load_explicit(&aptr, memory_order_relaxed) == ptrtest.string3);
}

int run_atomic_pointer_h_test_suite() {
    int r;
    void (*atomic_ptr_blank_tests[])() = { test_atomic_ptr_init, test_atomic_ptr_is_lock_free, NULL };
    char *atomic_ptr_blank_test_names[] = { "atomic_ptr_init", "atomic_ptr_is_lock_free", NULL };

    r = run_test_suite(test_atomic_ptr_blank_fixture, atomic_ptr_blank_test_names, atomic_ptr_blank_tests);
    if(r != 0) {
	return r;
    }

    void (*atomic_ptr_tests[])() = { test_atomic_ptr_store_explicit, test_atomic_ptr_store,
				     test_atomic_ptr_load_explicit, test_atomic_ptr_load,
				     test_atomic_ptr_exchange_explicit, test_atomic_ptr_exchange,
				     test_atomic_ptr_compare_exchange_strong_explicit,
				     test_atomic_ptr_compare_exchange_strong,
				     test_atomic_ptr_compare_exchange_weak_explicit,
				     test_atomic_ptr_compare_exchange_weak, NULL };
    char *atomic_ptr_test_names[] = { "atomic_ptr_store_explicit", "atomic_ptr_store",
				      "atomic_ptr_load_explicit", "atomic_ptr_load",
				      "atomic_ptr_exchange_explicit", "atomic_ptr_exchange",
				      "atomic_ptr_compare_exchange_strong_explicit",
				      "atomic_ptr_compare_exchange_strong",
				      "atomic_ptr_compare_exchange_weak_explicit",
				      "atomic_ptr_compare_exchange_weak", NULL };
    r = run_test_suite(test_atomic_ptr_fixture, atomic_ptr_test_names, atomic_ptr_tests);
    if(r != 0) {
	return r;
    }

    return 0;
}
