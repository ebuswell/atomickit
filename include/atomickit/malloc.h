/** @file malloc.h
 * Atomic Malloc, Free and Realloc
 *
 * This provides a simple replacement of malloc, free, and realloc that
 * requires no locking. It is optimized for simplicity of code, and may not
 * function well in high-contention situations, though a few simple
 * improvements could remedy that. Currently, everything over 2048 bytes is
 * allocated and freed in page increments directly with mmap. Allocations
 * smaller than that are not freed back to the system, but cached permanently
 * by the allocator. There is zero overhead for all allocations.
 *
 * For simplicity, this is not quite a drop-in replacement for libc's malloc,
 * free, and realloc. Rather, I have offloaded the responsibility for knowing
 * the size of the object to the caller. Except in the case of strings, where
 * a size can easily be obtained with strlen, the size is almost always known
 * by the caller regardless. Although not quite trivial, it would be possible
 * to make these a drop-in replacement for malloc and co. by adding a
 * lock-free hash table for pointer values, e.g. as described in:
 * http://www.stanford.edu/class/ee380/Abstracts/070221_LockFreeHash.pdf
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
#ifndef ATOMICKIT_MALLOC_H
#define ATOMICKIT_MALLOC_H 1

#include <sys/types.h>
#include <stdbool.h>

/**
 * Allocate and return a region of memory of the specified size.
 *
 * @param size the desired size of the memory region.
 *
 * @returns a pointer to an allocated, but not initialized, memory
 * region, or NULL on error.
 */
void *amalloc(size_t size);

/**
 * Free a region previously allocated with `amalloc()`.
 *
 * @param ptr a pointer to the memory region to be freed.
 * @param size the size of the memory region.
 */
void afree(void *ptr, size_t size);

/**
 * Resize a region previously allocated with `amalloc()`.
 *
 * @param ptr a pointer to the memory region to be resized.
 * @param oldsize the currently allocated size of the memory region.
 * @param newsize the desired size of the memory region.
 *
 * @returns a pointer to the same, or a new memory region, or NULL on
 * error.  The new memory region will be filled with the old data,
 * possibly truncated by `newsize`. Any memory contents beyond what
 * was defined previously is undefined.
 */
void *arealloc(void *ptr, size_t oldsize, size_t newsize);

/**
 * Resize a region previously allocated with `amalloc()` if it can be
 * done in place.
 *
 * @param ptr a pointer to the memory region to be resized.
 * @param oldsize the currently allocated size of the memory region.
 * @param newsize the desired size of the memory region.
 *
 * @returns true if the memory region was resized, false if it could
 * not be resized in place.
 */
bool atryrealloc(void *ptr, size_t oldsize, size_t newsize);

#endif /* ! ATOMICKIT_MALLOC_H */
