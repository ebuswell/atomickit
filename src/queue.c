/*
 * queue.c
 *
 * Copyright 2014 Evan Buswell
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
#include <stdbool.h>
#include "atomickit/rcp.h"
#include "atomickit/malloc.h"
#include "atomickit/queue.h"

static void aqueue_node_destroy(struct aqueue_node *node) {
	arcp_store(&node->next, NULL);
	arcp_store(&node->item, NULL);
	afree(node, sizeof(struct aqueue_node));
}

int aqueue_init(aqueue_t *aqueue) {
	struct aqueue_node *sentinel;
	/* allocate and initialize a sentinel node */
	sentinel = amalloc(sizeof(struct aqueue_node));
	if (sentinel == NULL) {
		return -1;
	}
	arcp_init(&sentinel->item, NULL);
	arcp_init(&sentinel->next, NULL);
	arcp_region_init(sentinel, (arcp_destroy_f) aqueue_node_destroy);

	/* set both head and tail to the sentinel */
	arcp_init(&aqueue->head, sentinel);
	arcp_init(&aqueue->tail, sentinel);
	arcp_release(sentinel);
	return 0;
}

int aqueue_enq(aqueue_t *aqueue, struct arcp_region *item) {
	struct aqueue_node *node;
	struct aqueue_node *tail;
	struct aqueue_node *next;

	/* allocate and initialize a new node */
	node = amalloc(sizeof(struct aqueue_node));
	if (node == NULL) {
		return -1;
	}
	arcp_init(&node->item, item);
	arcp_init(&node->next, NULL);
	arcp_region_init(node, (arcp_destroy_f) aqueue_node_destroy);

	for (;;) {
		/* acquire tail and tail->next */
		tail = (struct aqueue_node *) arcp_load(&aqueue->tail);
		next = (struct aqueue_node *) arcp_load(&tail->next);
		if (unlikely(next != NULL)) {
			/* another thread has added something to the tail, but
			 * has not yet set the tail to what it added; help it
			 * along */
			arcp_cas_release(&aqueue->tail, tail, next);
		} else if (likely(arcp_cas(&tail->next, NULL, node))) {
			/* successfully added something on the tail; try to
			 * set the new tail; if it doesn't work it just means
			 * that somebody else has set the tail further
			 * along. */
			arcp_cas_release(&aqueue->tail, tail, node);
			return 0;
		} else {
			/* couldn't add something on the end; release and
			 * loop */
			arcp_release(tail);
		}
	}
}

struct arcp_region *aqueue_deq(aqueue_t *aqueue) {
	struct aqueue_node *head;
	struct aqueue_node *next;
	for (;;) {
		/* get head and head->next */
		head = (struct aqueue_node *) arcp_load(&aqueue->head);
		next = (struct aqueue_node *) arcp_load(&head->next);
		if (next == NULL) {
			/* empty (sentinel is all there is) */
			arcp_release(head);
			return NULL;
		}
		if (likely(arcp_cas(&aqueue->head, head, next))) {
			/* successfully slurped the head of the queue */
			/* get item and remove it from the node */
			struct arcp_region *item =
				arcp_swap(&next->item, NULL);
			/* release head and head->next */
			arcp_release(next);
			arcp_release(head);
			return item;
		}
		/* the head of the queue moved out from under us */
		arcp_release(next);
		arcp_release(head);
	}
}

struct arcp_region *aqueue_peek(aqueue_t *aqueue) {
	struct aqueue_node *head;
	struct aqueue_node *next;
	struct arcp_region *item;
	for (;;) {
		/* get head and head->next */
		head = (struct aqueue_node *) arcp_load(&aqueue->head);
		next = (struct aqueue_node *) arcp_load(&head->next);
		if (next == NULL) {
			/* empty (sentinel is all there is) */
			arcp_release(head);
			return NULL;
		}
		/* grab a reference to item */
		item = arcp_load(&next->item);
		/* release stuff */
		arcp_release(head);
		arcp_release(next);
		if (item == NULL) {
			/* either item was snatched up from under us, or it
			 * really is NULL. Compare head w/ aqueue->head to
			 * tell the difference */
			if (unlikely((struct aqueue_node *)
				      arcp_load_phantom(&aqueue->head)
				     != head)) {
				/* item was snatched away */
				continue;
			}
		}
		return item;
	}
}

bool aqueue_cmpdeq(aqueue_t *aqueue, struct arcp_region *item) {
	struct aqueue_node *head;
	struct aqueue_node *next;
	struct arcp_region *c_item;
	for (;;) {
		/* get head and head->next */
		head = (struct aqueue_node *) arcp_load(&aqueue->head);
		next = (struct aqueue_node *) arcp_load(&head->next);
		if (next == NULL) {
			/* empty (sentinel is all there is) */
			arcp_release(head);
			return false;
		}
		/* peek at item */
		c_item = arcp_load_phantom(&next->item);
		if (c_item == NULL) {
			/* either item was snatched up from under us, or it
			 * really is NULL. Compare head w/ aqueue->head to
			 * tell the difference */
			if (unlikely((struct aqueue_node *)
				      arcp_load_phantom(&aqueue->head)
				     != head)) {
				/* item was snatched away */
				arcp_release(next);
				arcp_release(head);
				continue;
			}
		}
		/* check if we should go through with the dequeue */
		if (c_item != item) {
			arcp_release(next);
			arcp_release(head);
			return false;
		}
		/* try to dequeue item */
		if (likely(arcp_cas(&aqueue->head, head, next))) {
			/* mark item as dequeued and release reference */
			arcp_store(&next->item, NULL);
			arcp_release(next);
			arcp_release(head);
			return true;
		}
		arcp_release(next);
		arcp_release(head);
	}
}

void aqueue_destroy(aqueue_t *aqueue) {
	arcp_store(&aqueue->head, NULL);
	arcp_store(&aqueue->tail, NULL);
}
