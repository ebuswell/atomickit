/** @file string.h
 * Atomic String
 *
 * An immutable string suitable for use with Atomic Kit.
 */
/*
 * Copyright 2014 Evan Buswell
 * 
 * This file is part of Atomic Kit.
 * 
 * Atomic Kit is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 2.
 * 
 * Atomic Kit is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef ATOMICKIT_STRING_H
#define ATOMICKIT_STRING_H 1

#include <stddef.h>
#include <atomickit/rcp.h>

/**
 * Atomic String
 *
 * The string structure to be stored in an arcp_t container.
 */
struct astr {
	struct arcp_region;
	size_t len;		/**< the number of bytes in the string data */
	char *data;		/**< a pointer to the string data */
};

/**
 * Destruction function for string.
 */
typedef void (*astr_destroy_f)(struct astr *);

/**
 * Initialize a string with the specified data.
 *
 * @param str the string to initialize.
 * @param len the length of the string data.
 * @param data a pointer to the string data.
 * @param destroy the destruction function for this astr.
 */
void astr_init(struct astr *str, size_t len, char *data,
	       astr_destroy_f destroy);

/**
 * Create a new string using the specified data.
 *
 * @param len the length of the string data.
 * @param data a pointer to the string data.
 *
 * @returns a pointer to the string, or NULL if the string could not be
 * created.
 */
struct astr *astr_create(size_t len, char *data);

/**
 * Allocate a new str of the specified size.
 *
 * The length of the string is initially set to zero, but it is assumed that
 * the string will be filled to capacity before destruction.
 *
 * @param len the length of the string data.
 *
 * @returns a pointer to the string, or NULL if the string could not be
 * allocated.
 */
struct astr *astr_alloc(size_t len);

/**
 * Wrap a C string in an astr structure.
 *
 * @param cstr the C string to wrap.
 *
 * @returns a pointer to the string, or NULL if the string could not be
 * allocated.
 */
struct astr *astr_cstrwrap(char *cstr);

/**
 * Duplicate a string.
 *
 * @param str the string to duplicate
 *
 * @returns a pointer to the duplicated string, or NULL if the string could
 * not be duplicated.
 */
struct astr *astr_dup(struct astr *str);

/**
 * Create a string whose data is a duplicate of the provided C string.
 *
 * @param cstr the C string to duplicate.
 *
 * @returns a pointer to the string, or NULL of the string could not be
 * created.
 */
struct astr *astr_cstrdup(char *cstr);

/**
 * Get the length of a given string.
 *
 * @param str the string whose length we want.
 *
 * @returns the length of the given string.
 */
static inline size_t astr_len(struct astr *str) {
	return str->len;
}

/**
 * Copy the source to the destination string, overwriting the destination
 * string.
 *
 * @param dest the destination string.
 * @param src the source string.
 *
 * @returns the destination string.
 */
struct astr *astr_cpy(struct astr *dest, struct astr *src);

/**
 * Copy the C string to the destination string, overwriting the destination
 * string.
 *
 * @param dest the destination string.
 * @param src the source C string.
 *
 * @returns the destination string.
 */
struct astr *astr_cstrcpy(struct astr *dest, char *src);

/**
 * Concatenate str2 on to the end of str1.
 *
 * @param str1 the first string, and the string which will be written to.
 * @param str2 the second string to be copied on to the end of the first.
 *
 * @returns the modified str1.
 */
struct astr *astr_cat(struct astr *str1, struct astr *str2);

/**
 * Concatenate the C string str2 on to the end of str1.
 *
 * @param str1 the first string, and the string which will be written to.
 * @param str2 the C string to be copied on to the end of the first.
 *
 * @returns the modified str1.
 */
struct astr *astr_cstrcat(struct astr *str1, char *str2);

/**
 * Search through string for a specified character.
 *
 * @param str the string to search.
 * @param chr the character to search for.
 *
 * @returns a new string starting with the searched for character, or NULL if
 * the character is not found or there is an allocation error.
 */
struct astr *astr_chr(struct astr *str, char chr);

/**
 * Search through string for a specified character, starting at the end.
 *
 * @param str the string to search.
 * @param chr the character to search for.
 *
 * @returns a new string starting with the searched for character, or NULL if
 * the character is not found or there is an allocation error.
 */
struct astr *astr_rchr(struct astr *str, char chr);

/**
 * Search through string for a specified substring.
 *
 * @param haystack the string to search.
 * @param needle the string to search for.
 *
 * @returns a new string starting with the searched for string, or NULL if the
 * string is not found or there is an allocation error.
 */
struct astr *astr_str(struct astr *haystack, struct astr *needle);

/**
 * Search through string for a specified substring, starting at the end.
 *
 * Since there is not a good library function to be wrapped here, this is
 * naive and inefficient for right now.
 *
 * @param haystack the string to search.
 * @param needle the string to search for.
 *
 * @returns a new string starting with the searched for string, or NULL if the
 * string is not found or there is an allocation error.
 */
struct astr *astr_rstr(struct astr *haystack, struct astr *needle);

/**
 * Search through string for a specified C substring.
 *
 * @param haystack the string to search.
 * @param needle the C string to search for.
 *
 * @returns a new string starting with the searched for string, or NULL if the
 * string is not found or there is an allocation error.
 */
struct astr *astr_cstrstr(struct astr *haystack, char *needle);

/**
 * Search through string for a specified C substring, starting at the end.
 *
 * Since there is not a good library function to be wrapped here, this is
 * naive and inefficient for right now.
 *
 * @param haystack the string to search.
 * @param needle the C string to search for.
 *
 * @returns a new string starting with the searched for string, or NULL if the
 * string is not found or there is an allocation error.
 */
struct astr *astr_cstrrstr(struct astr *haystack, char *needle);

/**
 * Compare two strings.
 *
 * @param a the first string to compare.
 * @param b the second string to compare.
 *
 * @returns an integer less than, equal to, or greater than zero if a is less
 * than, equal to, or greater than b, respectively.
 */
int astr_cmp(struct astr *a, struct astr *b);

/**
 * Compare a string with a C string.
 *
 * @param a the string to compare.
 * @param b the C string to compare.
 *
 * @returns an integer less than, equal to, or greater than zero if a is less
 * than, equal to, or greater than b, respectively.
 */
int astr_cstrcmp(struct astr *a, char *b);

/**
 * Return the C string corresponding with a given string.
 *
 * @param str the string for which to return the C string.
 *
 * @returns the C string for the specified string.
 */
static inline char *astr_cstr(struct astr *str) {
	return str->data;
}

#endif /* ! ATOMICKIT_STRING_H */
