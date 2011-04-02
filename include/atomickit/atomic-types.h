/*
 * atomic-types.h
 *
 * Copyright 2011 Evan Buswell
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

#ifndef _ATOMICKIT_ATOMIC_TYPES_H
#define _ATOMICKIT_ATOMIC_TYPES_H 1

#include <atomickit/arch/atomic-types.h>

#ifdef __LP64__

typedef atomic64_t atomic_ptr_t;
typedef atomic64_t atomic_long_t;

#else /* #ifdef __LP64__ */

typedef atomic_t atomic_ptr_t;
typedef atomic_t atomic_long_t;

#endif /* #ifdef __LP64__ */

#endif /* #ifndef _ATOMICKIT_ATOMIC_TYPES_H */
