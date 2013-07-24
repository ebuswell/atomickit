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

#include <atomickit/atomic-txn.h>

/**
 * The size of the aqueue_node that is not the user data.
 */
#define AQUEUE_NODE_OVERHEAD (offsetof(struct aqueue_node, data))

struct aqueue_node;

/**
 * Queue node header
 */
struct aqueue_node_header {
    void (*destroy)(struct aqueue_node *); /** pointer to user provided destruction function */
    atxn_t next; /** the next item in the queue */
};

/**
 * Queue node
 *
 * Good code will not depend on the contents of this struct.
 */
struct aqueue_node {
    struct aqueue_node_header header;
    struct atxn_item_header txitem_header __attribute__((aligned(ATXN_ALIGN))); /** atxn_item header */
    uint8_t data[1] __attribute__((aligned(ATXN_ALIGN))); /** user-defined data area */
};

/**
 * Get the node for the corresponding node data.
 */
#define AQUEUE_PTR2NODE(ptr) ((struct aqueue_node *) (((intptr_t) (ptr)) - (offsetof(struct aqueue_node, data))))
/**
 * Get the node data for the corresponding node.
 */
#define AQUEUE_NODE2PTR(item) ((void *) (((intptr_t) (item)) + (offsetof(struct aqueue_node, data))))
/**
 * Get the node for the corresponding atxn_item
 */
#define AQUEUE_TXITEM2NODE(item) ((struct aqueue_node *) (((intptr_t) (item)) - (offsetof(struct aqueue_node, txitem_header))))
/**
 * Get the atxn_item for the corresponding node
 */
#define AQUEUE_NODE2TXITEM(item) ((struct atxn_item *) (((intptr_t) (item)) + (offsetof(struct aqueue_node, txitem_header))))


/**
 * Atomic Queue.
 */
typedef struct {
    atxn_t head;
    atxn_t tail;
} aqueue_t;

static void __aqueue_txitem_destroy(struct atxn_item *item) {
    struct aqueue_node *node = AQUEUE_TXITEM2NODE(item);
    atxn_destroy(&node->header.next);
    node->header.destroy(node);
}

/**
 * Initializes a queue node and returns a pointer to its coresponding
 * memory region.
 *
 * The node has a single reference at this point, belonging to the
 * calling function.
 *
 * @param node a pointer to a queue node.  This is typically allocated
 * with `malloc(AQUEUE_NODE_OVERHEAD + <user data size>).
 * @param destroy pointer to a function that will destroy the
 * allocated node once it is no longer in use.  If no cleanup is
 * needed, this could just be `free`.
 *
 * @returns a pointer to the memory region that corresponds with the
 * node.
 */
static inline void *aqueue_node_init(struct aqueue_node *node, void (*destroy)(struct aqueue_node *)) {
    node->header.destroy = destroy;
    atxn_init(&node->header.next, NULL);
    return atxn_item_init(AQUEUE_NODE2TXITEM(node), __aqueue_txitem_destroy);
}

/**
 * Releases a reference to a queue node. Convenience wrapper for
 * `atxn_release`.
 *
 * @param item the pointer to release a reference to.
 */
#define aqueue_node_release(item) atxn_release(item);

/**
 * Initializes a queue.
 *
 * @param queue a pointer to the queue being initialized.
 * @param node a pointer to the (zero-size) memory region
 * corresponding to an already initialized `aqueue_node`. This node is
 * a sentinel and will not be returned.
 */
static inline void aqueue_init(aqueue_t *aqueue, void *sentinel) {
    atxn_init(&aqueue->head, sentinel);
    atxn_init(&aqueue->tail, sentinel);
}

/**
 * Enqueues the given item.
 *
 * @param aqueue a pointer to the queue in which the item is being
 * enqueued.
 * @param item a pointer to the memory region corresponding to the
 * item to enqueue.
 */
static inline void aqueue_enq(aqueue_t *aqueue, void *item) {
    struct aqueue_node *node = AQUEUE_PTR2NODE(item);
    void *dummy = atxn_acquire(&node->header.next);
    atxn_commit(&node->header.next, dummy, NULL);
    atxn_release(dummy);

    void *tailptr;
    void *nextptr;
    struct aqueue_node *tail;
    for(;;) {
	tailptr = atxn_acquire(&aqueue->tail);
	tail = AQUEUE_PTR2NODE(tailptr);
	nextptr = atxn_acquire(&tail->header.next);
	if(unlikely(nextptr != NULL)) {
	    atxn_commit(&aqueue->tail, tailptr, nextptr);
	} else if(likely(atxn_commit(&tail->header.next, nextptr, item))) {
	    atxn_commit(&aqueue->tail, tailptr, item);
	    atxn_release(nextptr);
	    atxn_release(tailptr);
	    return;
	}
	atxn_release(nextptr);
	atxn_release(tailptr);
    }
}

/**
 * Dequeues an item.
 *
 * @param aqueue a pointer to the queue from which the item is being
 * dequeued.
 *
 * @returns a pointer to the memory region corresponding to the
 * dequeued item.
 */
static inline void *aqueue_deq(aqueue_t *aqueue) {
    void *headptr;
    void *nextptr;
    for(;;) {
	headptr = atxn_acquire(&aqueue->head);
	nextptr = atxn_acquire(&(AQUEUE_PTR2NODE(headptr)->header.next));
	if(nextptr == NULL) {
	    atxn_release(nextptr);
	    atxn_release(headptr);
	    return NULL;
	}
	if(likely(atxn_commit(&aqueue->head, headptr, nextptr))) {
	    /* DON'T release next */
	    atxn_release(headptr);
	    return nextptr;
	}
	atxn_release(nextptr);
	atxn_release(headptr);
    }
}

/**
 * Destroys a queue.
 *
 * @param queue a pointer to the queue being destroyed.
 */
static inline void aqueue_destroy(aqueue_t *aqueue) {
    void *node;
    while((node = aqueue_deq(aqueue)) != NULL) {
	aqueue_node_release(node);
    }
    /* Now we should have one node left... */
    atxn_destroy(&aqueue->head);
    atxn_destroy(&aqueue->tail);
}

#endif /* ! ATOMICKIT_ATOMIC_QUEUE_H */
