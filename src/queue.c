/*
 * queue.c
 *
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
	struct aqueue_node *sentinel = amalloc(sizeof(struct aqueue_node));
	if(sentinel == NULL) {
		return -1;
	}
	arcp_init(&sentinel->item, NULL);
	arcp_init(&sentinel->next, NULL);
	arcp_region_init(sentinel, (arcp_destroy_f) aqueue_node_destroy);

	arcp_init(&aqueue->head, sentinel);
	arcp_init(&aqueue->tail, sentinel);
	arcp_release(sentinel);
	return 0;
}

int aqueue_enq(aqueue_t *aqueue, struct arcp_region *item) {
	struct aqueue_node *node;
	struct aqueue_node *tail;
	struct aqueue_node *next;

	node = amalloc(sizeof(struct aqueue_node));
	if(node == NULL) {
		return -1;
	}
	arcp_init(&node->item, item);
	arcp_init(&node->next, NULL);
	arcp_region_init(node, (arcp_destroy_f) aqueue_node_destroy);

	for(;;) {
		tail = (struct aqueue_node *) arcp_load(&aqueue->tail);
		next = (struct aqueue_node *) arcp_load(&tail->next);
		if(unlikely(next != NULL)) {
			arcp_cas_release(&aqueue->tail, tail, next);
		} else if(likely(arcp_cas(&tail->next, NULL, node))) {
			arcp_cas_release(&aqueue->tail, tail, node);
			return 0;
		} else {
			arcp_release(tail);
		}
	}
}

struct arcp_region *aqueue_deq(aqueue_t *aqueue) {
	struct aqueue_node *head;
	struct aqueue_node *next;
	for(;;) {
		head = (struct aqueue_node *) arcp_load(&aqueue->head);
		next = (struct aqueue_node *) arcp_load(&head->next);
		if(next == NULL) {
			arcp_release(head);
			return NULL;
		}
		if(likely(arcp_cas(&aqueue->head, head, next))) {
			struct arcp_region *item =
				arcp_swap(&next->item, NULL);
			arcp_release(next);
			arcp_release(head);
			return item;
		}
		arcp_release(next);
		arcp_release(head);
	}
}

struct arcp_region *aqueue_peek(aqueue_t *aqueue) {
	struct aqueue_node *head;
	struct aqueue_node *next;
	struct arcp_region *item;
	for(;;) {
		head = (struct aqueue_node *) arcp_load(&aqueue->head);
		next = (struct aqueue_node *) arcp_load(&head->next);
		if(next == NULL) {
			arcp_release(head);
			return NULL;
		}
		item = arcp_load(&next->item);
		arcp_release(head);
		arcp_release(next);
		if(item == NULL) {
			if(unlikely(
					(struct aqueue_node *)
					arcp_load_phantom(&aqueue->head)
					!= head)) {
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
	for(;;) {
		head = (struct aqueue_node *) arcp_load(&aqueue->head);
		next = (struct aqueue_node *) arcp_load(&head->next);
		if(next == NULL) {
			arcp_release(head);
			return false;
		}
		c_item = arcp_load_phantom(&next->item);
		if(c_item == NULL) {
			if(item != NULL) {
				arcp_release(next);
				arcp_release(head);
				return false;
			}
			if(unlikely(
					(struct aqueue_node *)
					arcp_load_phantom(&aqueue->head)
					!= head)) {
				arcp_release(next);
				arcp_release(head);
				continue;
			}
		} else if(c_item != item) {
			arcp_release(next);
			arcp_release(head);
			return false;
		}
		if(likely(arcp_cas(&aqueue->head, head, next))) {
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
