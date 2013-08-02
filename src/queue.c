#include "atomickit/atomic-txn.h"
#include "atomickit/atomic-queue.h"

void __aqueue_txitem_destroy(struct atxn_item *item) {
    struct aqueue_node *node = AQUEUE_TXITEM2NODE(item);
    atxn_destroy(&node->header.next);
    if(node->header.destroy != NULL) {
	node->header.destroy(node);
    }
}
