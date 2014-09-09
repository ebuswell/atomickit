/*
 * dict.c
 *
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
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "atomickit/rcp.h"
#include "atomickit/string.h"
#include "atomickit/malloc.h"
#include "atomickit/dict.h"

static void adict_destroy(struct adict *dict) {
	size_t i;
	/* release each key and value */
	for(i = 0; i < dict->len; i++) {
		arcp_release(dict->items[i].key);
		arcp_release(dict->items[i].value);
	}
	/* free the memory */
	afree(dict, ADICT_SIZE(dict->len));
}

struct adict *adict_dup(struct adict *dict) {
	struct adict *ret;
	size_t i;
	/* try to allocate memory for the duplicate dictionary */
	ret = amalloc(ADICT_SIZE(dict->len));
	if(ret == NULL) {
		return NULL;
	}
	/* acquire values and copy them to the new dictionary */
	for(i = 0; i < dict->len; i++) {
		ret->items[i].key = arcp_acquire(dict->items[i].key);
		ret->items[i].value = arcp_acquire(dict->items[i].value);
	}
	/* set up dictionary */
	ret->len = dict->len;
	arcp_region_init(ret, (arcp_destroy_f) adict_destroy);
	return ret;
}

struct adict *adict_create() {
	struct adict *ret;
	/* try to allocate memory for the dictionary */
	ret = amalloc(ADICT_OVERHEAD);
	if(ret == NULL) {
		return NULL;
	}
	/* set up dictionary */
	ret->len = 0;
	arcp_region_init(ret, (arcp_destroy_f) adict_destroy);
	return ret;
}

/* IF_BSEARCH and ENDIF_BSEARCH macros for internal use.
 * list - the dictionary in which to search
 * nitems - the number of items in the list
 * key - the key to search for
 * retidx - this will be set to the index at which the key was found, or if
 * the key was not found, the place at which the key should be put to keep the
 * dictionary sorted */
#define IF_BSEARCH(list, nitems, key, retidx) do {		\
	size_t __l; /* lower limit */				\
	size_t __u; /* upper limit */				\
	(retidx) = __l = 0;					\
	__u = (nitems);						\
	while(__l < __u) {					\
		int __r;					\
		(retidx) = (__l + __u) / 2;			\
		__r = astr_cmp((key), (list)[retidx].key);	\
		if(__r < 0) {					\
			__u = (retidx);				\
		} else if(__r > 0) {				\
			__l = ++(retidx);			\
		} else

#define ENDIF_BSEARCH \
	}             \
} while(0)

/* IF_BSEARCHCSTR and ENDIF_BSEARCHCSTR macros for internal use.
 * These are the same as for IF_BSEARCH except that key is a cstr instead of
 * an astr.
 * list - the dictionary in which to search
 * nitems - the number of items in the list
 * key - the key to search for
 * retidx - this will be set to the index at which the key was found, or if
 * the key was not found, the place at which the key should be put to keep the
 * dictionary sorted */
#define IF_BSEARCHCSTR(list, nitems, key, retidx) do {		\
	size_t __l; /* lower limit */				\
	size_t __u; /* upper limit */				\
	(retidx) = __l = 0;					\
	__u = (nitems);						\
	while(__l < __u) {					\
		int __r;					\
		(retidx) = (__l + __u) / 2;			\
		__r = astr_cstrcmp((list)[retidx].key, (key));	\
		if(__r > 0) {					\
			__u = (retidx);				\
		} else if(__r < 0) {				\
			__l = ++(retidx);			\
		} else

#define ENDIF_BSEARCHCSTR \
	}                 \
} while(0)

struct arcp_region *adict_get(struct adict *dict, struct astr *key) {
	size_t i;
	size_t len;
	len = dict->len;
	/* search for key */
	IF_BSEARCH(dict->items, len, key, i) {
		/* key is present; return value */
		return arcp_acquire(dict->items[i].value);
	} ENDIF_BSEARCH;
	/* error: invalid key */
	errno = EINVAL;
	return NULL;
}

