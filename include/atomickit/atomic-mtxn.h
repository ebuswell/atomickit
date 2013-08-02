/** @file atomic-mtxn.h
 */
/*
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
#ifndef ATOMICKIT_ATOMIC_MTXN_H
#define ATOMICKIT_ATOMIC_MTXN_H 1

#include <stdbool.h>
#include <string.h>
#include <atomickit/atomic.h>
#include <atomickit/atomic-txn.h>
#include <atomickit/atomic-queue.h>
#include <atomickit/atomic-malloc.h>

/**
 * Atomic Multiple Transaction
 *
 * A container for transactional items suitable for transactions
 * spanning multiple containers.  Holds items of type `struct
 * atxn_item`, just as `atxn_t`.  Although the two containers can be
 * mixed, obviously transactional semantics across multiple containers
 * will obtain only to the extent that amtxn_t containers are not
 * mixed with atxn_t containers.
 */
typedef struct {
    atxn_t/*<struct amtxn_stub>*/ txn;
} amtxn_t;

/**
 * Atomic Multiple Transaction Stub
 *
 * A wrapper for the actual value necessary to synchronize the shift
 * of multiple transactional items.
 */
struct amtxn_stub {
    volatile atomic_uint clock; /** The clock value at which this stub
				 * was created. */
    atxn_t prev; /** The previous value. */
    atxn_t value; /** The current value. */
};

/**
 * Atomic Multiple Transaction Check
 *
 * An individual equality test that we must make to commit the
 * transaction.
 */
struct amtxn_check {
    amtxn_t *location; /** The location of the value to check. */
    void *value; /** The value that location should be. */
};

/**
 * Atomic Multiple Transaction Update
 *
 * An individual update we will make when we commit the transaction.
 */
struct amtxn_update {
    amtxn_t *location; /** The location of the update. */
    struct amtxn_stub *stub; /** The new stub for the update. */
};

/**
 * Atomic Multiple Transaction Status.
 * 
 * The possible resulting statuses of the transaction.
 */
enum amtxn_status {
    AMTXN_PENDING, /** The transaction has not yet finished being
		    * processed. It may not have started processing
		    * either. */
    AMTXN_FAILURE, /** The conditions of the transaction could not be
		    * fulfilled due to one of the acquired things
		    * having changed. */
    AMTXN_ERROR, /** There was an error processing the transaction. */
    AMTXN_SUCCESS /** The transaction was successfully committed. */
};

/**
 * Atomic Multiple Transaction Handle
 *
 * A handle for a given open transaction. Must not be referenced after
 * the transaction is closed, either by aborting (`amtxn_abort`) or by committing (`amtxn_commit`).
 */
typedef struct {
    volatile atomic_uint cstep; /** The current step in processing
				 * this transaction. */
    volatile atomic_uint clock; /** The clock value at which this
				 * transaction is being processed. */
    volatile atomic_int/*<enum amtxn_status>*/ status; /** The status
					 * of this transaction. */
    size_t nchecks; /** The number of checks for this transaction. */
    size_t nupdates; /** The number of updates for this
		      * transaction. */
    size_t norphans; /** The number of orphaned updates for this
		      * transaction. And orphaned update is kept to
		      * secure the reference count until the
		      * transaction is closed. */
    struct amtxn_check *check_list; /** The list of checks for this
				     * transaction. */
    struct amtxn_update *update_list; /** The list of updates for this
				       * transaction. */
    void **orphan_list; /** The list of orphaned updates for this
			 * transaction. And orphaned update is kept to
			 * secure the reference count until the
			 * transaction is closed. */
} amtxn_handle_t;

/**
 * The global amtxn clock
 *
 * Used to synchronize multiple updates so that they appear
 * simultaneously.
 */
extern volatile atomic_uint amtxn_clock;

/**
 * The global amtxn commit queue
 *
 * Used to serialize commits.
 */
extern aqueue_t amtxn_queue;

/**
 * Initialize a multi-item transaction container.
 *
 * @param txn a pointer to the transaction container being
 * initialized.
 * @param ptr initial contents of the transaction container. This
 * should be a pointer to the memory region corresponding to an
 * initialized `atxn_item`, or `NULL`.
 */
