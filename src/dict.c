/*
 * dict.c
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
#include <errno.h>
#include <atomickit/atomic-rcp.h>
#include <atomickit/atomic-string.h>
#include <atomickit/atomic-malloc.h>
#include <atomickit/atomic-dict.h>

static void adict_destroy(struct adict *dict) {
	size_t i;
	for(i = 0; i < dict->len; i++) {
		arcp_release(dict->items[i].key);
		arcp_release(dict->items[i].value);
	}
	afree(dict, ADICT_SIZE(dict->len));
}

struct adict *adict_dup(struct adict *dict) {
	struct adict *ret;
	size_t i;
	ret = amalloc(ADICT_SIZE(dict->len));
	if(ret == NULL) {
		return NULL;
	}
	arcp_region_init(ret, (void (*)(struct arcp_region *)) adict_destroy);
	ret->len = dict->len;
	for(i = 0; i < dict->len; i++) {
		ret->items[i].key = (struct astr *) arcp_acquire(dict->items[i].key);
		ret->items[i].value = arcp_acquire(dict->items[i].value);
	}
	return ret;
}

struct adict *adict_create() {
	struct adict *ret;
	ret = amalloc(ADICT_OVERHEAD);
	if(ret == NULL) {
		return NULL;
	}
	arcp_region_init(ret, (void (*)(struct arcp_region *)) adict_destroy);
	ret->len = 0;
	return ret;
}

#define IF_BSEARCH(list, nitems, key, retidx) do { \
	size_t __l;                                \
	size_t __u;                                \
	struct astr *__k;                          \
	int __r;                                   \
	(retidx) = __l = 0;                        \
	__u = (nitems);                            \
	while(__l < __u) {                         \
		(retidx) = (__l + __u) / 2;        \
		__k = (list)[retidx].key;          \
		__r = astr_cmp((key), __k);        \
		if(__r < 0) {                      \
			__u = (retidx);            \
		} else if(__r > 0) {               \
			__l = ++(retidx);          \
		} else

#define ENDIF_BSEARCH \
	}             \
} while(0)

#define IF_BSEARCHCSTR(list, nitems, key, retidx) do { \
	size_t __l;                                    \
	size_t __u;                                    \
	struct astr *__k;                              \
	int __r;                                       \
	(retidx) = __l = 0;                            \
	__u = (nitems);                                \
	while(__l < __u) {                             \
		(retidx) = (__l + __u) / 2;            \
		__k = (list)[retidx].key;              \
		__r = astr_cstrcmp(__k, (key));        \
		if(__r > 0) {                          \
			__u = (retidx);                \
		} else if(__r < 0) {                   \
			__l = ++(retidx);              \
		} else

#define ENDIF_BSEARCHCSTR \
	}                 \
} while(0)

int adict_set(struct adict *dict, struct astr *key, struct arcp_region *value) {
	size_t i;
	IF_BSEARCH(dict->items, dict->len, key, i) {
		arcp_release(dict->items[i].value);
		dict->items[i].value = arcp_acquire(value);
		return 0;
	} ENDIF_BSEARCH;
	errno = EINVAL;
	return -1;
}

int adict_cstrset(struct adict *dict, char *key, struct arcp_region *value) {
	size_t i;
	IF_BSEARCHCSTR(dict->items, dict->len, key, i) {
		arcp_release(dict->items[i].value);
		dict->items[i].value = arcp_acquire(value);
		return 0;
	} ENDIF_BSEARCHCSTR;
	errno = EINVAL;
	return -1;
}

struct arcp_region *adict_get(struct adict *dict, struct astr *key) {
	size_t i;
	IF_BSEARCH(dict->items, dict->len, key, i) {
		return arcp_acquire(dict->items[i].value);
	} ENDIF_BSEARCH;
	errno = EINVAL;
	return NULL;
}

struct arcp_region *adict_cstrget(struct adict *dict, char *key) {
	size_t i;
	IF_BSEARCHCSTR(dict->items, dict->len, key, i) {
		return arcp_acquire(dict->items[i].value);
	} ENDIF_BSEARCHCSTR;
	errno = EINVAL;
	return NULL;
}

bool adict_has(struct adict *dict, struct astr *key) {
	size_t i;
	IF_BSEARCH(dict->items, dict->len, key, i) {
		return true;
	} ENDIF_BSEARCH;
	return false;
}

bool adict_cstrhas(struct adict *dict, char *key) {
	size_t i;
	IF_BSEARCHCSTR(dict->items, dict->len, key, i) {
		return true;
	} ENDIF_BSEARCHCSTR;
	return false;
}

struct adict *adict_put(struct adict *dict, struct astr *key, struct arcp_region *value) {
	size_t i, j;
	struct adict *ret;
	IF_BSEARCH(dict->items, dict->len, key, i) {
		ret = amalloc(ADICT_SIZE(dict->len));
		if(ret == NULL) {
			return NULL;
		}
		arcp_region_init(ret, (void (*)(struct arcp_region *)) adict_destroy);
		ret->len = dict->len;
		for(j = 0; j < i; j++) {
			ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j].key);
			ret->items[j].value = arcp_acquire(dict->items[j].value);
		}
		ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j].key);
		ret->items[j].value = arcp_acquire(value);
		for(j++; j < dict->len; j++) {
			ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j].key);
			ret->items[j].value = arcp_acquire(dict->items[j].value);
		}
		return ret;
	} ENDIF_BSEARCH;
	ret = amalloc(ADICT_SIZE(dict->len + 1));
	if(ret == NULL) {
		return NULL;
	}
	arcp_region_init(ret, (void (*)(struct arcp_region *)) adict_destroy);
	ret->len = dict->len + 1;
	for(j = 0; j < i; j++) {
		ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j].key);
		ret->items[j].value = arcp_acquire(dict->items[j].value);
	}
	ret->items[j].key = (struct astr *) arcp_acquire(key);
	ret->items[j].value = arcp_acquire(value);
	for(j++; j < ret->len; j++) {
		ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j - 1].key);
		ret->items[j].value = arcp_acquire(dict->items[j - 1].value);
	}
	return ret;
}

struct adict *adict_cstrput(struct adict *dict, char *key, struct arcp_region *value) {
	size_t i, j;
	struct adict *ret;
	IF_BSEARCHCSTR(dict->items, dict->len, key, i) {
		ret = amalloc(ADICT_SIZE(dict->len));
		if(ret == NULL) {
			return NULL;
		}
		arcp_region_init(ret, (void (*)(struct arcp_region *)) adict_destroy);
		ret->len = dict->len;
		for(j = 0; j < i; j++) {
			ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j].key);
			ret->items[j].value = arcp_acquire(dict->items[j].value);
		}
		ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j].key);
		ret->items[j].value = arcp_acquire(value);
		for(j++; j < dict->len; j++) {
			ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j].key);
			ret->items[j].value = arcp_acquire(dict->items[j].value);
		}
		return ret;
	} ENDIF_BSEARCHCSTR;
	{
		struct astr *astrkey;
		ret = amalloc(ADICT_SIZE(dict->len + 1));
		if(ret == NULL) {
			return NULL;
		}
		astrkey = astr_cstrdup(key);
		if(astrkey == NULL) {
			afree(ret, ADICT_SIZE(dict->len + 1));
			return NULL;
		}
		arcp_region_init(ret, (void (*)(struct arcp_region *)) adict_destroy);
		ret->len = dict->len + 1;
		for(j = 0; j < i; j++) {
			ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j].key);
			ret->items[j].value = arcp_acquire(dict->items[j].value);
		}
		ret->items[j].key = astrkey;
		ret->items[j].value = arcp_acquire(value);
		for(j++; j < ret->len; j++) {
			ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j - 1].key);
			ret->items[j].value = arcp_acquire(dict->items[j - 1].value);
		}
		return ret;
	}
}

struct adict *adict_del(struct adict *dict, struct astr *key) {
	size_t i, j;
	struct adict *ret;
	IF_BSEARCH(dict->items, dict->len, key, i) {
		ret = amalloc(ADICT_SIZE(dict->len - 1));
		if(ret == NULL) {
			return NULL;
		}
		arcp_region_init(ret, (void (*)(struct arcp_region *)) adict_destroy);
		ret->len = dict->len - 1;
		for(j = 0; j < i; j++) {
			ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j].key);
			ret->items[j].value = arcp_acquire(dict->items[j].value);
		}
		for(; j < dict->len; j++) {
			ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j + 1].key);
			ret->items[j].value = arcp_acquire(dict->items[j + 1].value);
		}
		return ret;
	} ENDIF_BSEARCH;
	errno = EINVAL;
	return NULL;
}

struct adict *adict_cstrdel(struct adict *dict, char *key) {
	size_t i, j;
	struct adict *ret;
	IF_BSEARCHCSTR(dict->items, dict->len, key, i) {
		ret = amalloc(ADICT_SIZE(dict->len - 1));
		if(ret == NULL) {
			return NULL;
		}
		arcp_region_init(ret, (void (*)(struct arcp_region *)) adict_destroy);
		ret->len = dict->len - 1;
		for(j = 0; j < i; j++) {
			ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j].key);
			ret->items[j].value = arcp_acquire(dict->items[j].value);
		}
		for(; j < dict->len; j++) {
			ret->items[j].key = (struct astr *) arcp_acquire(dict->items[j + 1].key);
			ret->items[j].value = arcp_acquire(dict->items[j + 1].value);
		}
		return ret;
	} ENDIF_BSEARCHCSTR;
	errno = EINVAL;
	return NULL;
}
