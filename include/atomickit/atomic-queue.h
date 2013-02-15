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
    atxn_t next;
    uint8_t data[1] __attribute__((aligned(ATXN_ALIGN)));
};

struct aqueue_node {
    void (*destroy)(struct aqueue_node *);
    volatile atomic_int_fast8_t refcount;
    volatile atomic_int_fast8_t txn_refcount;
    struct aqueue_node_ptr nodeptr __attribute__((aligned(ATXN_ALIGN)));
};

#define AQUEUE_PTR2ITEM(ptr) ((struct aqueue_node *) (((intptr_t) (ptr)) - (offsetof(struct aqueue_node, nodeptr) + offsetof(struct aqueue_node_ptr, data))))
#define AQUEUE_ITEM2PTR(item) ((void *) &item->nodeptr.data)
#define AQUEUE_PTR2NODEPTR(ptr) ((struct aqueue_node_ptr *) (((intptr_t) (ptr)) - (offsetof(struct aqueue_node_ptr, data))))
#define AQUEUE_NODEPTR2PTR(item) ((void *) &item->data)

typedef struct {
    volatile atxn_t head;
    volatile atxn_t tail;
    struct aqueue_node sentinel;
} aqueue_t;

typedef aqueue_t atomic_ptr;

static inline void __aqueue_item_null_destroy(struct atxn_item *) {
    /* Do nothing */
}

static inline void *aqueue_node_init(struct aqueue_node *node, void (*destroy)(struct aqueue_node *)) {
    return AQUEUE_NODEPTR2PTR(atxn_item_init(node, destroy));
}

static inline void aqueue_item_release(void *item) {
    atxn_item_release(AQUEUE_PTR2NODEPTR(item));
}

static inline void aqueue_init(aqueue_t *aqueue, void *item) {
    struct aqueue_nodeptr *nodeptr = AQUEUE_PTR2NODEPTR(item);
    atxn_init(&nodeptr->next, NULL);
    atxn_init(&aqueue->head, nodeptr);
    atxn_init(&aqueue->tail, nodeptr);
}

static inline void aqueue_enq(aqueue_t *aqueue, void *item) {
    struct aqueue_node_ptr *tail;
    struct aqueue_node_ptr *next;
    struct aqueue_node_ptr *nodeptr = AQUEUE_PTR2NODEPTR(item);
    atxn_init(&nodeptr->next, NULL);
    for(;;) {
	tail = atxn_acquire(&aqueue->tail);
	next = atxn_acquire(&tail->next);
	if(likely(atxn_check(&aqueue->tail, tail))) {
	    if(likely(next == NULL)) {
		if(likely(atxn_commit(&tail->next, next, nodeptr))) {
		    atxn_commit(&aqueue->tail, tail, node_ptr);  /* fail is OK */
		    atxn_release(&tail->next, next);
		    atxn_release(&aqueue->tail, tail);
		    return;
		}
	    } else {
		atxn_commit(&aqueue->tail, tail, next); /* fail is OK */
	    }
	}
	atxn_release(tail, next);
	atxn_release(&aqueue->tail, tail);
    }
}

static inline void *aqueue_deq(aqueue_t *aqueue) {
    struct aqueue_node_ptr *head;
    struct aqueue_node_ptr *next;
    struct aqueue_node_ptr *tail;
    for(;;) {
	head = atxn_acquire(&aqueue->head);
	tail = atxn_acquire(&aqueue->tail);
	next = atxn_acquire(&head->next);
	if(atxn_check(&aqueue->head, head)) {
	    if(head == tail) {
		if(next == NULL) {
		    atxn_release(&head->next, next);
		    atxn_release(&aqueue->tail, tail);
		    atxn_release(&aqueue->head, head);
		    return NULL;
		}
		atxn_commit(&aqueue->tail, tail, next);
	    } else {
		if(atxn_commit(&aqueue->head, head, next)) {
		    /* DON'T release next */
		    atxn_release(&aqueue->tail, tail);
		    atxn_release(&aqueue->head, head);
		    return AQUEUE_NODEPTR2PTR(next);
		}
	    }
	}
	atxn_release(&head->next, next);
	atxn_release(&aqueue->tail, tail);
	atxn_release(&aqueue->head, head);
    }
}

#endif /* ! ATOMICKIT_ATOMIC_QUEUE_H */
