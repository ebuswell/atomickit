#include "atomickit/atomic.h"
#include "atomickit/atomic-txn.h"
#include "atomickit/atomic-malloc.h"
#include "atomickit/atomic-mtxn.h"

volatile atomic_uint amtxn_clock = ATOMIC_VAR_INIT(1);

static struct aqueue_node_header sentinel = AQUEUE_SENTINEL_HEADER_VAR_INIT(NULL);
aqueue_t amtxn_queue = AQUEUE_VAR_INIT((struct aqueue_node *) &sentinel);

static void __amtxn_destroy_stub(struct atxn_item *item) {
    struct amtxn_stub *stub = ATXN_ITEM2PTR(item);
    atxn_destroy(&stub->prev);
    atxn_destroy(&stub->value);
    afree(item, ATXN_ITEM_OVERHEAD + sizeof(struct amtxn_stub));
}

static inline struct amtxn_stub *create_stub(void *prev, void *ptr) {
    struct atxn_item *item = amalloc(ATXN_ITEM_OVERHEAD + sizeof(struct amtxn_stub));
    if(item == NULL) {
	return NULL;
    }
    struct amtxn_stub *stub = atxn_item_init(item, __amtxn_destroy_stub);
    atxn_init(&stub->prev, prev);
    atxn_init(&stub->value, ptr);
    atomic_store_explicit(&stub->clock, 0, memory_order_release);
    return stub;
}

int amtxn_init(amtxn_t *txn, void *ptr) {
    struct amtxn_stub *stub = create_stub(NULL, ptr);
    if(stub == NULL) {
	return -1;
    }
    atxn_init(&txn->txn, stub);
    atxn_release(stub);
    return 0;
}

void *amtxn_acquire1(amtxn_t *txn) {
    unsigned int clock;
    struct amtxn_stub *stub;
    void *ret;
    stub = atxn_acquire(&txn->txn);
    clock = atomic_load_explicit(&amtxn_clock, memory_order_acquire);
    if(atomic_load_explicit(&stub->clock, memory_order_acquire) == clock) {
	/* Load previous value */
	ret = atxn_acquire(&stub->prev);
	if(clock != atomic_load_explicit(&amtxn_clock, memory_order_acquire)) {
	    /* Counter changed; return current value */
	    atxn_release(ret);
	    ret = atxn_acquire(&stub->value);
	}
    } else {
	/* Load current value */
	ret = atxn_acquire(&stub->value);
    }
    atxn_release(stub);
    return ret;
}

void *amtxn_peek1(amtxn_t *txn) {
    unsigned int clock;
    struct amtxn_stub *stub;
    void *ret;
    stub = atxn_acquire(&txn->txn);
    clock = atomic_load_explicit(&amtxn_clock, memory_order_acquire);
    if(atomic_load_explicit(&stub->clock, memory_order_acquire) == clock) {
	/* Load previous value */
	ret = atxn_peek(&stub->prev);
	if(clock != atomic_load(&amtxn_clock)) {
	    /* Counter changed; return current value */
	    ret = atxn_peek(&stub->value);
	}
    } else {
	/* Load current value */
	ret = atxn_peek(&stub->value);
    }
    atxn_release(stub);
    return ret;
}

static void __amtxn_handle_destroy(struct aqueue_node *node) {
    amtxn_handle_t *handle = AQUEUE_NODE2PTR(node);
    size_t i;
    for(i = 0; i < handle->nchecks; i++) {
	atxn_release(handle->check_list[i].value);
    }
    afree(handle->check_list, sizeof(struct amtxn_check) * handle->nchecks);
    for(i = 0; i < handle->nupdates; i++) {
	atxn_destroy(&handle->update_list[i].stub->prev);
	atxn_release(handle->update_list[i].stub);
    }
    afree(handle->update_list, sizeof(struct amtxn_update) * handle->nupdates);
    for(i = 0; i < handle->norphans; i++) {
	atxn_release(handle->orphan_list[i]);
    }
    afree(handle->orphan_list, sizeof(void *) * handle->norphans);
    afree(node, AQUEUE_NODE_OVERHEAD + sizeof(amtxn_handle_t));
}

amtxn_handle_t *amtxn_start() {
    struct aqueue_node *node = amalloc(AQUEUE_NODE_OVERHEAD + sizeof(amtxn_handle_t));
    if(node == NULL) {
	return NULL;
    }
    amtxn_handle_t *ret = aqueue_node_init(node, __amtxn_handle_destroy);
    atomic_init(&ret->cstep, 0);
    atomic_init(&ret->clock, 0);
    atomic_init(&ret->status, AMTXN_PENDING);
    ret->nchecks = 0;
    ret->nupdates = 0;
    ret->norphans = 0;
    ret->check_list = NULL;
    ret->update_list = NULL;
    ret->orphan_list = NULL;
    return ret;
}

