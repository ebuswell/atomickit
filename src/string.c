/*
 * string.c
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

#include <stddef.h>
#define _GNU_SOURCE
#include <string.h>
#undef _GNU_SOURCE
#include "atomickit/malloc.h"
#include "atomickit/string.h"

struct astrstr {
	struct astr;
	char data_start[];
};

void astr_init(struct astr *str, size_t len, char *data, void (*destroy)(struct astr *)) {
	arcp_region_init(str, (void (*)(struct arcp_region *)) destroy);
	str->len = len;
	str->data = data;
}

static void __astr_destroystr(struct astr *str) {
	afree(str, sizeof(struct astrstr) + str->len + 1);
}

static void __astr_destroy(struct astr *str) {
	afree(str, sizeof(struct astr));
}

struct astr *astr_create(size_t len, char *data) {
	struct astr *str;
	str = amalloc(sizeof(struct astr));
	if(str == NULL) {
		return NULL;
	}

	astr_init(str, len, data, __astr_destroy);
	return str;
}

struct astr *astr_alloc(size_t len) {
	struct astrstr *str;
	str = amalloc(sizeof(struct astrstr) + len + 1);
	if(str == NULL) {
		return NULL;
	}

	astr_init(str, 0, str->data_start, __astr_destroystr);
	return str;
}

struct astr *astr_cstrwrap(char *cstr) {
	return astr_create(strlen(cstr), cstr);
}

struct astr *astr_cstrdup(char *cstr) {
	size_t len;
	struct astr *rstr;
	len = strlen(cstr);
	rstr = astr_alloc(len);
	if(rstr == NULL) {
		return NULL;
	}

	memcpy(rstr->data, cstr, len + 1);
	rstr->len = len;

	return rstr;
}

struct astr *astr_dup(struct astr *str) {
	struct astr *rstr;
	rstr = astr_alloc(str->len);
	if(rstr == NULL) {
		return NULL;
	}

	memcpy(rstr->data, str->data, str->len + 1);
	rstr->len = str->len;

	return rstr;
}

struct astr *astr_cpy(struct astr *dest, struct astr *src) {
	memcpy(dest->data, src->data, src->len + 1);
	dest->len = src->len;
	return dest;
}

struct astr *astr_cstrcpy(struct astr *dest, char *src) {
	size_t src_len;
	src_len = strlen(src);
	memcpy(dest->data, src, src_len + 1);
	dest->len = src_len;
	return dest;
}

struct astr *astr_cat(struct astr *str1, struct astr *str2) {
	memcpy(str1->data + str1->len, str2->data, str2->len + 1);
	str1->len += str2->len;
	return str1;
}

struct astr *astr_cstrcat(struct astr *str1, char *str2) {
	size_t str2_len;
	str2_len = strlen(str2);
	memcpy(str1->data + str1->len, str2, str2_len + 1);
	str1->len += str2_len;
	return str1;
}

struct astr *astr_chr(struct astr *str, char chr) {
	char *rdata;
	rdata = memchr(str->data, chr, str->len);
	if(rdata == NULL) {
		return NULL;
	}

	return astr_create(str->len - ((size_t) (rdata - str->data)), rdata);
}

struct astr *astr_rchr(struct astr *str, char chr) {
	char *rdata;
	rdata = memrchr(str->data, chr, str->len);
	if(rdata == NULL) {
		return NULL;
	}

	return astr_create(str->len - ((size_t) (rdata - str->data)), rdata);
}

struct astr *astr_str(struct astr *haystack, struct astr *needle) {
	char *rdata;
	rdata = memmem(haystack->data, haystack->len, needle->data, needle->len);
	if(rdata == NULL) {
		return NULL;
	}

	return astr_create(haystack->len - ((size_t) (rdata - haystack->data)), rdata); 
}

struct astr *astr_cstrstr(struct astr *haystack, char *needle) {
	char *rdata;
	rdata = memmem(haystack->data, haystack->len, needle, strlen(needle));
	if(rdata == NULL) {
		return NULL;
	}

	return astr_create(haystack->len - ((size_t) (rdata - haystack->data)), rdata); 
}

struct astr *astr_rstr(struct astr *haystack, struct astr *needle) {
	size_t last;
	size_t i;
	if(haystack->len < needle->len) {
		return NULL;
	}
	last = haystack->len - needle->len;
	for(i = last; i <= last; i--) {
		if(memcmp(haystack->data + i, needle->data, needle->len) == 0) {
			return astr_create(haystack->len - i, haystack->data + i);
		}
	}
	return NULL;
}

struct astr *astr_cstrrstr(struct astr *haystack, char *needle) {
	size_t last;
	size_t i;
	size_t needle_len;
	needle_len = strlen(needle);
	if(haystack->len < needle_len) {
		return NULL;
	}
	last = haystack->len - needle_len;
	for(i = last; i <= last; i--) {
		if(memcmp(haystack->data + i, needle, needle_len) == 0) {
			return astr_create(haystack->len - i, haystack->data + i);
		}
	}
	return NULL;
}

int astr_cmp(struct astr *a, struct astr *b) {
	int r;
	if(a->len > b->len) {
		r = memcmp(a->data, b->data, b->len);
		return r == 0 ? 1 : r;
	} else if(a->len < b->len) {
		r = memcmp(a->data, b->data, a->len);
		return r == 0 ? -1 : r;
	} else {
		return memcmp(a->data, b->data, a->len);
	}
}

int astr_cstrcmp(struct astr *a, char *b) {
	return strcmp(a->data, b);
}
