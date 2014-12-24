/** @file array.h
 * Atomic Array
 *
 * A copy-on-write array that can be used as a list or a set.
 *
 * The array code itself contains support for copy-on-write and and deals
 * correctly with the references to the array. Much of the code itself,
 * however, is not thread safe. Nor is copy-on-write exactly automatic. In
 * particular, all of the functions which write to the array without
 * duplicating the array assume that the thread calling these functions is the
 * sole owner of the array. A typical write operation, therefore, would
 * involve first calling one of the dup functions, doing whatever other
 * writing needs to be done, and finally storing the result in an arcp_t
 * container with arcp_cas. After that, the thread would have to release
 * *both* the duplicate and the original, which wouldn't have been written to
 * at all.
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
#ifndef ATOMICKIT_ARRAY_H
#define ATOMICKIT_ARRAY_H 1

#include <stddef.h>
#include <atomickit/rcp.h>

/**
 * Atomic Array
 *
 * The array structure to be stored in an arcp_t container.
 */
struct aary {
	struct arcp_region;
	size_t len;			/**< the length of the array */
	struct arcp_region *items[];	/**< the array of items */
};

/**
 * The overhead for an array.
 */
#define AARY_OVERHEAD (sizeof(struct aary))

/**
 * Get the size of an array for the given length.
 *
 * @param n the length to get the size for
 * @returns the size of an array for the given length
 */
#define AARY_SIZE(n) (AARY_OVERHEAD + sizeof(struct arcp_region *) * (n))

/**
 * Create a duplicate of a given array.
 *
 * @param array the array to duplicate.
 * @returns a duplicate of the array, or NULL if a duplicate could not be
 * created.
 */
struct aary *aary_dup(struct aary *array);

/**
 * Create an array of the given length.
 *
 * The values will be initialized to NULL.
 *
 * @param len the desired length of the array.
 * @returns the new array, or NULL if the array could not be created.
 */
struct aary *aary_create(size_t len);

/**
 * Get the length of the given array.
 *
 * @param array the array to get the length of.
 * @returns the length of the given array.
 */
static inline size_t aary_len(struct aary *array) {
	return array->len;
}

/**
 * Load a specific value from an index in the array.
 *
 * @param Array the array from which to load the value.
 * @param i The index in the array from which to load the value.
 * @returns The value of the array at the specified index.
 */
static inline struct arcp_region *aary_load(struct aary *array, size_t i) {
	return arcp_acquire(array->items[i]);
}

/**
 * Load a specific value from an index in the array without incrementing its
 * reference count.
 *
 * @param Array the array from which to load the value.
 * @param i The index in the array from which to load the value.
 * @returns The value of the array at the specified index.
 */
static inline struct arcp_region *aary_load_phantom(struct aary *array,
                                                    size_t i) {
	return array->items[i];
}

/**
 * Load the last value from the array.
 *
 * The array must have at least one value.
 *
 * @param Array the array from which to load the value.
 * @returns The value of the last item in the array.
 */
static inline struct arcp_region *aary_last(struct aary *array) {
	return arcp_acquire(array->items[array->len - 1]);
}

/**
 * Load the last value from the array without incrementing its reference
 * count.
 *
 * The array must have at least one value.
 *
 * @param Array the array from which to load the value.
 * @returns The value of the last item in the array.
 */
static inline struct arcp_region *aary_last_phantom(struct aary *array) {
	return array->items[array->len - 1];
}

/**
 * Load the first value from the array.
 *
 * The array must have at least one value.
 *
 * @param Array the array from which to load the value.
 * @returns The value of the last item in the array.
 */
static inline struct arcp_region *aary_first(struct aary *array) {
	return arcp_acquire(array->items[0]);
}

/**
 * Load the first value from the array without incrementing its reference
 * count.
 *
 * The array must have at least one value.
 *
 * @param Array the array from which to load the value.
 * @returns The value of the last item in the array.
 */
static inline struct arcp_region *aary_first_phantom(struct aary *array) {
	return array->items[0];
}

/**
 * Store the specified value at a specific index in the array.
 *
 * @param array the array in whih to store the value.
 * @param i the index into the array at which to store the value.
 * @param region the region to store in the array.
 */
static inline void aary_store(struct aary *array, size_t i,
			      struct arcp_region *region) {
	arcp_release(array->items[i]);
	array->items[i] = arcp_acquire(region);
}

/**
 * Store the specified value at the beginning of the array.
 *
 * The array must be at least one item long.
 *
 * @param array the array in whih to store the value.
 * @param region the region to store in the array.
 */
static inline void aary_storefirst(struct aary *array,
				   struct arcp_region *region) {
	arcp_release(array->items[0]);
	array->items[0] = arcp_acquire(region);
}

/**
 * Store the specified value at the end of the array.
 *
 * The array must be at least one item long.
 *
 * @param array the array in whih to store the value.
 * @param region the region to store in the array.
 */
static inline void aary_storelast(struct aary *array,
				  struct arcp_region *region) {
	arcp_release(array->items[array->len - 1]);
	array->items[array->len - 1] = arcp_acquire(region);
}