#define IF_BSEARCH(list, nitems, value, retidx)	do {	\
    (retidx) = 0;					\
    size_t __l = 0;					\
    size_t __u = (nitems);				\
    while(__l < __u) {					\
        (retidx) = (__l + __u) / 2;			\
	amtxn_t *__location = (list)[retidx].location;	\
	if((value) < __location) {			\
	    __u = (retidx);				\
	} else if((value) > __location) {		\
	    __l = ++(retidx);				\
	} else

#define ENDIF_BSEARCH	\
    }			\
} while(0)

#define LISTINSERT(list, size, slot) ({					\
    typeof(list) __ret;							\
    if(atryrealloc(list, sizeof(*list) * size, sizeof(*list) * (size + 1))) { \
	memmove(&list[slot], &list[slot + 1], sizeof(*list) * (size - slot)); \
	__ret = list;							\
    } else {								\
	typeof(list) __new_list = amalloc(sizeof(*list) * (size + 1));	\
	__ret = __new_list;						\
	if(__new_list != NULL) {					\
	    memcpy(__new_list, list, sizeof(*list) * slot);		\
	    memcpy(&__new_list[slot + 1], &list[slot], sizeof(*list) * (size - slot)); \
	    afree(list, sizeof(*list) * size);				\
	    __ret = __new_list;						\
	}								\
    }									\
    __ret;								\
})

void *amtxn_acquire(amtxn_handle_t *handle, amtxn_t *txn) {
    /* First see if we already have this */
    size_t i;
    /* Check update list */
    struct amtxn_update *update_list = handle->update_list;
    IF_BSEARCH(update_list, handle->nupdates, txn, i) {
	return atxn_peek(&update_list[i].stub->value);
    } ENDIF_BSEARCH;
    /* Check check list */
    struct amtxn_check *check_list = handle->check_list;
    IF_BSEARCH(check_list, handle->nchecks, txn, i) {
	return check_list[i].value;
    } ENDIF_BSEARCH;
    /* Couldn't find it. The above code has the side effect of finding
     * the place in check list where the new value ought to have been
     * and storing it in i. */
    /* Fetch the value */
    void *ret = amtxn_acquire1(txn);
    /* Now check that no previous values have changed */
    size_t nchecks = handle->nchecks;
    size_t j;
    for(j = 0; j < nchecks; j++) {
	if(amtxn_peek1(check_list[j].location) != check_list[j].value) {
	    atomic_store_explicit(&handle->status, AMTXN_FAILURE, memory_order_relaxed);
	    amtxn_release1(ret);
	    return NULL;
	}
    }
    /* Realloc check_list and move old values to their appropriate locations */
    check_list = LISTINSERT(check_list, nchecks, i);
    if(check_list == NULL) {
	amtxn_release1(ret);
	atomic_store_explicit(&handle->status, AMTXN_ERROR, memory_order_relaxed);
	return NULL;
    }
    handle->check_list = check_list;
    handle->nchecks = nchecks + 1;
    /* Add a new check at i */
    check_list[i].location = txn;
    check_list[i].value = ret;
    /* Return value */
    return ret;
}

int amtxn_set(amtxn_handle_t *handle, amtxn_t *txn, void *value) {
    /* See if we've already set this previously */
    size_t nupdates = handle->nupdates;
    struct amtxn_update *update_list = handle->update_list;
    size_t i;
    IF_BSEARCH(update_list, nupdates, txn, i) {
	/* Add old value to orphan list*/
	size_t norphans = handle->norphans;
	void **new_orphan_list = arealloc(handle->orphan_list, norphans * sizeof(void *), (norphans + 1) * sizeof(void *));
	if(new_orphan_list == NULL) {
	    atomic_store_explicit(&handle->status, AMTXN_ERROR, memory_order_relaxed);
	    return -1;
	}
	new_orphan_list[norphans] = atxn_acquire(&update_list[i].stub->value);
	handle->orphan_list = new_orphan_list;
	handle->norphans = norphans + 1;
	/* Update the stub with the new value */
	atxn_store(&update_list[i].stub->value, value);
	return 0;
    } ENDIF_BSEARCH;
    /* Couldn't find it. The above code has the side effect of finding
     * the place in update list where the new value ought to have been
     * and storing it in i. */
    /* Create a new stub */
    struct amtxn_stub *stub = create_stub(NULL, value);
    if(stub == NULL) {
	atomic_store_explicit(&handle->status, AMTXN_ERROR, memory_order_relaxed);
	return -1;
    }

    /* Add a new update at i */
    update_list = LISTINSERT(update_list, nupdates, i);
    if(update_list == NULL) {
	atxn_release(stub);
	atomic_store_explicit(&handle->status, AMTXN_ERROR, memory_order_relaxed);
	return -1;
    }
    handle->update_list = update_list;
    update_list[i].location = txn;
    update_list[i].stub = stub;
    handle->nupdates = nupdates + 1;
    return 0;
}