struct arcp_region *adict_cstrget(struct adict *dict, char *key) {
	size_t i;
	size_t len;
	len = dict->len;
	/* search for key */
	IF_BSEARCHCSTR(dict->items, len, key, i) {
		/* key is present; return value */
		return arcp_acquire(dict->items[i].value);
	} ENDIF_BSEARCHCSTR;
	/* error invalid key */
	errno = EINVAL;
	return NULL;
}

bool adict_has(struct adict *dict, struct astr *key) {
	size_t i;
	size_t len;
	len = dict->len;
	/* search for key */
	IF_BSEARCH(dict->items, len, key, i) {
		/* key is present */
		return true;
	} ENDIF_BSEARCH;
	/* key is not present */
	return false;
}

bool adict_cstrhas(struct adict *dict, char *key) {
	size_t i;
	size_t len;
	len = dict->len;
	/* search for key */
	IF_BSEARCHCSTR(dict->items, len, key, i) {
		/* key is present */
		return true;
	} ENDIF_BSEARCHCSTR;
	/* key is not present */
	return false;
}

struct adict *adict_put(struct adict *dict, struct astr *key,
                        struct arcp_region *value) {
	size_t i;
	size_t len;
	len = dict->len;
	/* search for key */
	IF_BSEARCH(dict->items, len, key, i) {
		/* key is present, set new value */
		arcp_release(dict->items[i].value);
		dict->items[i].value = arcp_acquire(value);
		return dict;
	} ENDIF_BSEARCH;
	/* key is not present, add new entry; reallocate in place or
 	 * allocate and copy */
	if(atryrealloc(dict, ADICT_SIZE(len), ADICT_SIZE(len + 1))) {
		memmove(&dict->items[i + 1], &dict->items[i],
		        sizeof(struct adict_entry) * (len - i));
	} else {
		struct adict *new_dict = amalloc(ADICT_SIZE(len + 1));
		if(new_dict == NULL) {
			return NULL;
		}
		memcpy(new_dict, dict, ADICT_SIZE(i));
		memcpy(&new_dict->items[i + 1], &dict->items[i],
		       sizeof(struct adict_entry) * (len - i));
		afree(dict, ADICT_SIZE(len));
		dict = new_dict;
	}
	/* add new entry */
	dict->items[i].key = arcp_acquire(key);
	dict->items[i].value = arcp_acquire(value);
	/* set up dictionary */
	dict->len = len + 1;
	return dict;
}

struct adict *adict_dup_put(struct adict *dict, struct astr *key,
                            struct arcp_region *value) {
	size_t i, j;
	struct adict *new_dict;
	size_t len;
	len = dict->len;
	/* search for key */
	IF_BSEARCH(dict->items, len, key, i) {
		/* key is present */
		/* allocate memory for the new dictionary */
		new_dict = amalloc(ADICT_SIZE(len));
		if(new_dict == NULL) {
			return NULL;
		}
		/* acquire items and copy them over, leaving a hole for the
 		 * new value. */
		for(j = 0; j < i; j++) {
			new_dict->items[j].key =
				arcp_acquire(dict->items[j].key);
			new_dict->items[j].value =
				arcp_acquire(dict->items[j].value);
		}
		for(j++; j < len; j++) {
			new_dict->items[j].key =
				arcp_acquire(dict->items[j].key);
			new_dict->items[j].value =
				arcp_acquire(dict->items[j].value);
		}
		/* set up length */
		new_dict->len = len;
		goto set;
	} ENDIF_BSEARCH;
	/* key is not present; insert new key at i */
	/* allocate memory for the new dictionary */
	new_dict = amalloc(ADICT_SIZE(dict->len + 1));
	if(new_dict == NULL) {
		return NULL;
	}
	/* acquire items and copy them over, leaving a hole for the new
 	 * value */
	for(j = 0; j < i; j++) {
		new_dict->items[j].key = arcp_acquire(dict->items[j].key);
		new_dict->items[j].value = arcp_acquire(dict->items[j].value);
	}
	for(; j < len; j++) {
		new_dict->items[j].key = arcp_acquire(dict->items[j].key);
		new_dict->items[j].value = arcp_acquire(dict->items[j].value);
	}
	/* set up length */
	new_dict->len = len + 1;
set:
	/* acquire passed-in key and value */
	new_dict->items[i].key = arcp_acquire(key);
	new_dict->items[i].value = arcp_acquire(value);
	/* set up dictionary */
	arcp_region_init(new_dict, (arcp_destroy_f) adict_destroy);
	return new_dict;
}

