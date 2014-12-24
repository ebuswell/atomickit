/** @file dict.h
 * Atomic Dictionary
 *
 * A copy-on-write dictionary, where the key is always an astr. This is less
 * limiting than it might be, as astr is designed to be a binary-safe string
 * (i.e. it can contain '\0'). As a result, any binary value can be used as
 * the key, provided it is first wrapped in an astr.
 *
 * Although NULL may be a valid value for a dictionary key, this is also
 * returned by fetch functions when the key cannot be found. To distinguish
 * between a NULL value and key not found, set errno prior to calling the
 * function and then check if it has been set to EINVAL.
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
#ifndef ATOMICKIT_DICT_H
#define ATOMICKIT_DICT_H 1

#include <stddef.h>
#include <atomickit/rcp.h>
#include <atomickit/string.h>

/**
 * A single entry in an Atomic Dictionary
 */
/* define this first so that adict can be an array of these */
struct adict_entry {
	struct astr *key;		/**< the key for this entry */
	struct arcp_region *value;	/**< the value for this entry */
};

/**
 * Atomic Dictionary
 *
 * The dictionary structure to be stored in an arcp_t container.
 */
struct adict {
	struct arcp_region;
	size_t len;			/**< The number of entries in this
					 *   dictionary */
	struct adict_entry items[];	/**< a sorted array of the entries in
					 *   this dictionary. */
};

/**
 * The overhead for a dictionary.
 */
#define ADICT_OVERHEAD (sizeof(struct adict))

/**
 * Get the size of a dictionary of the given number of entries.
 *
 * @param n the number of entries to get the size for.
 * @returns the size of a dictionary of the given number of entries.
 */
#define ADICT_SIZE(n) (ADICT_OVERHEAD + sizeof(struct adict_entry) * n)

/**
 * Create a duplicate of a given dictionary.
 *
 * @param dict the dictionary to duplicate.
 * @returns a duplicate of the dictionary, or NULL if a duplicate could not be
 * created.
 */
struct adict *adict_dup(struct adict *dict);

/**
 * Create an empty dictionary.
 *
 * @returns the new dictionary, or NULL if the dictionary could not be
 * created.
 */
struct adict *adict_create(void);

/**
 * Get the number of entries in the specified dictionary.
 *
 * @param the dictionary to get the number of entries for.
 * @returns the number of entries in the dictionary.
 */
static inline size_t adict_len(struct adict *dict) {
	return dict->len;
}

/**
 * Get the value for the given key.
 *
 * @param dict the dictionary from which to get the value.
 * @param key the key to look up.
 * @returns the value or if the value could not be found sets errno to EINVAL
 * and returns NULL.
 */
struct arcp_region *adict_get(struct adict *dict, struct astr *key);

/**
 * Get the value for the given key, using a cstr key.
 *
 * This is a convenience to not have to create an astr.
 *
 * @param dict the dictionary from which to get the value.
 * @param key the key to look up.
 * @returns the value or if the value could not be found sets errno to EINVAL
 * and returns NULL.
 */
struct arcp_region *adict_cstrget(struct adict *dict, char *key);

/**
 * Checks if a dictionary contains an entry for the specified key.
 *
 * @param dict the dictionary to check.
 * @param key the key to look up.
 * @returns true if the dictionary has the key, false otherwise.
 */
bool adict_has(struct adict *dict, struct astr *key);

/**
 * Checks if a dictionary contains an entry for the specified key, using a
 * cstr key.
 *
 * This is a convenience to not have to create an astr.
 *
 * @param dict the dictionary to check.
 * @param key the key to look up.
 * @returns true if the dictionary has the key, false otherwise.
 */
bool adict_cstrhas(struct adict *dict, char *key);

/**
 * Set the value for the given key.
 *
 * This will invalidate the passed in dictionary
 *
 * @param dict the dictionary for which to set the value.
 * @param key the key to look up.
 * @param value the value to set.
 * @returns the new dictionary.
 */
struct adict *adict_put(struct adict *dict,
			struct astr *key, struct arcp_region *value);

/**
 * Duplicate the dictionary and set the value for the given key.
 *
 * @param dict the dictionary for which to set the value.
 * @param key the key to look up.
 * @param value the value to set.
 * @returns the duplicated dictionary.
 */
struct adict *adict_dup_put(struct adict *dict,
			    struct astr *key, struct arcp_region *value);

/**
 * Set the value for the given key, using a cstr key.
 *
 * This is a convenience to not have to create an astr.
 *
 * This will invalidate the passed in dictionary
 *
 * @param dict the dictionary for which to set the value.
 * @param key the key to look up.
 * @param value the value to set.
 * @returns the new dictionary.
 */
struct adict *adict_cstrput(struct adict *dict,
			    char *key, struct arcp_region *value);

/**
 * Duplicate the dictionary and set the value for the given key, using a cstr
 * key.
 *
 * This is a convenience to not have to create an astr.
 *
 * This will invalidate the passed in dictionary
 *
 * @param dict the dictionary for which to set the value.
 * @param key the key to look up.
 * @param value the value to set.
 * @returns the duplicated dictionary.
 */
struct adict *adict_dup_cstrput(struct adict *dict,
				char *key, struct arcp_region *value);

/**
 * Create a dictionary with the value for the given key.
 *
 * @param key the key to set.
 * @param value the value to set.
 * @returns the new dictionary.
 */
struct adict *adict_create_put(struct astr *key, struct arcp_region *value);

/**
 * Create a dictionary with the value for the given key, using a cstr key.
 *
 * @param key the key to set.
 * @param value the value to set.
 * @returns the new dictionary.
 */
struct adict *adict_create_cstrput(char *key, struct arcp_region *value);

/**
 * Delete the dictionary entry for the given key.
 *
 * This will invalidate the passed in dictionary
 *
 * @param dict the dictionary for which to delete the value.
 * @param key the key to look up.
 * @returns the new dictionary, or NULL and set errno to EINVAL if there is no
 * entry for the key.
 */
struct adict *adict_del(struct adict *dict, struct astr *key);

/**
 * Duplicate the dictionary and delete the dictionary entry for the given key.
 *
 * @param dict the dictionary for which to delete the value.
 * @param key the key to look up.
 * @returns the duplicated dictionary, or NULL and set errno to EINVAL if
 * there is no entry for the key.
 */
struct adict *adict_dup_del(struct adict *dict, struct astr *key);

/**
 * Delete the dictionary entry for the given key, using a cstr key.
 *
 * This is a convenience to not have to create an astr.
 *
 * This will invalidate the passed in dictionary
 *
 * @param dict the dictionary for which to delete the value.
 * @param key the key to look up.
 * @returns the new dictionary, or NULL and set errno to EINVAL if there is no
 * entry for the key.
 */
struct adict *adict_cstrdel(struct adict *dict, char *key);

/**
 * Duplicate the dictionary and delete the dictionary entry for the given key,
 * using a cstr key.
 *
 * This is a convenience to not have to create an astr.
 *
 * @param dict the dictionary for which to delete the value.
 * @param key the key to look up.
 * @returns the duplicated dictionary, or NULL and set errno to EINVAL if
 * there is no entry for the key.
 */
struct adict *adict_dup_cstrdel(struct adict *dict, char *key);

#endif /* ! ATOMICKIT_DICT_H */
