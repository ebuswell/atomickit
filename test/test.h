#ifndef TEST_H
#define TEST_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

enum test_status { PASS, FAIL, UNRESOLVED, UNTESTED, UNSUPPORTED };

struct test_result {
    char *test_name;
    enum test_status status;
    char *explanation;
    char *file;
    char *line;
};

struct test_result_list {
    size_t nresults;
    size_t results_capacity;
    struct test_result *results[1];
};

extern struct test_result_list *test_results;

extern FILE *test_writer;

int test_results_push(struct test_result *result);
struct test_result *test_result_create(char *test_name, enum test_status result, char *explanation, char *file, char *line);
void test_result_free(struct test_result *result);
int no_test(char *test_name, enum test_status result, char *explanation);
int run_test(void (*fixture)(void (*)()), char *test_name, void (*test)());
int run_test_suite(void (*fixture)(void (*)()), char **test_name, void (**test)());
int print_test_results();
bool tests_succeeded();

#define CHECKPOINT()							\
    do {								\
	if((fprintf(test_writer, "CHECKPOINT:%s:%d\n", __FILE__, __LINE__) < 0) \
	   || (fflush(test_writer) != 0)) {				\
	    fclose(test_writer);					\
	    exit(EXIT_FAILURE);						\
	}								\
    } while(0)								\

#define ASSERT(assertion)						\
    do {								\
	CHECKPOINT();							\
	if(!assertion) {						\
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

#define FAIL(reason)							\
    do {								\
	CHECKPOINT();							\
	if(fprintf(test_writer, "FAIL:%s\n", reason) < 0) {		\
	    fclose(test_writer);					\
	    exit(EXIT_FAILURE);						\
	}								\
	if(fclose(test_writer) != 0) {					\
	    exit(EXIT_FAILURE);						\
	}								\
	exit(EXIT_SUCCESS);						\
    } while(0)

#define UNRESOLVED(reason)							\
    do {								\
	CHECKPOINT();							\
	if(fprintf(test_writer, "UNRESOLVED:%s\n", reason) < 0) {	\
	    fclose(test_writer);					\
	    exit(EXIT_FAILURE);						\
	}								\
	if(fclose(test_writer) != 0) {					\
	    exit(EXIT_FAILURE);						\
	}								\
	exit(EXIT_SUCCESS);						\
    } while(0)

#define UNTESTED(reason)							\
    do {								\
	CHECKPOINT();							\
	if(fprintf(test_writer, "UNTESTED:%s\n", reason) < 0) {	\
	    fclose(test_writer);					\
	    exit(EXIT_FAILURE);						\
	}								\
	if(fclose(test_writer) != 0) {					\
	    exit(EXIT_FAILURE);						\
	}								\
	exit(EXIT_SUCCESS);						\
    } while(0)

#define UNSUPPORTED(reason)							\
    do {								\
	CHECKPOINT();							\
	if(fprintf(test_writer, "UNSUPPORTED:%s\n", reason) < 0) {	\
	    fclose(test_writer);					\
	    exit(EXIT_FAILURE);						\
	}								\
	if(fclose(test_writer) != 0) {					\
	    exit(EXIT_FAILURE);						\
	}								\
	exit(EXIT_SUCCESS);						\
    } while(0)

/* It's recommended you don't use this. */
#define PASS(reason)							\
    do {								\
	CHECKPOINT();							\
	if(fprintf(test_writer, "PASS:%s\n", reason) < 0) {	\
	    fclose(test_writer);					\
	    exit(EXIT_FAILURE);						\
	}								\
	if(fclose(test_writer) != 0) {					\
	    exit(EXIT_FAILURE);						\
	}								\
	exit(EXIT_SUCCESS);						\
    } while(0)

#endif /* ! TEST_H */
