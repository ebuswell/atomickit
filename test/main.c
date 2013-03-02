#include "test.h"

int run_atomic_h_test_suite();
int run_atomic_float_h_test_suite();
int run_atomic_pointer_h_test_suite();

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
    int r;

    r = run_atomic_h_test_suite();
    r |= run_atomic_float_h_test_suite();
    r |= run_atomic_pointer_h_test_suite();
    r |= print_test_results();
    if(r != 0) {
	exit(EXIT_FAILURE);
    }
    if(tests_succeeded()) {
	exit(EXIT_SUCCESS);
    } else {
	exit(EXIT_FAILURE);
    }
}
