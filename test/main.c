#include <stdio.h>
#include "test.h"

int run_atomic_h_test_suite();
int run_atomic_float_h_test_suite();
int run_atomic_pointer_h_test_suite();
int run_atomic_txn_h_test_suite();

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
    int r;

    r = run_atomic_h_test_suite();
    if(r != 0) {
	fprintf(stderr, "Failed to run tests");
	exit(EXIT_FAILURE);
    }
    r = run_atomic_float_h_test_suite();
    if(r != 0) {
	fprintf(stderr, "Failed to run tests");
	exit(EXIT_FAILURE);
    }
    r = run_atomic_pointer_h_test_suite();
    if(r != 0) {
	fprintf(stderr, "Failed to run tests");
	exit(EXIT_FAILURE);
    }
    r = run_atomic_txn_h_test_suite();
    if(r != 0) {
	fprintf(stderr, "Failed to run tests");
	exit(EXIT_FAILURE);
    }
    r = print_test_results();
    if(r != 0) {
	fprintf(stderr, "Failed to print test results");
	exit(EXIT_FAILURE);
    }
    if(tests_succeeded()) {
	exit(EXIT_SUCCESS);
    } else {
	exit(EXIT_FAILURE);
    }
}
