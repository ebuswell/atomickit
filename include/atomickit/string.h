/** @file string.h
 * Atomic String
 *
 * An immutable string suitable for use with Atomic Kit.
 */
/*
 * Copyright 2013 Evan Buswell
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

struct astr {
	struct arcp_region;
	size_t len;
	char *data;
};

typedef void (*astr_destroy_f)(struct astr *);

void astr_init(struct astr *str, size_t len, char *data,
               astr_destroy_f destroy);
struct astr *astr_create(size_t len, char *data);
struct astr *astr_alloc(size_t len);
struct astr *astr_cstrwrap(char *cstr);
struct astr *astr_dup(struct astr *str);
struct astr *astr_cstrdup(char *cstr);
static inline size_t astr_len(struct astr *str) {
	return str->len;
}
struct astr *astr_cpy(struct astr *dest, struct astr *src);
struct astr *astr_cstrcpy(struct astr *dest, char *src);
struct astr *astr_cat(struct astr *str1, struct astr *str2);
struct astr *astr_cstrcat(struct astr *str1, char *str2);
struct astr *astr_chr(struct astr *str, char chr);
struct astr *astr_rchr(struct astr *str, char chr);
struct astr *astr_str(struct astr *haystack, struct astr *needle);
struct astr *astr_rstr(struct astr *haystack, struct astr *needle);
struct astr *astr_cstrstr(struct astr *haystack, char *needle);
struct astr *astr_cstrrstr(struct astr *haystack, char *needle);
int astr_cmp(struct astr *a, struct astr *b);
int astr_cstrcmp(struct astr *a, char *b);
static inline char *astr_cstr(struct astr *str) {
	return str->data;
}

#endif /* ! ATOMICKIT_STRING_H */
