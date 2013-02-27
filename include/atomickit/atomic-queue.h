/*
 * atomic-queue.h
 * 
 * Copyright 2012 Evan Buswell
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
/*
 * This algorithm is from Maged M. Michael and Michael L. Scott,
 * "Simple, Fast, and Practical Non-Blocking and Blocking Concurrent
 * Queue Algorithms," PODC 1996.
 */
#ifndef ATOMICKIT_ATOMIC_QUEUE_H
#define ATOMICKIT_ATOMIC_QUEUE_H 1

#include <atomickit/spinlock.h>

struct aqueue_node_ptr {
    void (*destroy)(struct aqueue_node *);
    atxn_t next;
    uint8_t data[1] __attribute__((aligned(ATXN_ALIGN)));
};

struct aqueue_sentinel_node_ptr {
    void (*destroy)(struct aqueue_node *);
    atxn_t next;
};

struct aqueue_node {
    void (*destroy)(struct aqueue_node *);
    volatile atomic_int_fast8_t refcount;
    volatile atomic_int_fast8_t txn_refcount;
    struct aqueue_node_ptr nodeptr __attribute__((aligned(ATXN_ALIGN)));
};

struct aqueue_sentinel_node {
    void (*destroy)(struct aqueue_node *);
    volatile atomic_int_fast8_t refcount;
    volatile atomic_int_fast8_t txn_refcount;
    struct aqueue_sentinel_node_ptr nodeptr __attribute__((aligned(ATXN_ALIGN)));
};


#define AQUEUE_PTR2NODEPTR(ptr) ((struct aqueue_node_ptr *) (((intptr_t) (ptr)) - (offsetof(struct aqueue_node_ptr, data))))
#define AQUEUE_NODEPTR2PTR(item) ((void *) (((intptr_t) (item)) + (offsetof(struct aqueue_node_ptr, data))))

typedef struct {
    atxn_t head;
    atxn_t tail;
} aqueue_t;

static void __aqueue_node_destroy(struct aqueue_node *node) {
    atxn_destroy(&node->nodeptr.next);
    node->nodeptr.destroy(node);
}

static inline void *aqueue_node_init(struct aqueue_node *node, void (*destroy)(struct aqueue_node *)) {
    struct aqueue_node_ptr *node_ptr = atxn_item_init(node, __aqueue_node_destroy);
    node_ptr->destroy = destroy;
    atxn_init(&node_ptr->next, NULL);
    return AQUEUE_NODEPTR2PTR(node_ptr);
}

static inline void aqueue_node_release(void *item) {
    atxn_item_release(AQUEUE_PTR2NODEPTR(item));
}

static inline void aqueue_init(aqueue_t *aqueue, struct aqueue_sentinel_node *node) {
    atxn_init(&aqueue->head, &node->nodeptr);
    atxn_init(&aqueue->tail, &node->nodeptr);
}

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

#endif /* ! ATOMICKIT_ATOMIC_QUEUE_H */
