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
#include "atomickit/atomic-list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *test_string1 = "Test String 1";
char *test_string2 = "Test String 2";
char *test_string3 = "Test String 3";

char sprintf_val[256];

static char *my_sprintf(char *template, int line) {
    snprintf(sprintf_val, 256, template, line);
    return sprintf_val;
}

static char *my_strdup(void * str, int dummy) {
    return strdup((char *) str);
}

#define CHECKING(function)			\
    printf("Checking " #function "...")		\
    
#define OK()					\
    printf("OK\n")


#define CHECK_GET(function, expected)					\
    do {								\
	test_return = function(&list);					\
	if(test_return == ALST_ERROR) {					\
	    perror(my_sprintf("Error at line %i", __LINE__));		\
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
	if(test_return == ALST_EMPTY) {					\
	    printf("Error at line %i: list claims to be empty\n", __LINE__); \
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
	if(test_return != expected) {					\
	    printf("Error at line %i: received unexpected value \"%s\" (expected \"%s\")\n", __LINE__, (char *) test_return, (char *) expected); \
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
    } while(0)

#define CHECK_GET_EMPTY(function)					\
    do {								\
	test_return = function(&list);					\
	if(test_return == ALST_ERROR) {					\
	    perror(my_sprintf("Error at line %i", __LINE__));		\
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
	if(test_return != ALST_EMPTY) {					\
	    printf("Error at line %i: list should be empty\n", __LINE__); \
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
    } while(0)

#define CHECK_GET_ERROR(function)					\
    do {								\
	test_return = function(&list);					\
	if(test_return != ALST_ERROR) {					\
	    printf("Error at line %i: list should have given an error\n", __LINE__); \
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
    } while(0)

#define CHECK_GET2(function, index, expected)				\
    do {								\
	test_return = function(&list, index);				\
	if(test_return == ALST_ERROR) {					\
	    perror(my_sprintf("Error at line %i", __LINE__));		\
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
	if(test_return == ALST_EMPTY) {					\
	    printf("Error at line %i: list claims to be empty\n", __LINE__); \
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
	if(test_return != expected) {					\
	    printf("Error at line %i: received unexpected value \"%s\" (expected \"%s\")\n", __LINE__, (char *) test_return, (char *) expected); \
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
    } while(0)

#define CHECK_GET2_EMPTY(function, index)				\
    do {								\
	test_return = function(&list, index);				\
	if(test_return == ALST_ERROR) {					\
	    perror(my_sprintf("Error at line %i", __LINE__));		\
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
	if(test_return != ALST_EMPTY) {					\
	    printf("Error at line %i: list should be empty\n", __LINE__); \
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
    } while(0)

#define CHECK_GET2_ERROR(function, index)				\
    do {								\
	test_return = function(&list, index);				\
	if(test_return != ALST_ERROR) {					\
	    printf("Error at line %i: list should have given an error\n", __LINE__); \
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
    } while(0)

#define CHECK_PUT(function, value)					\
    do {								\
	r = function(&list, value);					\
	if(r != 0) {							\
	    perror(my_sprintf("Error at line %i", __LINE__));		\
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
    } while(0)

#define CHECK_PUT2(function, index, value)			\
    do {							\
	r = function(&list, index, value);			\
	if(r != 0) {						\
	    perror(my_sprintf("Error at line %i", __LINE__));	\
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));	\
	    exit(1);						\
	}							\
    } while(0)

#define CHECK_PUT2_ERROR(function, index, value)			\
    do {								\
	r = function(&list, index, value);				\
	if(r == 0) {							\
	    printf("Error at line %i: list should have given an error\n", __LINE__); \
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
    } while(0)

#define CHECK_LENGTH(expected)						\
    do {								\
	length = atomic_list_length(&list);				\
	if(length != expected) {					\
	    printf("Error at line %i: expected length %zd, found length %zd\n", __LINE__, (size_t) expected, length); \
	    printf("%s\n", atomic_list_to_string(&list, my_strdup));		\
	    exit(1);							\
	}								\
    } while(0)


int cmpstrings(void *str1, void *str2) {
    return memcmp(str1, str2, 13);
}

int main(int argc, char **argv) {
    int r;
    void *test_return;
    size_t length;

    atomic_list_t list;
    setbuf(stdout, NULL);
    CHECKING(atomic_list_init);
    r = atomic_list_init(&list);
    if(r != 0) {
	perror(my_sprintf("Error at line %i", __LINE__));
	printf("%s\n", atomic_list_to_string(&list, my_strdup));
	exit(1);
    }
    OK();

    CHECKING(atomic_list_init_with_capacity);
    r = atomic_list_init_with_capacity(&list, 1);
    if(r != 0) {
	perror(my_sprintf("Error at line %i", __LINE__));
	printf("%s\n", atomic_list_to_string(&list, my_strdup));
	exit(1);
    }
    OK();

    CHECKING(atomic_list_length);
    CHECK_LENGTH(0);
    OK();

    CHECKING(atomic_list_push);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_LENGTH(1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_LENGTH(2);
    CHECK_PUT(atomic_list_push, test_string3);
    CHECK_LENGTH(3);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string2);
    CHECK_GET2(atomic_list_get, 2, test_string3);
    OK();

    CHECKING(atomic_list_clear);
    atomic_list_clear(&list);
    CHECK_LENGTH(0);
    OK();

    CHECKING(atomic_list_unshift);
    CHECK_PUT(atomic_list_unshift, test_string3);
    CHECK_LENGTH(1);
    CHECK_PUT(atomic_list_unshift, test_string2);
    CHECK_LENGTH(2);
    CHECK_PUT(atomic_list_unshift, test_string1);
    CHECK_LENGTH(3);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string2);
    CHECK_GET2(atomic_list_get, 2, test_string3);
    OK();

    CHECKING(atomic_list_get);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string2);
    CHECK_GET2(atomic_list_get, 2, test_string3);
    CHECK_GET2_ERROR(atomic_list_get, 3);
    CHECK_GET2_ERROR(atomic_list_get, -1);
    CHECK_LENGTH(3);
    OK();

    CHECKING(atomic_list_compact);
    r = atomic_list_compact(&list);
    if(r != 0) {
    	perror(my_sprintf("Error at line %i", __LINE__));
	printf("%s\n", atomic_list_to_string(&list, my_strdup));
    	exit(1);
    }
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string2);
    CHECK_GET2(atomic_list_get, 2, test_string3);
    CHECK_GET2_ERROR(atomic_list_get, 3);
    CHECK_GET2_ERROR(atomic_list_get, -1);
    OK();

    CHECKING(atomic_list_prealloc);
    r = atomic_list_prealloc(&list, 200);
    if(r != 0) {
    	perror(my_sprintf("Error at line %i", __LINE__));
	printf("%s\n", atomic_list_to_string(&list, my_strdup));
    	exit(1);
    }
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string2);
    CHECK_GET2(atomic_list_get, 2, test_string3);
    CHECK_GET2_ERROR(atomic_list_get, 3);
    CHECK_GET2_ERROR(atomic_list_get, -1);
    OK();


    CHECKING(atomic_list_pop);
    CHECK_GET(atomic_list_pop, test_string3);
    CHECK_LENGTH(2);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string2);
    CHECK_GET(atomic_list_pop, test_string2);
    CHECK_LENGTH(1);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET(atomic_list_pop, test_string1);
    CHECK_LENGTH(0);
    CHECK_GET_EMPTY(atomic_list_pop);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    CHECK_LENGTH(3);
    OK();

    CHECKING(atomic_list_shift);
    CHECK_GET(atomic_list_shift, test_string1);
    CHECK_LENGTH(2);
    CHECK_GET2(atomic_list_get, 0, test_string2);
    CHECK_GET2(atomic_list_get, 1, test_string3);
    CHECK_GET(atomic_list_shift, test_string2);
    CHECK_LENGTH(1);
    CHECK_GET2(atomic_list_get, 0, test_string3);
    CHECK_GET(atomic_list_shift, test_string3);
    CHECK_LENGTH(0);
    CHECK_GET_EMPTY(atomic_list_shift);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    OK();

    CHECKING(atomic_list_first);
    CHECK_GET(atomic_list_first, test_string1);
    CHECK_LENGTH(3);
    CHECK_GET(atomic_list_shift, test_string1);
    CHECK_GET(atomic_list_first, test_string2);
    CHECK_LENGTH(2);
    CHECK_GET(atomic_list_shift, test_string2);
    CHECK_GET(atomic_list_first, test_string3);
    CHECK_LENGTH(1);
    CHECK_GET(atomic_list_shift, test_string3);
    CHECK_GET_EMPTY(atomic_list_first);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    OK();

    CHECKING(atomic_list_last);
    CHECK_GET(atomic_list_last, test_string3);
    CHECK_LENGTH(3);
    CHECK_GET(atomic_list_pop, test_string3);
    CHECK_GET(atomic_list_last, test_string2);
    CHECK_LENGTH(2);
    CHECK_GET(atomic_list_pop, test_string2);
    CHECK_GET(atomic_list_last, test_string1);
    CHECK_LENGTH(1);
    CHECK_GET(atomic_list_pop, test_string1);
    CHECK_GET_EMPTY(atomic_list_last);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    OK();

    CHECKING(atomic_list_set);
    CHECK_PUT2(atomic_list_set, 0, test_string3);
    CHECK_PUT2(atomic_list_set, 1, test_string1);
    CHECK_PUT2(atomic_list_set, 2, test_string2);
    CHECK_LENGTH(3);
    CHECK_GET2(atomic_list_get, 0, test_string3);
    CHECK_GET2(atomic_list_get, 1, test_string1);
    CHECK_GET2(atomic_list_get, 2, test_string2);
    CHECK_PUT2_ERROR(atomic_list_set, -1, test_string1);
    CHECK_PUT2_ERROR(atomic_list_set, 3, test_string1);
    atomic_list_clear(&list);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    OK();

    CHECKING(atomic_list_insert);
    CHECK_PUT2(atomic_list_insert, 1, test_string3);
    CHECK_LENGTH(4);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string3);
    CHECK_GET2(atomic_list_get, 2, test_string2);
    CHECK_GET2(atomic_list_get, 3, test_string3);
    CHECK_PUT2(atomic_list_insert, 4, test_string1);
    CHECK_LENGTH(5);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string3);
    CHECK_GET2(atomic_list_get, 2, test_string2);
    CHECK_GET2(atomic_list_get, 3, test_string3);
    CHECK_GET2(atomic_list_get, 4, test_string1);
    atomic_list_clear(&list);
    CHECK_PUT2(atomic_list_insert, 0, test_string3);
    CHECK_PUT2(atomic_list_insert, 0, test_string2);
    CHECK_PUT2(atomic_list_insert, 0, test_string1);
    CHECK_LENGTH(3);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string2);
    CHECK_GET2(atomic_list_get, 2, test_string3);
    OK();

    CHECKING(atomic_list_remove);
    CHECK_GET2(atomic_list_remove, 1, test_string2);
    CHECK_LENGTH(2);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string3);

    CHECK_GET2(atomic_list_remove, 1, test_string3);
    CHECK_LENGTH(1);
    CHECK_GET2(atomic_list_get, 0, test_string1);

    CHECK_GET2(atomic_list_remove, 0, test_string1);
    CHECK_LENGTH(0);
    CHECK_GET2_ERROR(atomic_list_remove, 0);
    CHECK_GET2_ERROR(atomic_list_remove, 1);
    CHECK_GET2_ERROR(atomic_list_remove, -1);
    CHECK_LENGTH(0);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    CHECK_GET2_ERROR(atomic_list_remove, 3);
    CHECK_GET2_ERROR(atomic_list_remove, -1);
    OK();

    CHECKING(atomic_list_remove_by_value);
    atomic_list_remove_by_value(&list, test_string2);
    CHECK_LENGTH(2);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string3);
    CHECK_PUT2(atomic_list_insert, 1, test_string2);

    atomic_list_remove_by_value(&list, test_string3);
    CHECK_LENGTH(2);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);

    atomic_list_remove_by_value(&list, test_string1);
    CHECK_LENGTH(2);
    CHECK_GET2(atomic_list_get, 0, test_string2);
    CHECK_GET2(atomic_list_get, 1, test_string3);
    CHECK_PUT(atomic_list_unshift, test_string1);

    atomic_list_remove_by_value(&list, test_string1);
    atomic_list_remove_by_value(&list, test_string2);
    atomic_list_remove_by_value(&list, test_string3);
    CHECK_LENGTH(0);
    atomic_list_remove_by_value(&list, test_string1);
    atomic_list_remove_by_value(&list, test_string2);
    atomic_list_remove_by_value(&list, test_string3);
    CHECK_LENGTH(0);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    OK();

    CHECKING(atomic_list_readlock);
    atomic_list_readlock(&list);
    OK();

    CHECKING(atomic_list_readunlock);
    atomic_list_readunlock(&list);
    OK();

    CHECKING(nonatomic_list_length);
    atomic_list_readlock(&list);
    length = nonatomic_list_length(&list);
    if(length != 3) {
	printf("Error at line %i: expected length %zd, found length %zd\n", __LINE__, (size_t) 3, length);
	printf("%s\n", atomic_list_to_string(&list, my_strdup));
	exit(1);
    }
    atomic_list_readunlock(&list);
    CHECK_GET(atomic_list_shift, test_string1);
    CHECK_GET(atomic_list_shift, test_string2);
    CHECK_GET(atomic_list_shift, test_string3);
    atomic_list_readlock(&list);
    length = nonatomic_list_length(&list);
    if(length != 0) {
	printf("Error at line %i: expected length %zd, found length %zd\n", __LINE__, (size_t) 3, length);
	exit(1);
    }
    atomic_list_readunlock(&list);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    OK();

    CHECKING(nonatomic_list_first);
    atomic_list_readlock(&list);
    CHECK_GET(nonatomic_list_first, test_string1);
    atomic_list_readunlock(&list);
    CHECK_LENGTH(3);
    CHECK_GET(atomic_list_shift, test_string1);
    atomic_list_readlock(&list);
    CHECK_GET(nonatomic_list_first, test_string2);
    atomic_list_readunlock(&list);
    CHECK_LENGTH(2);
    CHECK_GET(atomic_list_shift, test_string2);
    atomic_list_readlock(&list);
    CHECK_GET(nonatomic_list_first, test_string3);
    atomic_list_readunlock(&list);
    CHECK_LENGTH(1);
    CHECK_GET(atomic_list_shift, test_string3);
    atomic_list_readlock(&list);
    CHECK_GET_EMPTY(nonatomic_list_first);
    atomic_list_readunlock(&list);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    OK();

    CHECKING(nonatomic_list_last);
    atomic_list_readlock(&list);
    CHECK_GET(nonatomic_list_last, test_string3);
    atomic_list_readunlock(&list);
    CHECK_LENGTH(3);
    CHECK_GET(atomic_list_pop, test_string3);
    atomic_list_readlock(&list);
    CHECK_GET(nonatomic_list_last, test_string2);
    atomic_list_readunlock(&list);
    CHECK_LENGTH(2);
    CHECK_GET(atomic_list_pop, test_string2);
    atomic_list_readlock(&list);
    CHECK_GET(nonatomic_list_last, test_string1);
    atomic_list_readunlock(&list);
    CHECK_LENGTH(1);
    CHECK_GET(atomic_list_pop, test_string1);
    atomic_list_readlock(&list);
    CHECK_GET_EMPTY(nonatomic_list_last);
    atomic_list_readunlock(&list);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    OK();

    CHECKING(nonatomic_list_get);
    atomic_list_readlock(&list);
    CHECK_GET2(nonatomic_list_get, 0, test_string1);
    CHECK_GET2(nonatomic_list_get, 1, test_string2);
    CHECK_GET2(nonatomic_list_get, 2, test_string3);
    CHECK_GET2_ERROR(nonatomic_list_get, 3);
    CHECK_GET2_ERROR(nonatomic_list_get, -1);
    atomic_list_readunlock(&list);
    CHECK_GET(atomic_list_shift, test_string1);
    CHECK_GET(atomic_list_shift, test_string2);
    CHECK_GET(atomic_list_shift, test_string3);
    atomic_list_readlock(&list);
    CHECK_GET2_ERROR(nonatomic_list_get, 0);
    CHECK_GET2_ERROR(nonatomic_list_get, 1);
    CHECK_GET2_ERROR(nonatomic_list_get, -1);
    atomic_list_readunlock(&list);
    OK();

    CHECKING(atomic_iterator_init);
    atomic_iterator_t i;
    r = atomic_iterator_init(&list, &i);
    if(r != 0) {
	perror(my_sprintf("Error at line %i", __LINE__));
	printf("%s\n", atomic_list_to_string(&list, my_strdup));
	exit(1);
    }
    OK();

    CHECKING(atomic_iterator_destroy);
    atomic_iterator_destroy(&list, &i);
    OK();

    CHECKING(atomic_iterator_next);
    r = atomic_iterator_init(&list, &i);
    if(r != 0) {
	perror(my_sprintf("Error at line %i", __LINE__));
	printf("%s\n", atomic_list_to_string(&list, my_strdup));
	exit(1);
    }
    CHECK_GET2_EMPTY(atomic_iterator_next, &i);
    atomic_iterator_destroy(&list, &i);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    r = atomic_iterator_init(&list, &i);
    if(r != 0) {
	perror(my_sprintf("Error at line %i", __LINE__));
	printf("%s\n", atomic_list_to_string(&list, my_strdup));
	exit(1);
    }
    CHECK_GET2(atomic_iterator_next, &i, test_string1);
    CHECK_GET2(atomic_iterator_next, &i, test_string2);
    CHECK_PUT(atomic_list_unshift, test_string1);
    CHECK_PUT(atomic_list_unshift, test_string1);
    CHECK_PUT(atomic_list_unshift, test_string1);
    CHECK_GET2(atomic_iterator_next, &i, test_string3);
    CHECK_GET2_EMPTY(atomic_iterator_next, &i);
    atomic_iterator_destroy(&list, &i);
    atomic_list_clear(&list);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    OK();

    CHECKING(atomic_list_reverse);
    atomic_list_reverse(&list);
    CHECK_LENGTH(3);
    CHECK_GET2(atomic_list_get, 0, test_string3);
    CHECK_GET2(atomic_list_get, 1, test_string2);
    CHECK_GET2(atomic_list_get, 2, test_string1);
    atomic_list_clear(&list);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    CHECK_PUT(atomic_list_push, test_string1);
    atomic_list_reverse(&list);
    CHECK_LENGTH(4);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string3);
    CHECK_GET2(atomic_list_get, 2, test_string2);
    CHECK_GET2(atomic_list_get, 3, test_string1);
    atomic_list_clear(&list);
    atomic_list_reverse(&list);
    CHECK_LENGTH(0);
    OK();

    CHECKING(atomic_list_sort);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string2);
    CHECK_PUT(atomic_list_push, test_string3);
    CHECK_PUT(atomic_list_push, test_string1);
    CHECK_PUT(atomic_list_push, test_string3);
    atomic_list_sort(&list, cmpstrings);
    CHECK_LENGTH(5);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string1);
    CHECK_GET2(atomic_list_get, 2, test_string2);
    CHECK_GET2(atomic_list_get, 3, test_string3);
    CHECK_GET2(atomic_list_get, 4, test_string3);
    atomic_list_clear(&list);
    atomic_list_sort(&list, cmpstrings);    
    CHECK_PUT(atomic_list_push, test_string1);
    atomic_list_sort(&list, cmpstrings);    
    CHECK_LENGTH(1);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    atomic_list_clear(&list);
    OK();

    CHECKING(atomic_list_insert_sorted);
    CHECK_PUT2(atomic_list_insert_sorted, cmpstrings, test_string1);
    CHECK_PUT2(atomic_list_insert_sorted, cmpstrings, test_string3);
    CHECK_PUT2(atomic_list_insert_sorted, cmpstrings, test_string2);
    CHECK_PUT2(atomic_list_insert_sorted, cmpstrings, test_string1);
    CHECK_PUT2(atomic_list_insert_sorted, cmpstrings, test_string2);
    CHECK_LENGTH(5);
    CHECK_GET2(atomic_list_get, 0, test_string1);
    CHECK_GET2(atomic_list_get, 1, test_string1);
    CHECK_GET2(atomic_list_get, 2, test_string2);
    CHECK_GET2(atomic_list_get, 3, test_string2);
    CHECK_GET2(atomic_list_get, 4, test_string3);
    OK();

    CHECKING(atomic_list_to_string);
    char *str = atomic_list_to_string(&list, my_strdup);
    if(str == NULL) {
	perror(my_sprintf("Error at line %i", __LINE__));
	exit(1);
    }
    printf("\n%s\n", str);
    free(str);
    OK();

    CHECKING(atomic_list_destroy);
    atomic_list_destroy(&list);
    OK();

    exit(0);
}
