/*
 * test.c
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
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/select.h>
#include <setjmp.h>
#include <limits.h>

#include "test.h"

struct test_result_list *test_results = NULL;
static FILE *test_writer;
static struct test_result *c_test_res = NULL;
static jmp_buf test_end_jmp;

static int run_test_fork(void (*fixture)(void (*)()),
			 char *test_name, void (*test)());
static void checkpoint_test_fork(char *file, int line);
static void end_test_fork(enum test_status status, char *explanation);
static int run_test_nofork(void (*fixture)(void (*)()),
			   char *test_name, void (*test)());
static void checkpoint_test_nofork(char *file, int line);
static void end_test_nofork(enum test_status status, char *explanation);

int (*run_test)(void (*fixture)(void (*)()), char *test_name, void (*test)())
	= run_test_fork;
void (*checkpoint_test)(char *file, int line) = checkpoint_test_fork;
void (*end_test)(enum test_status status, char *explanation) = end_test_fork;

void test_config_nofork() {
	run_test = run_test_nofork;
	checkpoint_test = checkpoint_test_nofork;
	end_test = end_test_nofork;
}

#define TEST_RESULT_LIST_INITIAL_SIZE 8

int test_results_push(struct test_result *result) {
	if (test_results == NULL) {
		test_results = malloc(sizeof(struct test_result_list)
				      + (TEST_RESULT_LIST_INITIAL_SIZE - 1)
				        * sizeof(struct test_result *));
		if (test_results == NULL) {
			return -1;
		}
		test_results->nresults = 0;
		test_results->results_capacity
			= TEST_RESULT_LIST_INITIAL_SIZE;
	} else if (test_results->nresults == test_results->results_capacity) {
		struct test_result_list *new_test_results;
		test_results->results_capacity *= 2;
		new_test_results = realloc(test_results,
					   sizeof(struct test_result_list)
					   + (test_results->results_capacity
					      - 1)
					     * sizeof(struct test_result *));
		if (new_test_results == NULL) {
			test_results->results_capacity /= 2;
			return -1;
		}
		test_results = new_test_results;
	}
	test_results->results[test_results->nresults++] = result;
	return 0;
}

struct test_result *test_result_create(char *test_name,
				       enum test_status status,
				       char *explanation,
				       char *file, int line) {
	struct test_result *result;
	result = malloc(sizeof(struct test_result));
	if (result == NULL) {
		return NULL;
	}
	result->test_name = NULL;
	result->status = status;
	result->explanation = NULL;
	result->file = NULL;
	result->line = -1;
	if (test_name != NULL) {
		result->test_name = strdup(test_name);
		if (result->test_name == NULL) {
			test_result_free(result);
			return NULL;
		}
	}
	if (explanation != NULL) {
		result->explanation = strdup(explanation);
		if (result->explanation == NULL) {
			test_result_free(result);
			return NULL;
		}
	}
	if (file != NULL) {
		result->file = strdup(file);
		if (result->file == NULL) {
			test_result_free(result);
			return NULL;
		}
	}
	result->line = line;
	return result;
}

void test_result_free(struct test_result *result) {
	if (result->test_name != NULL) {
		free(result->test_name);
	}
	if (result->explanation != NULL) {
		free(result->explanation);
	}
	if (result->file != NULL) {
		free(result->file);
	}
	free(result);
}

int no_test(char *test_name, enum test_status result, char *explanation) {
	int r;
	struct test_result *myresult;
	myresult = test_result_create(test_name, result, explanation,
				      NULL, 0);
	if (myresult == NULL) {
		return -1;
	}
	r = test_results_push(myresult);
	if (r != 0) {
		test_result_free(myresult);
		return r;
	}
	return 0;
}

static int test_set_error(int myerrno, char *explanation,
			  struct test_result *result) {
	if (result->explanation != NULL) {
		free(result->explanation);
	}
	if (myerrno != 0) {
		char *se;
		se = strerror(errno);
		result->explanation = malloc(strlen(explanation)
					     + strlen(": ")
					     + strlen(se)
					     + 1);
		if (result->explanation == NULL) {
			test_result_free(result);
			return -1;
		}
		sprintf(result->explanation, "%s: %s", explanation, se);
	} else {
		result->explanation = strdup(explanation);
		if (result->explanation == NULL) {
			test_result_free(result);
			return -1;
		}
	}
	result->status = TEST_UNRESOLVED;
	return 0;
}

void checkpoint_test_fork(char *file, int line) {
	if ((fprintf(test_writer, "CHECKPOINT:%s:%d\n", file, line) < 0)
	   || (fflush(test_writer) != 0)) {
		fclose(test_writer);
		exit(EXIT_FAILURE);
	}
}

void checkpoint_test_nofork(char *file, int line) {
	file = strdup(file);
	if (file == NULL) {
		int r;
		r = test_set_error(0, "Could not strdup file", c_test_res);
		longjmp(test_end_jmp, r == 0 ? 1 : -1);
	}
	if (c_test_res->file != NULL) {
		free(c_test_res->file);
	}
	c_test_res->file = file;
	c_test_res->line = line;
}

void end_test_fork(enum test_status status, char *explanation) {
	const char *format;
	int ret;
	switch (status) {
	case TEST_PASS:
		format = "PASS:%s\n";
		break;
	case TEST_FAIL:
		format = "FAIL:%s\n";
		break;
	case TEST_UNTESTED:
		format = "UNTESTED:%s\n";
		break;
	case TEST_UNSUPPORTED:
		format = "UNSUPPORTED:%s\n";
		break;
	case TEST_UNRESOLVED:
	default:
		format = "UNRESOLVED:%s\n";
	}
	ret = EXIT_SUCCESS;
	if (fprintf(test_writer, format, explanation) < 0) {
		ret = EXIT_FAILURE;
	}
	fclose(test_writer);
	exit(ret);
}

static void end_test_nofork(enum test_status status, char *explanation) {
	int r;
	explanation = strdup(explanation);
	if (explanation == NULL) {
		r = test_set_error(errno, "Could not strdup explanation",
				   c_test_res);
		longjmp(test_end_jmp, r == 0 ? 1 : -1);
	}
	c_test_res->status = status;
	c_test_res->explanation = explanation;
	longjmp(test_end_jmp, 1);
}

static int run_test_nofork(void (*fixture)(void (*)()),
			   char *test_name, void (*test)()) {
	int r;
	c_test_res = test_result_create(test_name, TEST_PASS, NULL, NULL, 0);
	if (c_test_res == NULL) {
		return -1;
	}
	if ((r = setjmp(test_end_jmp)) == 0) {
		r = 1;
		if (fixture != NULL) {
			fixture(test);
		} else {
			test();
		}
	}
	/* allow for a fall-through pass */
	if (r != 1) {
		return r;
	}
	/* success! */
	r = test_results_push(c_test_res);
	if (r != 0) {
		test_result_free(c_test_res);
		return r;
	}
	return 0;
}