struct adict *adict_cstrput(struct adict *dict, char *key,
                            struct arcp_region *value) {
	size_t i;
	size_t len;
	len = dict->len;
	/* search for key */
	IF_BSEARCHCSTR(dict->items, len, key, i) {
		/* key is present, set new value */
		arcp_release(dict->items[i].value);
		dict->items[i].value = arcp_acquire(value);
		return dict;
	} ENDIF_BSEARCHCSTR;
	{
		/* key is not present, insert key at i */
		/* create a new key to be stored here */
		struct astr *astrkey = astr_cstrdup(key);
		if(astrkey == NULL) {
			return NULL;
		}
		/* reallocate in place or allocate and copy */
		if(atryrealloc(dict, ADICT_SIZE(len), ADICT_SIZE(len + 1))) {
			memmove(&dict->items[i + 1], &dict->items[i],
		        	sizeof(struct adict_entry) * (len - i));
		} else {
			struct adict *new_dict = amalloc(ADICT_SIZE(len + 1));
			if(new_dict == NULL) {
				arcp_release(astrkey);
				return NULL;
			}
			memcpy(new_dict, dict, ADICT_SIZE(i));
			memcpy(&new_dict->items[i + 1], &dict->items[i],
		       	       sizeof(struct adict_entry) * (len - i));
			afree(dict, ADICT_SIZE(len));
			dict = new_dict;
		}
		/* set new entry */
		dict->items[i].key = astrkey;
		dict->items[i].value = arcp_acquire(value);
		/* set up dictionary */
		dict->len = len + 1;
		return dict;
	}
}

struct adict *adict_dup_cstrput(struct adict *dict, char *key,
                                struct arcp_region *value) {
	size_t i, j;
	struct adict *new_dict;
	size_t len;
	len = dict->len;
	/* search for key */
	IF_BSEARCHCSTR(dict->items, len, key, i) {
		/* key present, duplicate and set key entry to value */
		/* allocate new dictionary */
		new_dict = amalloc(ADICT_SIZE(len));
		if(new_dict == NULL) {
			return NULL;
		}
		/* acquire items and copy them over */
		for(j = 0; j < i; j++) {
			new_dict->items[j].key =
				arcp_acquire(dict->items[j].key);
			new_dict->items[j].value =
				arcp_acquire(dict->items[j].value);
		}
		for(j++; j < len; j++) {
			new_dict->items[j].key =
				arcp_acquire(dict->items[j].key);
			new_dict->items[j].value =
				arcp_acquire(dict->items[j].value);
		}
		/* set new key */
		new_dict->items[i].key = arcp_acquire(dict->items[i].key);
		/* set up length */
		new_dict->len = len;
		goto set;
	} ENDIF_BSEARCHCSTR;
	{
		/* key not present, insert key at i */
		/* create the key */
		struct astr *astrkey = astr_cstrdup(key);
		if(astrkey == NULL) {
			return NULL;
		}
		/* allocate new dictionary */
		new_dict = amalloc(ADICT_SIZE(dict->len + 1));
		if(new_dict == NULL) {
			arcp_release(astrkey);
			return NULL;
		}
		/* acquire items and copy them over, leaving a hole for the
 		 * new value */
		for(j = 0; j < i; j++) {
			new_dict->items[j].key =
				arcp_acquire(dict->items[j].key);
			new_dict->items[j].value =
				arcp_acquire(dict->items[j].value);
		}
		for(; j < len; j++) {
			new_dict->items[j].key =
				arcp_acquire(dict->items[j].key);
			new_dict->items[j].value =
				arcp_acquire(dict->items[j].value);
		}
		/* set new key */
		new_dict->items[i].key = astrkey;
		/* set up length */
		new_dict->len = len + 1;
	}
set:
	/* set new value */
	new_dict->items[i].value = arcp_acquire(value);
	/* set up dictionary */
	arcp_region_init(new_dict, (arcp_destroy_f) adict_destroy);
	return new_dict;
}

