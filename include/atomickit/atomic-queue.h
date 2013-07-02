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
#define AQUEUE_NODE_OVERHEAD (sizeof(struct aqueue_node) - 1)

struct aqueue_node;

/**
 * Queue node
 *
 * Good code will not depend on the contents of this struct.
 */
struct aqueue_node_ptr {
    void (*destroy)(struct aqueue_node *); /** pointer to user provided destruction function */
    atxn_t next; /** the next item in the queue */
    uint8_t data[1] __attribute__((aligned(ATXN_ALIGN))); /** user-defined data area */
};

/**
 * Sentinel node
 *
 * Used for the initial contents of an empty queue.
 */
struct aqueue_sentinel_node_ptr {
    void (*destroy)(struct aqueue_node *); /** pointer to user provided destruction function */
    atxn_t next; /** the next item in the queue */
};

/**
 * Queue node, low-level
 *
 * This is essentially a queue node in a `struct atxn_item` wrapper.
 */
struct aqueue_node {
    void (*destroy)(struct aqueue_node *);
    volatile atomic_int_fast8_t refcount;
    volatile atomic_int_fast8_t txn_refcount;
    struct aqueue_node_ptr nodeptr __attribute__((aligned(ATXN_ALIGN)));
};

/**
 * Sentinel node, low-level
 *
 * This is essentially a sentinel node in a `struct atxn_item` wrapper.
 */
struct aqueue_sentinel_node {
    void (*destroy)(struct aqueue_node *);
    volatile atomic_int_fast8_t refcount;
    volatile atomic_int_fast8_t txn_refcount;
    struct aqueue_sentinel_node_ptr nodeptr __attribute__((aligned(ATXN_ALIGN)));
};

/**
 * Get the node for the corresponding node data.
 */
#define AQUEUE_PTR2NODEPTR(ptr) ((struct aqueue_node_ptr *) (((intptr_t) (ptr)) - (offsetof(struct aqueue_node_ptr, data))))
/**
 * Get the node data for the corresponding node.
 */
#define AQUEUE_NODEPTR2PTR(item) ((void *) (((intptr_t) (item)) + (offsetof(struct aqueue_node_ptr, data))))

/**
 * Atomic Queue.
 */
typedef struct {
    atxn_t head;
    atxn_t tail;
} aqueue_t;

static void __aqueue_node_destroy(struct aqueue_node *node) {
    atxn_destroy(&node->nodeptr.next);
    node->nodeptr.destroy(node);
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
    struct aqueue_node_ptr *node_ptr = atxn_item_init((struct atxn_item *) node,
						      (void (*)(struct atxn_item *)) __aqueue_node_destroy);
    node_ptr->destroy = destroy;
    atxn_init(&node_ptr->next, NULL);
    return AQUEUE_NODEPTR2PTR(node_ptr);
}

/**
 * Releases a reference to a queue node.
 *
 * @param item the pointer to release a reference to.
 */
static inline void aqueue_node_release(void *item) {
    atxn_item_release(AQUEUE_PTR2NODEPTR(item));
}

/**
 * Initializes a queue.
 *
 * @param queue a pointer to the queue being initialized.
 * @param node a pointer to the (zero-size) memory region
 * corresponding to an already initialized `aqueue_sentinel_node`.
 */
static inline void aqueue_init(aqueue_t *aqueue, void *sentinel) {
    atxn_init(&aqueue->head, AQUEUE_PTR2NODEPTR(sentinel));
    atxn_init(&aqueue->tail, AQUEUE_PTR2NODEPTR(sentinel));
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
    struct aqueue_node_ptr *nodeptr = AQUEUE_PTR2NODEPTR(item);
    struct aqueue_node_ptr *dummy = atxn_acquire(&nodeptr->next);
    atxn_commit(&nodeptr->next, dummy, NULL);
    atxn_release(&nodeptr->next, dummy);

    struct aqueue_node_ptr *tail;
    struct aqueue_node_ptr *next;
    for(;;) {
	tail = atxn_acquire(&aqueue->tail);
	next = atxn_acquire(&tail->next);
	if(unlikely(next != NULL)) {
	    atxn_commit(&aqueue->tail, tail, next);
	} else if(likely(atxn_commit(&tail->next, next, nodeptr))) {
	    atxn_commit(&aqueue->tail, tail, nodeptr);
	    atxn_item_release(next);
	    atxn_item_release(tail);
	    return;
	}
	atxn_release(&tail->next, next);
	atxn_release(&aqueue->tail, tail);
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
    struct aqueue_node_ptr *head;
    struct aqueue_node_ptr *next;
    for(;;) {
	head = atxn_acquire(&aqueue->head);
	next = atxn_acquire(&head->next);
	if(next == NULL) {
	    atxn_release(&head->next, next);
	    atxn_release(&aqueue->head, head);
	    return NULL;
	}
	if(likely(atxn_commit(&aqueue->head, head, next))) {
	    /* DON'T release next */
	    atxn_item_release(head);
	    return AQUEUE_NODEPTR2PTR(next);
	}
	atxn_release(&head->next, next);
	atxn_release(&aqueue->head, head);
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