static int test_parse_line(char *line, struct test_result *result) {
	if ((strlen(line) < 1)
	   || (strcmp(line, "\n") == 0)) {
		/* blank line */
		return 0;
	}
	if (strncmp(line, "CHECKPOINT:", strlen("CHECKPOINT:")) == 0) {
		char *file;
		char *lineno;
		char *lineno_end;
		unsigned long lineno_int;
		file = line + strlen("CHECKPOINT:");
		lineno = strchr(file, ':');
		if (lineno == NULL) {
			return test_set_error(0,
					      "Failure to parse child"
					      " CHECKPOINT file name",
					      result);
		}
		*lineno++ = '\0';
		lineno_end = strchr(lineno, '\n');
		if (lineno_end != NULL) {
			*lineno_end = '\0';
		}
		lineno_int = strtoul(lineno, &lineno_end, 0);
		if (*lineno_end != '\0') {
			return test_set_error(0,
					      "Failure to parse child"
					      " CHECKPOINT line number",
					      result);
		}
		if (lineno_int > INT_MAX) {
			return test_set_error(0,
					      "Failure to parse child"
					      " CHECKPOINT line number:"
					      " value out of range",
					      result);
		}
		file = strdup(file);
		if (file == NULL) {
			return test_set_error(errno,
					      "Could not strdup file",
					      result);
		}
		if (result->file != NULL) {
			free(result->file);
		}
		result->file = file;
		result->line = (int) lineno_int;
	} else {
		char *message;
		char *maybenl;
		message = line;
		if (strncmp(message, "PASS:", strlen("PASS:")) == 0) {
			result->status = TEST_PASS;
			message += strlen("PASS:");
		} else if (strncmp(message, "FAIL:", strlen("FAIL:")) == 0) {
			result->status = TEST_FAIL;
			message += strlen("FAIL:");
		} else if (strncmp(message, "UNRESOLVED:",
				   strlen("UNRESOLVED:")) == 0) {
			result->status = TEST_UNRESOLVED;
			message += strlen("UNRESOLVED:");
		} else if (strncmp(message, "UNTESTED:",
				   strlen("UNTESTED:")) == 0) {
			result->status = TEST_UNTESTED;
			message += strlen("UNTESTED:");
		} else if (strncmp(message, "UNSUPPORTED:",
				   strlen("UNSUPPORTED:")) == 0) {
			result->status = TEST_UNSUPPORTED;
			message += strlen("UNSUPPORTED:");
		} else {
			return test_set_error(0,
					      "Failure to parse child"
					      "message",
					      result);
		}
		maybenl = strchr(message, '\n');
		if (maybenl != NULL) {
			*maybenl = '\0';
		}
		result->explanation = strdup(message);
		if (result->explanation == NULL) {
			return test_set_error(0,
					      "Could not strdup message",
					      result);
		}
	}
	return 0;
}

