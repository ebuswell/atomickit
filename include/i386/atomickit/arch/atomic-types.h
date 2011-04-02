/*
 * atomic-types.h
 *
 * Copyright 2011 Evan Buswell
 * 
 * This file is a redaction of arch/x86/include/atomic*.h and some
 * other files from the Linux Kernel.  In the future, someone should
 * make similar files for other architectures.  For now, though, we
 * depend on x86.  See the Linux documentation for more information.
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
#ifndef _ATOMICKIT_ARCH_ATOMIC_TYPES_H
#define _ATOMICKIT_ARCH_ATOMIC_TYPES_H 1

typedef struct {
	volatile int counter;
} atomic_t;

typedef struct {
	volatile float counter;
} atomic_float_t;

#ifdef __LP64__

typedef struct {
	volatile long counter;
} atomic64_t;

#endif /* #ifdef __LP64__ */

#endif /* #ifndef _ATOMICKIT_ARCH_ATOMIC_TYPES_H */