struct adict *adict_create_put(struct astr *key,
                               struct arcp_region *value) {
	struct adict *new_dict;
	/* allocate memory for the new dictionary */
	new_dict = amalloc(ADICT_SIZE(1));
	if(new_dict == NULL) {
		return NULL;
	}
	/* acquire passed-in key and value */
	new_dict->items[0].key = arcp_acquire(key);
	new_dict->items[0].value = arcp_acquire(value);
	/* set up dictionary */
	new_dict->len = 1;
	arcp_region_init(new_dict, (arcp_destroy_f) adict_destroy);
	return new_dict;
}

struct adict *adict_create_cstrput(char *key,
                                   struct arcp_region *value) {
	struct adict *new_dict;
	struct astr *astrkey;
	astrkey = astr_cstrdup(key);
	if(astrkey == NULL) {
		return NULL;
	}
	new_dict = adict_create_put(astrkey, value);
	arcp_release(astrkey); /* we want to release this whether or not
				  there's an error. */
	return new_dict;
}

struct adict *adict_del(struct adict *dict, struct astr *key) {
	size_t i;
	size_t len;
	struct adict_entry deleted;
	struct adict_entry last;
	len = dict->len;
	/* search for key */
	IF_BSEARCH(dict->items, len, key, i) {
		/* store key and value to release only after completing the
 		 * deletion */
		deleted.key = dict->items[i].key;
		deleted.value = dict->items[i].value;
		/* store last value as it's always deleted */
		last.key = dict->items[len - 1].key;
		last.value = dict->items[len - 1].value;
		/* reallocate or copy */
		if(atryrealloc(dict, ADICT_SIZE(len), ADICT_SIZE(len - 1))) {
			if(deleted.key != last.key) {
				memmove(&dict->items[i],
				        &dict->items[i + 1],
			                sizeof(struct adict_entry)
				        * ((len - 1) - (i + 1)));
				dict->items[len - 2].key = last.key;
				dict->items[len - 2].value = last.value;
			}
		} else {
			struct adict *new_dict;
			new_dict = amalloc(ADICT_SIZE(len - 1));
			if(new_dict == NULL) {
				return NULL;
			}
			memcpy(new_dict, dict, ADICT_SIZE(i));
			memcpy(&new_dict->items[i], &dict->items[i + 1],
			       sizeof(struct adict_entry) * (len - (i + 1)));
			afree(dict, ADICT_SIZE(len));
			dict = new_dict;
		}
		/* release deleted key */
		arcp_release(deleted.key);
		arcp_release(deleted.value);
		/* set up length */
		dict->len = len - 1;
		return dict;
	} ENDIF_BSEARCH;
	/* error: request deletion of invalid key */
	errno = EINVAL;
	return NULL;
}