int amtxn_init(amtxn_t *txn, void *ptr);

/**
 * Destroys a multi-item transaction container.
 *
 * @param txn the transaction container to destroy.
 */
static inline void amtxn_destroy(amtxn_t *txn) {
    atxn_destroy(&txn->txn);
}

/**
 * Acquire the contents of an amtxn outside of the context of an open
 * transaction.
 *
 * @param txn the transaction container from which to acquire the
 * current contents.
 *
 * @returns pointer to the memory region for the particular
 * transaction item.
 */
void *amtxn_acquire1(amtxn_t *txn);

/**
 * Peek at the contents of an amtxn outside of the context of an open
 * transaction.
 *
 * @param txn the transaction container from which to look at the
 * current contents.
 *
 * @returns pointer to the memory region for the particular
 * transaction item.
 */
void *amtxn_peek1(amtxn_t *txn);

/**
 * Check to see if the ptr is still the value of the amtxn, outside
 * of the context of an open transaction.
 *
 * @param txn the transaction container to check.
 * @param ptr pointer to the memory region for a particular
 * transaction item.
 *
 * @returns true if `ptr` is equal to the content, false otherwise.
*/
static inline bool amtxn_check1(amtxn_t *txn, void *ptr) {
    return ptr == amtxn_peek1(txn);
}

/**
 * Release a pointer acquired with amtxn_acquire1. Convenience
 * wrapper for atxn_release.
 *
 * Note that pointers acquired with the normal transaction semantics
 * have a lifespace equal to that of the transaction, and are
 * automatically released on closing that transaction.
 *
 * @param ptr pointer to the memory region for a particular
 * transaction item.
 * 
 */
static inline void amtxn_release1(void *ptr) {
    atxn_release(ptr);
}

/**
 * Open a transaction and return a handle to that transaction.
 *
 * @returns a pointer to the transaction handle.
 */
amtxn_handle_t *amtxn_start();

/**
 * Abort and close a transaction, releasing all acquired values.
 *
 * @param handle a pointer to the handle of the transaction to abort
 * and close.
 */
static inline void amtxn_abort(amtxn_handle_t *handle) {
    aqueue_node_release(handle);
}

/**
 * Get the status of an open transaction.
 *
 * Useful to check on possible errors after `amtxn_acquire` or
 * `amtxn_set`. Note that errors or failure during those methods
 * should be followed by aborting the transaction with `amtxn_abort`.
 *
 * @param handle a pointer to the handle of the transaction to get the
 * status of.
 */
static inline enum amtxn_status amtxn_status(amtxn_handle_t *handle) {
    return (enum amtxn_status) atomic_load_explicit(&handle->status, memory_order_acquire);
}

/**
 * Acquire a value within an open transaction.
 *
 * The values obtained during an open transaction will appear as
 * though they were obtained during a single moment. If we are
 * obtaining a new value and a value we have previously acquired has
 * changed since it was obtained, amtxn_acquire will fail with
 * NULL. However, NULL is also a valid value for a transaction
 * container, so `amtxn_status` should be run to get the full
 * result.
 *
 * Values acquired are referenced exactly as long as the transaction
 * is open, and the reference expires as soon as it is closed.
 *
 * @param handle a pointer to the handle of the transaction for which
 * to acquire this value.
 * @param txn the transaction container from which to get the value.
 *
 * @returns pointer to the memory region for the particular
 * transaction item.
 */
void *amtxn_acquire(amtxn_handle_t *handle, amtxn_t *txn);

/**
 *
 * Set a value within an open transaction.
 *
 * @param handle a pointer to the handle of the transaction for which
 * to set this value.
 * @param txn the transaction container in which to set the value.
 * @param value the new content of the transaction container.
 *
 * @returns 0 on success, nonzero on error.
 */
int amtxn_set(amtxn_handle_t *handle, amtxn_t *txn, void *value);

/**
 * Commit and close the transaction, returning the resulting status.
 *
 * @param handle a pointer to the handle of the transaction to commit.
 *
 * @returns the final status of the transaction.
 */
enum amtxn_status amtxn_commit(amtxn_handle_t *handle);

#endif /* ! ATOMICKIT_ATOMIC_MTXN_H */
