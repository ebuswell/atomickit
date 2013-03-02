#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "test.h"

struct test_result_list *test_results = NULL;

#define TEST_RESULT_LIST_INITIAL_SIZE 8

int test_results_push(struct test_result *result) {
    if(test_results == NULL) {
	test_results = malloc(sizeof(struct test_result_list) + (TEST_RESULT_LIST_INITIAL_SIZE - 1) * sizeof(struct test_result *));
	if(test_results == NULL) {
	    return -1;
	}
	test_results->nresults = 0;
	test_results->results_capacity = TEST_RESULT_LIST_INITIAL_SIZE;
    } else if(test_results->nresults == test_results->results_capacity) {
	test_results->results_capacity *= 2;
	struct test_result_list *new_test_results = realloc(test_results, sizeof(struct test_result_list) + (test_results->results_capacity - 1) * sizeof(struct test_result *));
	if(new_test_results == NULL) {
	    test_results->results_capacity /= 2;
	    return -1;
	}
	test_results = new_test_results;
    }
    test_results->results[test_results->nresults++] = result;
    return 0;
}

struct test_result *test_result_create(char *test_name, enum test_status status, char *explanation, char *file, char *line) {
    struct test_result *result = malloc(sizeof(struct test_result));
    if(result == NULL) {
	return NULL;
    }
    result->test_name = NULL;
    result->status = status;
    result->explanation = NULL;
    result->file = NULL;
    result->line = NULL;
    if(test_name != NULL) {
	result->test_name = strdup(test_name);
	if(result->test_name == NULL) {
	    test_result_free(result);
	    return NULL;
	}
    }
    if(explanation != NULL) {
	result->explanation = strdup(explanation);
	if(result->explanation == NULL) {
	    test_result_free(result);
	    return NULL;
	}
    }
    if(file != NULL) {
	result->file = strdup(file);
	if(result->file == NULL) {
	    test_result_free(result);
	    return NULL;
	}
    }
    if(line != NULL) {
	result->line = strdup(line);
	if(result->line == NULL) {
	    test_result_free(result);
	    return NULL;
	}
    }
    return result;
}

void test_result_free(struct test_result *result) {
    if(result->test_name != NULL) {
	free(result->test_name);
    }
    if(result->explanation != NULL) {
	free(result->explanation);
    }
    if(result->file != NULL) {
	free(result->file);
    }
    if(result->line != NULL) {
	free(result->line);
    }
    free(result);
}

int no_test(char *test_name, enum test_status result, char *explanation) {
    int r;
    struct test_result *myresult = test_result_create(test_name, result, explanation, NULL, NULL);
    if(myresult == NULL) {
	return -1;
    }
    r = test_results_push(myresult);
    if(r != 0) {
	test_result_free(myresult);
	return r;
    }
    return 0;
}

FILE *test_writer;

