/** @file atomic-txn.h
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
#ifndef ATOMICKIT_ATOMIC_TXN_H
#define ATOMICKIT_ATOMIC_TXN_H 1

#include <stdbool.h>
#include <string.h>
#include <atomickit/atomic.h>
#include <atomickit/atomic-rcp.h>
#include <atomickit/atomic-queue.h>
#include <atomickit/atomic-malloc.h>

/**
 * Atomic Transaction
 *
 * A reference counted pointer that obeys transactional semantics.
 * Holds items of type `struct arcp_region`, just as `arcp_t`.
 * Although the two containers can be mixed, obviously transactional
 * semantics obtain only to the extent that atxn_t containers are not
 * mixed with arcp_t containers.
 */
typedef struct {
    arcp_t/*<struct atxn_stub>*/ rcp;
} atxn_t;

/**
 * Atomic Transaction Stub
 *
 * A wrapper for the actual value. This is necessary to synchronize
 * the shift of multiple transactional locations.
 */
struct atxn_stub {
    struct arcp_region_header header;
    atomic_uint clock; /** The clock value at which this stub was
			* created. */
    arcp_t prev; /** The previous value. */
    arcp_t value; /** The current value. */
};

/**
 * Atomic Transaction Check
 *
 * An individual equality test that we must make to commit the
 * transaction.
 */
struct atxn_check {
    atxn_t *location; /** The location of the value to check. */
    struct arcp_region *value; /** The value that location should be. */
};

/**
 * Atomic Transaction Update
 *
 * An individual update we will make when we commit the transaction.
 */
struct atxn_update {
    atxn_t *location; /** The location of the update. */
    struct atxn_stub *stub; /** The new stub for the update. */
};

/**
 * Atomic Transaction Status.
 * 
 * The possible resulting statuses of the transaction.
 */
enum atxn_status {
    ATXN_SUCCESS = 0, /** The transaction was successfully committed. */
    ATXN_PENDING, /** The transaction has not yet finished being
		    * processed. It may not have started processing
		    * either. */
    ATXN_FAILURE, /** The conditions of the transaction could not be
		    * fulfilled due to one of the acquired things
		    * having changed. */
    ATXN_ERROR, /** There was an error processing the transaction. */
};

/**
 * Atomic Transaction Handle
 *
 * A handle for a given open transaction. Must not be referenced after
 * the transaction is closed, either by aborting (`atxn_abort`) or by
 * committing (`atxn_commit`).
 */
typedef struct {
    struct arcp_region_header header;
    volatile atomic_ulong procstatus; /** The processing status for this transaction. */
    volatile atomic_uint clock; /** The clock value at which this
				 * transaction is being processed. */
    volatile atomic_int/*<enum atxn_status>*/ status; /** The final status
						       * of this transaction. */
    size_t nchecks; /** The number of checks for this transaction. */
    size_t nupdates; /** The number of updates for this
		      * transaction. */
    size_t norphans; /** The number of orphaned updates for this
		      * transaction. And orphaned update is kept to
		      * secure the reference count until the
		      * transaction is closed. */
    struct atxn_check *check_list; /** The list of checks for this
				     * transaction. */
    struct atxn_update *update_list; /** The list of updates for this
				       * transaction. */
    struct arcp_region **orphan_list; /** The list of orphaned updates
				       * for this transaction. And
				       * orphaned update is kept to
				       * secure the reference count
				       * until the transaction is
				       * closed. */
} atxn_handle_t;

/**
 * Initialize a transaction container.
 *
 * @param txn a pointer to the transaction container being
 * initialized.
 * @param region the initial contents of the transaction
 * container. This should be a pointer to an initialized
 * `arcp_region`, or `NULL`.
 */
int atxn_init(atxn_t *txn, struct arcp_region *region);

/**
 * Destroys a transaction container.
 *
 * @param txn the transaction container to destroy.
 */
static inline void atxn_destroy(atxn_t *txn) {
    arcp_store(&txn->rcp, NULL);
}

/**
 * Load the contents of an atxn outside of the context of an open
 * transaction.
 *
 * @param txn the transaction container from which to acquire the
 * current contents.
 *
 * @returns pointer to the contents of this item.
 */
struct arcp_region *atxn_load1(atxn_t *txn);

/**
 * Load the contents of an atxn outside of the context of an open
 * transaction without affecting the reference count.
 *
 * @param txn the transaction container from which to acquire the
 * current contents.
 *
 * @returns pointer to the contents of this item.
 */
struct arcp_region *atxn_load_weak1(atxn_t *txn);

/**
 * Release a pointer acquired with atxn_load1. Convenience
 * wrapper for arcp_release.
 *
 * Note that pointers acquired with the normal transaction semantics
 * (as opposed to atxn_load1) have a lifespan equal to that of the
 * transaction, and are automatically released on closing that
 * transaction.
 *
 * @param ptr pointer to the memory region for a particular
 * transaction item.
 */
static inline void atxn_release1(struct arcp_region *region) {
    arcp_release(region);
}

/**
 * Open a transaction and return a handle to that transaction.
 *
 * @returns a pointer to the transaction handle.
 */
atxn_handle_t *atxn_start(void);

/**
 * Abort and close a transaction, releasing all acquired values.
 *
 * @param handle a pointer to the handle of the transaction to abort
 * and close.
 */
static inline void atxn_abort(atxn_handle_t *handle) {
    arcp_release((struct arcp_region *) handle);
}

/**
 * Get the status of an open transaction.
 *
 * Useful to check on possible errors after `atxn_acquire` or
 * `atxn_set`. Note that errors or failure during those methods
 * should be followed by aborting the transaction with `atxn_abort`.
 *
 * @param handle a pointer to the handle of the transaction to get the
 * status of.
 */
static inline enum atxn_status atxn_status(atxn_handle_t *handle) {
    return (enum atxn_status) atomic_load_explicit(&handle->status, memory_order_acquire);
}

/**
 * Acquire a value within an open transaction.
 *
 * The values obtained during an open transaction will appear as
 * though they were obtained during a single moment. If we are
 * obtaining a new value and a value we have previously acquired has
 * changed since it was obtained, atxn_acquire will fail with
 * NULL. However, NULL is also a valid value for a transaction
 * container, so `atxn_status` should be run to get the full
 * result.
 *
 * Values acquired are referenced exactly as long as the transaction
 * is open, and the reference expires as soon as it is closed.
 *
 * @param handle a pointer to the handle of the transaction for which
 * to acquire this value.
 * @param txn the transaction container from which to get the value.
 * @param region a pointer to a region pointer in which the value will
 * be returned.
 *
 * @returns 0 on success, nonzero on error.  Failure can happen either
 * because of error or because the transaction semantics could not be
 * guaranteed.
 */
enum atxn_status atxn_load(atxn_handle_t *handle, atxn_t *txn, struct arcp_region **region);

/**
 *
 * Set a value within an open transaction.
 *
 * @param handle a pointer to the handle of the transaction for which
 * to set this value.
 * @param txn the transaction container in which to set the value.
 * @param value the new content of the transaction container.
 *
 * @returns 0 on success, nonzero on error.  Failure can happen either
 * because of error or because the transaction semantics could not be
 * guaranteed.
 */
enum atxn_status atxn_store(atxn_handle_t *handle, atxn_t *txn, struct arcp_region *value);

/**
 * Commit and close the transaction, returning the resulting status.
 *
 * @param handle a pointer to the handle of the transaction to commit.
 *
 * @returns the final status of the transaction.
 */
enum atxn_status atxn_commit(atxn_handle_t *handle);

#endif /* ! ATOMICKIT_ATOMIC_TXN_H */
