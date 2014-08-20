/** @file dict.h
 * Atomic Dictionary
 *
 * A copy-on-write dictionary
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
#ifndef ATOMICKIT_DICT_H
#define ATOMICKIT_DICT_H 1

#include <stddef.h>
#include <atomickit/rcp.h>
#include <atomickit/string.h>

struct adict_entry {
	struct astr *key;
	struct arcp_region *value;
};

struct adict {
	struct arcp_region;
	size_t len;
	struct adict_entry items[];
};

#define ADICT_OVERHEAD (sizeof(struct adict))

#define ADICT_SIZE(n) (ADICT_OVERHEAD + sizeof(struct adict_entry) * n)

struct adict *adict_dup(struct adict *dict);
struct adict *adict_create(void);

static inline size_t adict_len(struct adict *dict) {
	return dict->len;
}

struct arcp_region *adict_get(struct adict *dict, struct astr *key);
struct arcp_region *adict_cstrget(struct adict *dict, char *key);
bool adict_has(struct adict *dict, struct astr *key);
bool adict_cstrhas(struct adict *dict, char *key);
struct adict *adict_put(struct adict *dict, struct astr *key, struct arcp_region *value);
struct adict *adict_dup_put(struct adict *dict, struct astr *key, struct arcp_region *value);
struct adict *adict_cstrput(struct adict *dict, char *key, struct arcp_region *value);
struct adict *adict_dup_cstrput(struct adict *dict, char *key, struct arcp_region *value);
struct adict *adict_del(struct adict *dict, struct astr *key);
struct adict *adict_dup_del(struct adict *dict, struct astr *key);
struct adict *adict_cstrdel(struct adict *dict, char *key);
struct adict *adict_dup_cstrdel(struct adict *dict, char *key);

#endif /* ! ATOMICKIT_DICT_H */