int run_test(void (*fixture)(void (*)()), char *test_name, void (*test)()) {
    pid_t pid = 0;
    int pipefd[2];
    int r;
    char *uerror;
    int myerrno = 0;
    struct test_result *result = test_result_create(test_name, UNRESOLVED, NULL, NULL, NULL);
    if(result == NULL) {
	return -1;
    }

    r = pipe(pipefd);
    if(r != 0) {
	myerrno = errno;
	uerror = "Could not create pipe";
	goto error;
    }

    if((pid = fork()) == 0) {
	/* child */
	r = close(pipefd[0]); /* close read */
	if(r != 0) {
	    exit(EXIT_FAILURE);
	}
	test_writer = fdopen(pipefd[1], "w");
	if(test_writer == NULL) {
	    exit(EXIT_FAILURE);
	}
	fixture(test);
	fprintf(test_writer, "PASS:\n");
	fclose(test_writer); /* ignore errors */
	exit(EXIT_SUCCESS);
    } else if(pid == -1) {
	myerrno = errno;
	uerror = "Could not fork";
	goto error;
    }

    /* parent */
    char line[256];
    FILE *reader;

    r = close(pipefd[1]); /* close write */
    if(r != 0) {
	myerrno = errno;
	uerror = "Could not close pipefd[1]";
	goto error;
    }

    reader = fdopen(pipefd[0], "r");
    if(reader == NULL) {
	myerrno = errno;
	uerror = "Could not fdopen reader";
	goto error;
    }

    while(fgets(line, 256, reader) != NULL) {
	if((strlen(line) < 1)
	   || (strcmp(line, "\n") == 0)) {
	    /* blank line */
	    continue;
	}
	if(result->explanation != NULL) {
	    /* We already did this... */
	    uerror = "Received double result from child process";
	    goto error;
	}
	if(strncmp(line, "CHECKPOINT:", strlen("CHECKPOINT:")) == 0) {
	    char *file = line + strlen("CHECKPOINT:");
	    char *lineno = strchr(file, ':');
	    if(lineno == NULL) {
		uerror = "Failure to parse child message";
		goto error;
	    }
	    *lineno++ = '\0';
	    char *maybenl = strchr(lineno, '\n');
	    if(maybenl != NULL) {
		*maybenl = '\0';
	    }
	    file = strdup(file);
	    if(file == NULL) {
		myerrno = errno;
		uerror = "Could not strdup file";
		goto error;
	    }
	    lineno = strdup(lineno);
	    if(lineno == NULL) {
		myerrno = errno;
		free(file);
		uerror = "Could not strdup line";
		goto error;
	    }
	    if(result->file != NULL) {
		free(result->file);
	    }
	    if(result->line != NULL) {
		free(result->line);
	    }
	    result->file = file;
	    result->line = lineno;
	} else {
	    char *message = line;
	    if(strncmp(message, "PASS:", strlen("PASS:")) == 0) {
		result->status = PASS;
		message += strlen("PASS:");
	    } else if(strncmp(message, "FAIL:", strlen("FAIL:")) == 0) {
		result->status = FAIL;
		message += strlen("FAIL:");
	    } else if(strncmp(message, "UNRESOLVED:", strlen("UNRESOLVED:")) == 0) {
		result->status = UNRESOLVED;
		message += strlen("UNRESOLVED:");
	    } else if(strncmp(message, "UNTESTED:", strlen("UNTESTED:")) == 0) {
		result->status = UNTESTED;
		message += strlen("UNTESTED:");
	    } else if(strncmp(message, "UNSUPPORTED:", strlen("UNSUPPORTED:")) == 0) {
		result->status = UNSUPPORTED;
		message += strlen("UNSUPPORTED:");
	    } else {
		uerror = "Failure to parse child message";
		goto error;
	    }
	    char *maybenl = strchr(message, '\n');
	    if(maybenl != NULL) {
		*maybenl = '\0';
	    }
	    result->explanation = strdup(message);
	    if(result->explanation == NULL) {
		uerror = "Could not strdup message";
		goto error;
	    }
	}
    }
    if(ferror(reader)) {
	/* uhhh... */
	myerrno = errno;
	uerror = "Error when reading";
	goto error;
    }
    r = fclose(reader);
    if(r != 0) {
	myerrno = errno;
	uerror = "Could not close reader";
	goto error;
    }
    int status;
    pid = waitpid(pid, &status, 0);
    if(pid == -1) {
	myerrno = errno;
	uerror = "Error cleaning up child process";
	goto error;
    }
    pid = 0;
    if(WIFEXITED(status)) {
	if((status = WEXITSTATUS(status)) != EXIT_SUCCESS) {
	    uerror = alloca(strlen("Child process terminated with failure exit status: ") + 3 + 1);
	    sprintf(uerror, "Child process terminated with failure exit status: %d", status);
	    goto error;
	} else if(result->explanation == NULL) {
	    uerror = "Child process exited early";
	    goto error;
	}
    } else if(WIFSIGNALED(status)) {
	char *ss = strsignal(WTERMSIG(status));
	uerror = alloca(strlen("Child process terminated on signal: ") + strlen(ss) + 1);
	sprintf(uerror, "Child process terminated on signal: %s", ss);
	goto error;
    } else {
	/* unknown state */
	uerror = "Child process terminated in unknown state";
	goto error;
    }
    /* Success! */
    r = test_results_push(result);
    if(r != 0) {
	test_result_free(result);
	return r;
    }
    return 0;

error:
    if(pid > 0) {
	pid = waitpid(pid, &status, 0); /* ignore errors */
    }
    if(result->explanation != NULL) {
	free(result->explanation);
    }
    if(myerrno != 0) {
	char *se = strerror(errno);
	result->explanation = malloc(strlen(uerror) + strlen(": ") + strlen(se) + 1);
	if(result->explanation == NULL) {
	    test_result_free(result);
	    return -1;
	}
	sprintf(result->explanation, "%s: %s", uerror, se);
    } else {
	result->explanation = strdup(uerror);
	if(result->explanation == NULL) {
	    test_result_free(result);
	    return -1;
	}
    }
    result->status = UNRESOLVED;
    r = test_results_push(result);
    if(r != 0) {
	test_result_free(result);
	return r;
    }
    return 0;
}

