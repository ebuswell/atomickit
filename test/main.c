#include "test.h"

int run_atomic_h_test_suite();

int main(int argc, char **argv) {
    int r;

    r = run_atomic_h_test_suite();
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
