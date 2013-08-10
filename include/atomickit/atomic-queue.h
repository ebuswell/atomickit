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
#include <atomickit/atomic-malloc.h>

/**
 * Queue node
 */
struct aqueue_node {
    struct arcp_region_header header;
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

void __aqueue_node_destroy(struct aqueue_node *node);

#define AQUEUE_NODE_VAR_INIT(ptrcount, refcount, next, item) { ARCP_REGION_HEADER_VAR_INIT(ptrcount, refcount, NULL), ARCP_VAR_INIT(next), ARCP_VAR_INIT(item) }

#define AQUEUE_SENTINEL_VAR_INIT(ptrcount, refcount, next) AQUEUE_NODE_VAR_INIT(ptrcount, refcount, next, NULL)

#define AQUEUE_VAR_INIT(head, tail) { ARCP_VAR_INIT(head), ARCP_VAR_INIT(tail) }

/**
 * Initializes a queue.
 *
 * @param queue a pointer to the queue being initialized.
 *
 * @returns zero on success, nonzero on error.
 */
static inline int aqueue_init(aqueue_t *aqueue) {
    struct aqueue_node *sentinel = amalloc(sizeof(struct aqueue_node));
    if(sentinel == NULL) {
	return -1;
    }
    arcp_init(&sentinel->next, NULL);
    arcp_region_init((struct arcp_region *) sentinel, (void (*)(struct arcp_region *)) __aqueue_node_destroy);

    arcp_init(&aqueue->head, (struct arcp_region *) sentinel);
    arcp_init(&aqueue->tail, (struct arcp_region *) sentinel);
    arcp_release((struct arcp_region *) sentinel);
    return 0;
}

/**
 * Enqueues the given item.
 *
 * @param aqueue a pointer to the queue in which the item is being
 * enqueued.
 * @param item a pointer to the item to enqueue.
 *
 * @returns zero on success, nonzero on error.
 */
static inline int aqueue_enq(aqueue_t *aqueue, struct arcp_region *item) {
    struct aqueue_node *node = amalloc(sizeof(struct aqueue_node));
    if(node == NULL) {
	return -1;
    }
    arcp_init(&node->item, item);
    arcp_init(&node->next, NULL);
    arcp_region_init((struct arcp_region *) node, (void (*)(struct arcp_region *)) __aqueue_node_destroy);

    struct aqueue_node *tail;
    struct aqueue_node *next;
    for(;;) {
	tail = (struct aqueue_node *) arcp_load(&aqueue->tail);
	next = (struct aqueue_node *) arcp_load(&tail->next);
	if(unlikely(next != NULL)) {
	    arcp_compare_exchange(&aqueue->tail, (struct arcp_region *) tail, (struct arcp_region *) next);
	} else if(likely(arcp_compare_exchange(&tail->next, NULL, (struct arcp_region *) node))) {
	    arcp_compare_exchange(&aqueue->tail, (struct arcp_region *) tail, (struct arcp_region *) node);
	    arcp_release((struct arcp_region *) tail);
	    arcp_release((struct arcp_region *) node);
	    return 0;
	}
	arcp_release((struct arcp_region *) next);
	arcp_release((struct arcp_region *) tail);
    }
}

/**
 * Dequeues an item.
 *
 * @param aqueue a pointer to the queue from which the item is being
 * dequeued.
 *
 * @returns a pointer to the dequeued item.
 */
static inline struct arcp_region *aqueue_deq(aqueue_t *aqueue) {
    struct aqueue_node *head;
    struct aqueue_node *next;
    for(;;) {
	head = (struct aqueue_node *) arcp_load(&aqueue->head);
	next = (struct aqueue_node *) arcp_load(&head->next);
	if(next == NULL) {
	    arcp_release((struct arcp_region *) head);
	    return NULL;
	}
	if(likely(arcp_compare_exchange(&aqueue->head, (struct arcp_region *) head, (struct arcp_region *) next))) {
	    struct arcp_region *item = arcp_exchange(&next->item, NULL);
	    arcp_release((struct arcp_region *) next);
	    arcp_release((struct arcp_region *) head);
	    return item;
	}
	arcp_release((struct arcp_region *) next);
	arcp_release((struct arcp_region *) head);
    }
}

/**
 * Returns a pointer to the first item without dequeueing it.
 *
 * @param aqueue a pointer to the queue from which the first item is
 * to be gotten.
 *
 * @returns a pointer to the memory region corresponding to the
 * first item in the queue.
 */
static inline struct arcp_region *aqueue_peek(aqueue_t *aqueue) {
    struct aqueue_node *head;
    struct aqueue_node *next;
    struct arcp_region *item;
    for(;;) {
	head = (struct aqueue_node *) arcp_load(&aqueue->head);
	next = (struct aqueue_node *) arcp_load(&head->next);
	if(next == NULL) {
	    arcp_release((struct arcp_region *) head);
	    return NULL;
	}
	item = arcp_load(&next->item);
	arcp_release((struct arcp_region *) head);
	arcp_release((struct arcp_region *) next);
	if(item == NULL) {
	    atomic_thread_fence(memory_order_seq_cst);
	    if(unlikely((struct aqueue_node *) arcp_load_weak(&aqueue->head) != head)) {
		continue;
	    }
	}
	return item;
    }
}

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
static inline bool aqueue_compare_deq(aqueue_t *aqueue, struct arcp_region *item) {
    struct aqueue_node *head;
    struct aqueue_node *next;
    struct arcp_region *c_item;
    for(;;) {
	head = (struct aqueue_node *) arcp_load(&aqueue->head);
	next = (struct aqueue_node *) arcp_load(&head->next);
	if(next == NULL) {
	    arcp_release((struct arcp_region *) head);
	    return false;
	}
	c_item = arcp_load_weak(&next->item);
	if(c_item == NULL) {
	    if(item != NULL) {
		arcp_release((struct arcp_region *) next);
		arcp_release((struct arcp_region *) head);
		return false;
	    }
	    atomic_thread_fence(memory_order_seq_cst);
	    if(unlikely((struct aqueue_node *) arcp_load_weak(&aqueue->head) != head)) {
		arcp_release((struct arcp_region *) next);
		arcp_release((struct arcp_region *) head);
		continue;
	    }
	} else if(c_item != item) {
	    arcp_release((struct arcp_region *) next);
	    arcp_release((struct arcp_region *) head);
	    return false;
	}
	if(likely(arcp_compare_exchange(&aqueue->head, (struct arcp_region *) head, (struct arcp_region *) next))) {
	    arcp_store(&next->item, NULL);
	    arcp_release((struct arcp_region *) next);
	    arcp_release((struct arcp_region *) head);
	    return true;
	}
	arcp_release((struct arcp_region *) next);
	arcp_release((struct arcp_region *) head);
    }
}

/**
 * Destroys a queue.
 *
 * @param queue a pointer to the queue being destroyed.
 *
 * @returns zero on success or nonzero if one or more of the contained
 * items failed in the destructor.
 */
static inline void aqueue_destroy(aqueue_t *aqueue) {
    arcp_store(&aqueue->head, NULL);
    arcp_store(&aqueue->tail, NULL);
}

#endif /* ! ATOMICKIT_ATOMIC_QUEUE_H */