struct adict *adict_dup_del(struct adict *dict, struct astr *key) {
	size_t i, j;
	size_t len;
	struct adict *new_dict;
	len = dict->len;
	/* search for key */
	IF_BSEARCH(dict->items, len, key, i) {
		/* allocate new dictionary */
		new_dict = amalloc(ADICT_SIZE(len - 1));
		if(new_dict == NULL) {
			return NULL;
		}
		/* acquire items and copy them over, leaving out the deleted
 		 * value */
		for(j = 0; j < i; j++) {
			new_dict->items[j].key =
				arcp_acquire(dict->items[j].key);
			new_dict->items[j].value =
				arcp_acquire(dict->items[j].value);
		}
		for(; j + 1 < len; j++) {
			new_dict->items[j].key =
				arcp_acquire(dict->items[j + 1].key);
			new_dict->items[j].value =
				arcp_acquire(dict->items[j + 1].value);
		}
		/* set up dictionary */
		new_dict->len = len - 1;
		arcp_region_init(
			new_dict, (arcp_destroy_f) adict_destroy);
		return new_dict;
	} ENDIF_BSEARCH;
	/* error: request deletion of invalid key */
	errno = EINVAL;
	return NULL;
}

struct adict *adict_cstrdel(struct adict *dict, char *key) {
	size_t i;
	size_t len;
	struct adict_entry deleted;
	struct adict_entry last;
	len = dict->len;
	/* search for key */
	IF_BSEARCHCSTR(dict->items, len, key, i) {
		/* store key and value to release only after completing the
 		 * deletion */
		deleted.key = dict->items[i].key;
		deleted.value = dict->items[i].value;
		/* store last value as it's always deleted */
		last.key = dict->items[len - 1].key;
		last.value = dict->items[len - 1].value;
		/* reallocate or copy */
		if(atryrealloc(dict, ADICT_SIZE(len), ADICT_SIZE(len - 1))) {
			if(deleted.key != last.key) {
				memmove(&dict->items[i],
				        &dict->items[i + 1],
			                sizeof(struct adict_entry)
				        * ((len - 1) - (i + 1)));
				dict->items[len - 2].key = last.key;
				dict->items[len - 2].value = last.value;
			}
		} else {
			struct adict *new_dict;
			new_dict = amalloc(ADICT_SIZE(len - 1));
			if(new_dict == NULL) {
				return NULL;
			}
			memcpy(new_dict, dict, ADICT_SIZE(i));
			memcpy(&new_dict->items[i], &dict->items[i + 1],
			       sizeof(struct adict_entry) * (len - (i + 1)));
			afree(dict, ADICT_SIZE(len));
			dict = new_dict;
		}
		/* release deleted key */
		arcp_release(deleted.key);
		arcp_release(deleted.value);
		/* set up length */
		dict->len = len - 1;
		return dict;
	} ENDIF_BSEARCHCSTR;
	/* error: request deletion of invalid key */
	errno = EINVAL;
	return NULL;
}

struct adict *adict_dup_cstrdel(struct adict *dict, char *key) {
	size_t i, j;
	size_t len;
	struct adict *new_dict;
	len = dict->len;
	/* search for key */
	IF_BSEARCHCSTR(dict->items, len, key, i) {
		/* allocate new dictionary */
		new_dict = amalloc(ADICT_SIZE(len - 1));
		if(new_dict == NULL) {
			return NULL;
		}
		/* acquire items and copy them over, leaving out the deleted
 		 * value */
		for(j = 0; j < i; j++) {
			new_dict->items[j].key =
				arcp_acquire(dict->items[j].key);
			new_dict->items[j].value =
				arcp_acquire(dict->items[j].value);
		}
		for(; j + 1 < len; j++) {
			new_dict->items[j].key =
				arcp_acquire(dict->items[j + 1].key);
			new_dict->items[j].value =
				arcp_acquire(dict->items[j + 1].value);
		}
		/* set up dictionary */
		new_dict->len = len - 1;
		arcp_region_init(new_dict, (arcp_destroy_f) adict_destroy);
		return new_dict;
	} ENDIF_BSEARCHCSTR;
	/* error: request deletion of invalid key */
	errno = EINVAL;
	return NULL;
}
