/*
 * test.c
 * 
 * Copyright 2010 Evan Buswell
 * 
 * This file is part of Cshellsynth.
 * 
 * Cshellsynth is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Cshellsynth is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Cshellsynth.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <atomickit/atomic-list.h>
#include <stdio.h>
#include <stdlib.h>

char *test_string1 = "Test String 1";
char *test_string2 = "Test String 2";
char *test_string3 = "Test String 3";

#define CHECKING(function)			\
    printf("Checking " #function "...")		\
    
#define OK()					\
    printf("OK\n")


#define CHECK_GET(function, expected)				\
    do {							\
	test_return = function(&list);				\
	if(test_return == ALST_ERROR) {				\
	    perror("Error: ");					\
	    exit(1);						\
	}							\
	if(test_return == ALST_EMPTY) {				\
	    printf("Error: list claims to be empty\n");		\
	    exit(1);						\
	}							\
	if(test_return != expected) {				\
	    printf("Error: received unexpected value\n");	\
	    exit(1);						\
	}							\
    } while(0)

#define CHECK_GET2(function, index, expected)			\
    do {							\
	test_return = function(&list, index);			\
	if(test_return == ALST_ERROR) {				\
	    perror("Error: ");					\
	    exit(1);						\
	}							\
	if(test_return == ALST_EMPTY) {				\
	    printf("Error: list claims to be empty\n");		\
	    exit(1);						\
	}							\
	if(test_return != expected) {				\
	    printf("Error: received unexpected value\n");	\
	    exit(1);						\
	}							\
    } while(0)

#define CHECK_PUT(function, value)		\
    do {					\
	r = function(&list, value);		\
	if(r != 0) {				\
	    perror("Error: ");			\
	    exit(1);				\
	}					\
    } while(0)

#define CHECK_PUT2(function, index, value)	\
    do {					\
	r = function(&list, index, value);	\
	if(r != 0) {				\
	    perror("Error: ");			\
	    exit(1);				\
	}					\
    } while(0)

#define CHECK_LENGTH(expected)						\
    do {								\
	length = atomic_list_length(&list);				\
	if(r == -1) {							\
	    perror("Error: ");						\
	    exit(1);							\
	}								\
	if(length != expected) {					\
	    printf("Error: expected length %zd, found length %zd\n", (size_t) expected, length); \
	    exit(1);							\
	}								\
    } while(0)

int main(int argc, char **argv) {
    int r;
    void *test_return;
    size_t length;

    atomic_list_t list;
    CHECKING(atomic_list_init);
    r = atomic_list_init(&list);
    if(r != 0) {
	perror("Error: ");
	exit(1);
    }
    OK();

    CHECKING(atomic_list_length);
    CHECK_LENGTH(0);
    OK();

    CHECKING(atomic_list_push);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_LENGTH(1);
    CHECK_PUT(atomic_list_push, test_string3);
    CHECK_LENGTH(2);
    OK();

    CHECKING(atomic_list_unshift);
    CHECK_PUT(atomic_list_unshift, test_string1);
    CHECK_LENGTH(3);
    OK();

    CHECKING(atomic_list_get);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string2);
    CHECK_GET2(atomic_list_get, 2, test_string3);
    CHECK_LENGTH(3);
    OK();

    CHECKING(atomic_list_pop);
    CHECK_GET(atomic_list_pop, test_string3);
    CHECK_LENGTH(2);
    CHECK_PUT(atomic_list_push, test_string3);
    CHECK_LENGTH(3);
    OK();

    CHECKING(atomic_list_shift);
    CHECK_GET(atomic_list_shift, test_string1);
    CHECK_LENGTH(2);
    CHECK_PUT(atomic_list_unshift, test_string1);
    CHECK_LENGTH(3);
    OK();

    CHECKING(atomic_list_first);
    CHECK_GET(atomic_list_first, test_string1);
    CHECK_LENGTH(3);
    OK();

    CHECKING(atomic_list_last);
    CHECK_GET(atomic_list_last, test_string3);
    CHECK_LENGTH(3);
    OK();

    CHECKING(atomic_list_set);
    CHECK_PUT2(atomic_list_set, 1, test_string3);
    CHECK_LENGTH(3);
    CHECK_GET2(atomic_list_get, 1, test_string3);
    CHECK_PUT2(atomic_list_set, 1, test_string2);
    CHECK_LENGTH(3);
    OK();

    CHECKING(atomic_list_insert);
    CHECK_PUT2(atomic_list_insert, 1, test_string3);
    CHECK_LENGTH(4);
    CHECK_GET2(atomic_list_get, 1, test_string3);
    CHECK_GET2(atomic_list_get, 2, test_string2);
    CHECK_LENGTH(4);
    OK();

    CHECKING(atomic_list_remove);
    CHECK_GET2(atomic_list_remove, 1, test_string3);
    CHECK_LENGTH(3);
    CHECK_GET2(atomic_list_get, 1, test_string2);
    CHECK_LENGTH(3);
    OK();

    CHECKING(atomic_list_remove_by_value);
    r = atomic_list_remove_by_value(&list, test_string2);
    if(r != 0) {
	perror("Error: ");
	exit(1);
    }
    CHECK_LENGTH(2);
    CHECK_GET(atomic_list_first, test_string1);
    CHECK_GET(atomic_list_last, test_string3);
    OK();    

    CHECKING(atomic_list_clear);
    r = atomic_list_clear(&list);
    if(r != 0) {
	perror("Error: ");
	exit(1);
    }
    CHECK_LENGTH(0);
    OK();

    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    
    CHECKING(atomic_list_readlock);
    atomic_list_readlock(&list);
    OK();

    CHECKING(nonatomic_list_length);
    length = nonatomic_list_length(&list);
    if(length != 3) {
	printf("Error: expected length %zd, found length %zd\n", (size_t) 3, length);
	exit(1);
    }
    OK();

    CHECKING(nonatomic_list_first);
    CHECK_GET(nonatomic_list_first, test_string1);
    OK();

    CHECKING(nonatomic_list_last);
    CHECK_GET(nonatomic_list_last, test_string3);
    OK();

    CHECKING(nonatomic_list_get);
    CHECK_GET2(nonatomic_list_get, 0, test_string1);
    CHECK_GET2(nonatomic_list_get, 1, test_string2);
    CHECK_GET2(nonatomic_list_get, 2, test_string3);
    OK();

    CHECKING(atomic_list_readunlock);
    atomic_list_readunlock(&list);
    OK();

    CHECKING(atomic_list_destroy);
    r = atomic_list_destroy(&list);
    if(r != 0) {
	perror("Error: ");
	exit(1);
    }
    OK();

    exit(0);
}