/**
 * Insert the specified value at a specific index in the array, shifting
 * values upward and increasing the length by one.
 *
 * Invalidates the passed in array.
 *
 * @param array the array in which to store the value.
 * @param i the index into the array at which to store the value.
 * @param region the region to store in the array.
 * @returns the new array.
 */
struct aary *aary_insert(struct aary *array, size_t i,
			 struct arcp_region *region);

/**
 * Duplicates the array and inserts the specified value at a specific index in
 * the array, shifting values upward and increasing the length by one.
 *
 * @param array the array in which to store the value.
 * @param i the index into the array at which to store the value.
 * @param region the region to store in the array.
 * @returns the duplicated array.
 */
struct aary *aary_dup_insert(struct aary *array, size_t i,
			     struct arcp_region *region);

/**
 * Removes the value at a specified index in the array, shifting values
 * downward and decreasing the length by one.
 *
 * Invalidates the passed in array.
 *
 * @param array the array from which to remove the value.
 * @param i the index into the array at which to remove the value.
 * @returns the new array.
 */
struct aary *aary_remove(struct aary *array, size_t i);

/**
 * Duplicates the array and removes the value at a specified index in the
 * array, shifting values downward and decreasing the length by one.
 *
 * @param array the array from which to remove the value.
 * @param i the index into the array at which to remove the value.
 * @returns the duplicated array.
 */
struct aary *aary_dup_remove(struct aary *array, size_t i);

/**
 * Append the specified value to the end of the array, increasing the length
 * by one.
 *
 * Invalidates the passed in array.
 *
 * @param array the array in which to store the value.
 * @param region the region to store in the array.
 * @returns the new array.
 */
struct aary *aary_append(struct aary *array, struct arcp_region *region);

/**
 * Duplicates the array and appends the specified value to the end of the
 * array, increasing the length by one.
 *
 * @param array the array in which to store the value.
 * @param region the region to store in the array.
 * @returns the duplicated array.
 */
struct aary *aary_dup_append(struct aary *array, struct arcp_region *region);

/**
 * Removes the value at the end of the array, decreasing the length by one.
 *
 * Invalidates the passed in array.
 *
 * @param array the array from which to remove the value.
 * @returns the new array.
 */
struct aary *aary_pop(struct aary *array);

/**
 * Duplicates the array and removes the value at the end of the array,
 * decreasing the length by one.
 *
 * @param array the array from which to remove the value.
 * @returns the duplicated array.
 */
struct aary *aary_dup_pop(struct aary *array);

/**
 * Prepend the specified value to the beginning of the array, shifting values
 * upward and increasing the length by one.
 *
 * Invalidates the passed in array.
 *
 * @param array the array in which to store the value.
 * @param region the region to store in the array.
 * @returns the new array.
 */
struct aary *aary_prepend(struct aary *array, struct arcp_region *region);

/**
 * Duplicates the array and prepends the specified value to the beginning of
 * the array, shifting values upward and increasing the length by one.
 *
 * @param array the array in which to store the value.
 * @param region the region to store in the array.
 * @returns the duplicated array.
 */
struct aary *aary_dup_prepend(struct aary *array,
			      struct arcp_region *region);

/**
 * Removes the value at the beginning of the array, shifting values downward
 * and decreasing the length by one.
 *
 * Invalidates the passed in array.
 *
 * @param array the array from which to remove the value.
 * @returns the new array.
 */
struct aary *aary_shift(struct aary *array);

/**
 * Duplicates the array and removes the value at the beginning of the array,
 * shifting values downward and decreasing the length by one.
 *
 * @param array the array from which to remove the value.
 * @returns the duplicated array.
 */
struct aary *aary_dup_shift(struct aary *array);

/* array/range equivalents of above functions */
/* struct aary *aary_storeary(struct aary *array1, size_t i,
 * 			   struct aary *array2); */
/* struct aary *aary_dup_storeary(struct aary *array1, size_t i,
 * 			       struct aary *array2); */
/* struct aary *aary_storeendary(struct aary *array1,
 * 			      struct aary *array2); */
/* struct aary *aary_dup_storeendary(struct aary *array1,
 * 				  struct aary *array2); */
/* struct aary *aary_storestartary(struct aary *array1,
 * 				struct aary *array2); */
/* struct aary *aary_dup_storestartary(struct aary *array1,
 * 				    struct aary *array2); */
/* struct aary *aary_insertary(struct aary *array1, size_t i,
 * 			    struct aary *array2); */
/* struct aary *aary_dup_insertary(struct aary *array1, size_t i,
 * 				struct aary *array2); */
/* struct aary *aary_removerange(struct aary *array1, size_t i,
 * 			      size_t len); */
/* struct aary *aary_dup_removerange(struct aary *array1, size_t i,
 * 				  size_t len); */
/* struct aary *aary_poprange(struct aary *array1, size_t len); */
/* struct aary *aary_dup_poprange(struct aary *array1, size_t len); */
/* struct aary *aary_shiftrange(struct aary *array1, size_t len); */
/* struct aary *aary_dup_shiftrange(struct aary *array1, size_t len); */

