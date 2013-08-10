#include "atomickit/atomic-rcp.h"
#include "atomickit/atomic-queue.h"
#include "atomickit/atomic-malloc.h"

void __aqueue_node_destroy(struct aqueue_node *node) {
    arcp_store(&node->next, NULL);
    arcp_store(&node->item, NULL);
    afree(node, sizeof(struct aqueue_node));
}