int run_test_suite(void (*fixture)(void (*)()), char **test_name, void (**test)()) {
    int r;
    for(; *test_name != NULL; test_name++, test++) {
	if(*test == NULL) {
	    r = no_test(*test_name, UNTESTED, "No test defined");
	} else {
	    r = run_test(fixture, *test_name, *test);
	}
	if(r != 0) {
	    char *explanation = strdup("Test framework failed to record result");
	    if(explanation == NULL) {
		return -1;
	    }
	    struct test_result *myresult = test_result_create(test_name, UNRESOLVED, explanation, NULL, NULL);
	    if(myresult == NULL) {
		free(explanation);
		return -1;
	    }
	    r = test_results_push(myresult);
	    if(r != 0) {
		test_result_free(myresult);
		return r;
	    }
	}
    }
    return 0;
}

int print_test_results() {
    size_t nresults = 0, passed = 0, failed = 0, unresolved = 0, untested = 0, unsupported = 0;
    int r;
    if(test_results != NULL) {
	/* count results */
	nresults = test_results->nresults;
	size_t i;
	for(i = 0; i < nresults; i++) {
	    char *status;
	    switch(test_results->results[i]->status) {
	    case PASS:
		passed++;
		continue;
	    case FAIL:
		failed++;
		status = "FAIL";
		break;
	    case UNTESTED:
		untested++;
		status = "UNTESTED";
		break;
	    case UNSUPPORTED:
		unsupported++;
		status = "UNSUPPORTED";
		break;
	    case UNRESOLVED:
	    default:
		status = "UNRESOLVED";
		unresolved++;
	    }
	    char *file;
	    char *line;
	    if(test_results->results[i]->file != NULL) {
		file = test_results->results[i]->file;
		if(test_results->results[i]->line != NULL) {
		    line = test_results->results[i]->line;
		} else {
		    line = "1";
		}
	    } else {
		file = "<unknown>";
		line = "0";
	    }
	    r = fprintf(stderr, "%s:%s: %s:%s", file, line, status, test_results->results[i]->test_name);
	    if(test_results->results[i]->explanation != NULL) {
		r = fprintf(stderr, ": %s", test_results->results[i]->explanation);
		if(r < 0) {
		    return r;
		}
	    }
	    r = fprintf(stderr, "\n");
	    if(r < 0) {
		return r;
	    }
	}
    }
    r = printf("Results: Passed: %zd/%zd, Failed: %zd/%zd, Unresolved: %zd/%zd, Untested: %zd/%zd, Unsupported: %zd/%zd\n",
	   passed, nresults, failed, nresults, unresolved, nresults, untested, nresults, unsupported, nresults);
    if(r < 0) {
	return r;
    }
    return 0;
}

bool tests_succeeded() {
   if(test_results == NULL) {
       return true;
   }
   size_t i;
   for(i = 0; i < test_results->nresults; i++) {
       switch(test_results->results[i]->status) {
       case PASS:
       case UNTESTED:
       case UNSUPPORTED:
	   break;
       case FAIL:
       case UNRESOLVED:
       default:
	   return false;
       }
   }
   return true;
}