static int run_test_fork(void (*fixture)(void (*)()),
			 char *test_name, void (*test)()) {
	pid_t pid = 0;
	int pipefd[2] = { 0, 0 };
	FILE *reader = NULL;
	int r;
	int ret = 0;
	struct test_result *result;

	result = test_result_create(test_name, TEST_PASS, NULL, NULL, 0);
	if (result == NULL) {
		return -1;
	}

	r = pipe(pipefd);
	if (r != 0) {
		pipefd[0] = 0;
		pipefd[1] = 0;
		ret = test_set_error(errno, "Could not create pipe", result);
		goto abort;
	}

	if ((pid = fork()) == 0) {
		/* child */
		r = close(pipefd[0]); /* close read */
		if (r != 0) {
			exit(EXIT_FAILURE);
		}
		test_writer = fdopen(pipefd[1], "w");
		if (test_writer == NULL) {
			exit(EXIT_FAILURE);
		}
		if (fixture != NULL) {
			fixture(test);
		} else {
			test();
		}
		fprintf(test_writer, "PASS:\n");
		fclose(test_writer); /* ignore errors */
		exit(EXIT_SUCCESS);
	} else if (pid == -1) {
		ret = test_set_error(errno, "Could not fork", result);
		goto abort;
	}

	/* parent */
	{
		char line[256];
		struct timeval tv;
		fd_set read_fds;
		fd_set error_fds;
		int status;

		r = close(pipefd[1]); /* close write */
		if (r != 0) {
			ret = test_set_error(errno,
					     "Could not close pipefd[1]",
					     result);
			goto abort;
		}
		pipefd[1] = 0;

		reader = fdopen(pipefd[0], "r");
		if (reader == NULL) {
			ret = test_set_error(errno,
					     "Could not fdopen reader",
					     result);
			goto abort;
		}

		for (;;) {
			tv.tv_sec = 30;
			tv.tv_usec = 0;
			FD_ZERO(&read_fds);
			FD_ZERO(&error_fds);
			FD_SET(pipefd[0], &read_fds);
			FD_SET(pipefd[0], &error_fds);
			r = select(pipefd[0] + 1, &read_fds, NULL, &error_fds,
				   &tv);
			if (r < 0) {
				if (errno == EINTR || errno == EAGAIN) {
					continue;
				} else {
					ret = test_set_error(errno,
							     "select failed",
							     result);
					goto abort;
				}
			}
			if (r == 0) {
				kill(pid, SIGTERM);
				ret = test_set_error(0, "Timeout", result);
				goto abort;
			}
			if (fgets(line, 256, reader) == NULL) {
				break;
			}
			if (result->explanation != NULL) {
				/* We already have a result... */
				ret = test_set_error(0,
						     "Received double result"
						     " from child process",
						     result);
				goto abort;
			}
			ret = test_parse_line(line, result);
			if ((ret != 0)
			   || (result->status != TEST_PASS)) {
				goto abort;
			}
		}
		if (ferror(reader)) {
			ret = test_set_error(errno,
					     "Error when reading",
					     result);
			goto abort;
		}
		r = fclose(reader);
		if (r != 0) {
			ret = test_set_error(errno,
					     "Could not close reader",
					     result);
			goto abort;
		}
		reader = NULL;
		pipefd[0] = 0;
		pid = waitpid(pid, &status, 0);
		if (pid == -1) {
			ret = test_set_error(errno,
					     "Error cleaning up child"
					     " process",
					     result);
			goto abort;
		}
		pid = 0;
		if (WIFEXITED(status)) {
			if ((status = WEXITSTATUS(status)) != EXIT_SUCCESS) {
				char *uerror;
				uerror = alloca(strlen("Child process"
						       " terminated with"
						       " failure exit"
						       " status: ") + 16);
				sprintf(uerror,
					"Child process terminated"
					" with failure exit status: %d",
					status);
				ret = test_set_error(0, uerror, result);
				goto abort;
			}
		} else if (WIFSIGNALED(status)) {
			char *ss;
			char *uerror;
			ss = strsignal(WTERMSIG(status));
			uerror = alloca(strlen("Child process terminated on"
					       " signal: ") + strlen(ss) + 1);
			sprintf(uerror,
				"Child process terminated on"
				" signal: %s", ss);
			ret = test_set_error(0, uerror, result);
			goto abort;
		} else {
			/* unknown state */
			ret = test_set_error(0,
					     "Child process terminated in"
					     " unknown state",
					     result);
			goto abort;
		}
		if (result->explanation == NULL) {
			ret = test_set_error(0,
					     "Child process exited early",
					     result);
			goto abort;
		}

	abort:
		/* clean up, ignoring errors */
		if (reader != NULL) {
			fclose(reader);
		} else if (pipefd[0] != 0) {
			close(pipefd[0]);
		}
		if (pipefd[1] != 0) {
			close(pipefd[1]);
		}
		if (pid > 0) {
			pid = waitpid(pid, &status, 0);
		}
		if (ret == 0) {
			if (strlen(result->explanation) == 0) {
				free(result->explanation);
				result->explanation = NULL;
			}
			r = test_results_push(result);
			if (r != 0) {
				test_result_free(result);
				return r;
			}
		}
		return ret;
	}
}