/**
 * Determines if the two arrays are of equal length with identical members at
 * identical indices.
 *
 * @param array1 one of the arrays for which to determine equality.
 * @param array2 the other of the arrays for which to determine equality.
 * @returns true if the arrays are equal, false otherwise.
 */
bool aary_equal(struct aary *array1, struct aary *array2);

/**
 * Sorts an array in-place by the pointer value of the region.
 *
 * This is useful in doing searches for specific pointers.
 * @param array the array to sort.
 */
void aary_sortx(struct aary *array);

/**
 * Sorts an array in-place via the specified function.
 *
 * @param array the array to sort.
 * @param compar the comparison function, which will return a number less
 * than, equal to, or greater than zero if the first region is less than,
 * equal to, or greater than the second.
 */
void aary_sort(struct aary *array, int (*compar)(const struct arcp_region *,
						 const struct arcp_region *));

/**
 * Sorts an array in-place via the specified function.
 *
 * The only difference from aary_sort is that the specified function here
 * accepts an extra argument.
 *
 * @param array the array to sort.
 * @param compar the comparison function, which will return a number less
 * than, equal to, or greater than zero if the first region is less than,
 * equal to, or greater than the second.
 * @param arg an arbitrary argument to pass to the comparison function.
 */
void aary_sort_r(struct aary *array, int (*compar)(const struct arcp_region *,
						   const struct arcp_region *,
						   void *arg),
		 void *arg);

/**
 * Reverses the order of the elements of the array in-place.
 *
 * @param array the array whose elements will be reversed.
 */
void aary_reverse(struct aary *array);

/*****************
 * Set Functions *
 *****************/

/**
 * Add the region to the array if it is not already present in the array.
 *
 * The array must be sorted by arcp_region pointer before calling this
 * function. The added item will be added such that the sorting of the array
 * is preserved.
 *
 * The passed in array is invalidated.
 *
 * @param array the array to add the region to.
 * @param region the region to add to the array.
 * @returns the new array.
 */
struct aary *aary_set_add(struct aary *array, struct arcp_region *region);

/**
 * Duplicate the array and add the region to the duplicated array if it is not
 * already present
 *
 * The array must be sorted by arcp_region pointer before calling this
 * function. The added item will be added such that the sorting of the array
 * is preserved.
 *
 * @param array the array to add the region to.
 * @param region the region to add to the array.
 * @returns the duplicated array.
 */
struct aary *aary_dup_set_add(struct aary *array, struct arcp_region *region);

/**
 * Remove the region from the array if it is present in the array.
 *
 * The array must be sorted by arcp_region pointer before calling this
 * function. The removed item will be removed such that the sorting of the
 * array is preserved.
 *
 * The passed in array is invalidated.
 *
 * @param array the array to remove the region from.
 * @param region the region to remove from the array.
 * @returns the new array.
 */
struct aary *aary_set_remove(struct aary *array, struct arcp_region *region);

/**
 * Duplicate the array and remove the region from the duplicated array if it
 * is present.
 *
 * The array must be sorted by arcp_region pointer before calling this
 * function. The removed item will be removed such that the sorting of the
 * array is preserved.
 *
 * @param array the array to remove the region from.
 * @param region the region to remove from the array.
 * @returns the duplicated array.
 */
struct aary *aary_dup_set_remove(struct aary *array,
                                 struct arcp_region *region);

/**
 * Determine if the array contains the specified region.
 *
 * The array must be sorted by arcp_region pointer before calling this
 * function. The removed item will be removed such that the sorting of the
 * array is preserved.
 *
 * @param array the array to search.
 * @param region the region to search for.
 * @returns true if the array contains the specified region, false if it does
 * not.
 */
bool aary_set_contains(struct aary *array, struct arcp_region *region);

/* struct aary *aary_set_union(struct aary *array2,
 * 			    struct aary *array2); */
/* struct aary *aary_dup_set_union(struct aary *array1,
 * 				struct aary *array2); */
/* struct aary *aary_set_intersection(struct aary *array1,
 * 				   struct aary *array2); */
/* struct aary *aary_dup_set_intersection(struct aary *array1,
 * 				       struct aary *array2); */
/* struct aary *aary_set_difference(struct aary *array1,
 * 				 struct aary *array2); */
/* struct aary *aary_dup_set_difference(struct aary *array1,
 * 				     struct aary *array2); */
/* struct aary *aary_set_disjunction(struct aary *array1,
 * 				  struct aary *array2); */
/* struct aary *aary_dup_set_disjunction(struct aary *array1,
 * 				      struct aary *array2); */

/**
 * Convenience alias for aary_equal
 */
#define aary_set_equal aary_equal
/* bool aary_disjoint(struct aary *array1, struct aary *array2); */
/* bool aary_subset(struct aary *array1, struct aary *array2); */
/* bool aary_proper_subset(struct aary *array1, struct aary *array2); */
/* bool aary_superset(struct aary *array1, struct aary *array2); */
/* bool aary_proper_superset(struct aary *array1, struct aary *array2); */

#endif /* ! ATOMICKIT_ARRAY_H */
