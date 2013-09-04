/** @file atomic-queue.h
 * Atomic Queue
 *
 * Implements an unlimited lock free FIFO queue with reference counted
 * items.  Each item should be enqueued in exactly one queue.  The
 * queue will hold a reference to the last dequeued item until the
 * next item is dequeued.
 *
 * This algorithm is a slightly simplified version of Maged M. Michael
 * and Michael L. Scott, "Simple, Fast, and Practical Non-Blocking and
 * Blocking Concurrent Queue Algorithms," PODC 1996.
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
#ifndef ATOMICKIT_ATOMIC_QUEUE_H
#define ATOMICKIT_ATOMIC_QUEUE_H 1

#include <stdbool.h>
#include <atomickit/atomic-rcp.h>

/**
 * Queue node
 */
struct aqueue_node {
    struct arcp_region;
    arcp_t next; /** the next item in the queue */
    arcp_t item; /** the content of this node */
};

/**
 * Atomic Queue.
 */
typedef struct {
    arcp_t head;
    arcp_t tail;
} aqueue_t;

#define AQUEUE_NODE_VAR_INIT(ptrcount, refcount, next, item) { ARCP_REGION_VAR_INIT(ptrcount, refcount, NULL, NULL), ARCP_VAR_INIT(next), ARCP_VAR_INIT(item) }

#define AQUEUE_SENTINEL_VAR_INIT(ptrcount, refcount, next) AQUEUE_NODE_VAR_INIT(ptrcount, refcount, next, NULL)

#define AQUEUE_VAR_INIT(head, tail) { ARCP_VAR_INIT(head), ARCP_VAR_INIT(tail) }

/**
 * Initializes a queue.
 *
 * @param queue a pointer to the queue being initialized.
 *
 * @returns zero on success, nonzero on error.
 */
int aqueue_init(aqueue_t *aqueue);

/**
 * Enqueues the given item.
 *
 * @param aqueue a pointer to the queue in which the item is being
 * enqueued.
 * @param item a pointer to the item to enqueue.
 *
 * @returns zero on success, nonzero on error.
 */
int aqueue_enq(aqueue_t *aqueue, struct arcp_region *item);

/**
 * Dequeues an item.
 *
 * @param aqueue a pointer to the queue from which the item is being
 * dequeued.
 *
 * @returns a pointer to the dequeued item.
 */
struct arcp_region *aqueue_deq(aqueue_t *aqueue);

/**
 * Returns a pointer to the first item without dequeueing it.
 *
 * @param aqueue a pointer to the queue from which the first item is
 * to be gotten.
 *
 * @returns a pointer to the memory region corresponding to the
 * first item in the queue.
 */
struct arcp_region *aqueue_peek(aqueue_t *aqueue);

/**
 * Dequeues the first node if it is a certain item.
 *
 * @param aqueue a pointer to the queue from which the item is being
 * dequeued.
 *
 * @param item the item to dequeue.
 *
 * @returns true if the item was dequeued, false otherwise.
 */
bool aqueue_compare_deq(aqueue_t *aqueue, struct arcp_region *item);

/**
 * Destroys a queue.
 *
 * @param queue a pointer to the queue being destroyed.
 *
 * @returns zero on success or nonzero if one or more of the contained
 * items failed in the destructor.
 */
void aqueue_destroy(aqueue_t *aqueue);

#endif /* ! ATOMICKIT_ATOMIC_QUEUE_H */