enum amtxn_status amtxn_commit(amtxn_handle_t *handle) {
    enum amtxn_status ret;
    /* Enqueue handle */
    aqueue_enq(&amtxn_queue, handle);
    do {
	/* Get next pending handle */
	amtxn_handle_t *next = aqueue_peek(&amtxn_queue);
	if(next == NULL) {
	    /* Our handle must have finished before we could start working on it. */
	    break;
	}
	/* TODO: could this be efficiently parallelized? */
	while(atomic_load_explicit(&next->status, memory_order_acquire) == AMTXN_PENDING) {
	    unsigned int cstep = atomic_load_explicit(&next->cstep, memory_order_acquire);
	    unsigned int i = cstep;
	    if(i == 0) {
		/* 1. Set clock */
		unsigned int zero = 0;
		atomic_compare_exchange_strong_explicit(&next->clock, &zero, atomic_load_explicit(&amtxn_clock, memory_order_acquire),
							memory_order_acq_rel, memory_order_relaxed);
		/* Advance cstep */
		atomic_compare_exchange_strong_explicit(&next->cstep, &cstep, cstep + 1,
							memory_order_acq_rel, memory_order_relaxed);
	    } else if(--i < next->nchecks) {
		/* 2. Perform checks */
		struct amtxn_stub *stub = atxn_acquire(&next->check_list[i].location->txn);
		/* Make sure step hasn't changed to ensure that stub is the correct value */
		if(atomic_load(&next->cstep) == cstep) {
		    /* Do the check*/
		    if(atxn_check(&stub->value, next->check_list[i].value)) {
			/* Success; advance cstep */
			atomic_compare_exchange_strong_explicit(&next->cstep, &cstep, cstep + 1,
								memory_order_acq_rel, memory_order_relaxed);
		    } else {
			/* Something changed; transaction failed */
			atomic_store_explicit(&next->status, AMTXN_FAILURE, memory_order_release);
		    }
		} /* Otherwise someone else did this step */
		atxn_release(stub);
	    } else if((i -= next->nchecks) < next->nupdates) {
		/* 3. Load stubs with prev value */
		struct amtxn_stub *oldstub = atxn_acquire(&next->update_list[i].location->txn);
		/* Make sure step hasn't changed to ensure that oldstub is the correct value */
		if(atomic_load(&next->cstep) == cstep) {
		    /* Get old value and store it in previous */
		    void *oldvalue = atxn_acquire(&oldstub->value);
		    atxn_store(&next->update_list[i].stub->prev, oldvalue);
		    atxn_release(oldvalue);
		    /* Store clock value */
		    atomic_store_explicit(&next->update_list[i].stub->clock, atomic_load_explicit(&next->clock, memory_order_acquire), memory_order_release);
		    /* Advance cstep */
		    atomic_compare_exchange_strong_explicit(&next->cstep, &cstep, cstep + 1,
							    memory_order_acq_rel, memory_order_relaxed);
		} /* Otherwise someone else did this step */
		atxn_release(oldstub);
	    } else if((i -= next->nupdates) < next->nupdates) {
		/* 4. Set the new stubs */
		struct amtxn_stub *oldstub = atxn_acquire(&next->update_list[i].location->txn);
		/* Make sure step hasn't changed to ensure that oldstub is the correct old value */
		if(atomic_load(&next->cstep) == cstep) {
		    /* Set to the current stub */
		    atxn_commit(&next->update_list[i].location->txn, oldstub, next->update_list[i].stub);
		    /* Advance cstep */
		    atomic_compare_exchange_strong_explicit(&next->cstep, &cstep, cstep + 1,
							    memory_order_acq_rel, memory_order_relaxed);
		} /* Otherwise someone else did this step */
		atxn_release(oldstub);
	    } else {
		/* 5. Change clock to have stubs reflect new values */
		unsigned int oldclock = atomic_load_explicit(&next->clock, memory_order_acquire);
		unsigned int newclock = oldclock + 1;
		if(newclock == 0) {
		    /* Disallow a zero-valued clock */
		    newclock++;
		}
		atomic_compare_exchange_strong_explicit(&amtxn_clock, &oldclock, newclock,
							memory_order_acq_rel, memory_order_relaxed);
		/* Finished; mark success */
		atomic_store_explicit(&next->status, AMTXN_SUCCESS, memory_order_release);
	    }
	}
	/* Dequeue the handle */
	aqueue_deq_cond(&amtxn_queue, next);
	aqueue_node_release(next);
    } while((ret = atomic_load_explicit(&handle->status, memory_order_acquire)) == AMTXN_PENDING);
    aqueue_node_release(handle);
    return ret;
}