int run_test_suite(void (*fixture)(void (*)()),
		   char **test_name, void (**test)()) {
	int r;
	for (; *test_name != NULL; test_name++, test++) {
		if (*test == NULL) {
			r = no_test(*test_name, TEST_UNTESTED,
				    "No test defined");
		} else {
			r = run_test(fixture, *test_name, *test);
		}
		if (r != 0) {
			char *explanation;
			struct test_result *result;
			explanation = strdup("Test framework failed to record"
					     " result");
			if (explanation == NULL) {
				return -1;
			}
			result = test_result_create(*test_name,
						    TEST_UNRESOLVED,
						    explanation, NULL, 0);
			if (result == NULL) {
				free(explanation);
				return -1;
			}
			r = test_results_push(result);
			if (r != 0) {
				test_result_free(result);
				return r;
			}
		}
	}
	return 0;
}

int print_test_results() {
	size_t nresults = 0, passed = 0, failed = 0, unresolved = 0,
	       untested = 0, unsupported = 0;
	int r;
	if (test_results != NULL) {
		size_t i;
		/* count results */
		nresults = test_results->nresults;
		for (i = 0; i < nresults; i++) {
			char *status;
			char *file;
			int line;
			switch (test_results->results[i]->status) {
			case TEST_PASS:
				passed++;
				continue;
			case TEST_FAIL:
				failed++;
				status = "FAIL";
				break;
			case TEST_UNTESTED:
				untested++;
				status = "UNTESTED";
				break;
			case TEST_UNSUPPORTED:
				unsupported++;
				status = "UNSUPPORTED";
				break;
			case TEST_UNRESOLVED:
			default:
				status = "UNRESOLVED";
				unresolved++;
			}
			file = test_results->results[i]->file;
			line = test_results->results[i]->line;
			if (file == NULL) {
				file = "<unknown>";
			}
			r = fprintf(stderr, "%s:%d: %s:%s", file, line,
				    status,
				    test_results->results[i]->test_name);
			if (test_results->results[i]->explanation != NULL) {
				r = fprintf(stderr, ":%s",
					test_results->results[i]->explanation);
				if (r < 0) {
					return r;
				}
			}
			r = fprintf(stderr, "\n");
			if (r < 0) {
				return r;
			}
		}
	}
	r = printf("Results: Passed: %zd/%zd, Failed: %zd/%zd,"
		   " Unresolved: %zd/%zd, Untested: %zd/%zd,"
		   " Unsupported: %zd/%zd\n",
		   passed, nresults, failed, nresults, unresolved, nresults,
		   untested, nresults, unsupported, nresults);
	if (r < 0) {
		return r;
	}
	return 0;
}

bool tests_succeeded() {
   size_t i;
   if (test_results == NULL) {
	   return true;
   }
   for (i = 0; i < test_results->nresults; i++) {
	   switch (test_results->results[i]->status) {
	   case TEST_PASS:
	   case TEST_UNTESTED:
	   case TEST_UNSUPPORTED:
		   break;
	   case TEST_FAIL:
	   case TEST_UNRESOLVED:
	   default:
		   return false;
	   }
   }
   return true;
}
