#include "atomickit/atomic.h"
#include "atomickit/atomic-rcp.h"
#include "atomickit/atomic-malloc.h"
#include "atomickit/atomic-txn.h"

static volatile atomic_uint atxn_clock = ATOMIC_VAR_INIT(1);

static struct aqueue_node sentinel = AQUEUE_SENTINEL_VAR_INIT(2, 0, NULL);
static aqueue_t atxn_queue = AQUEUE_VAR_INIT(&sentinel, &sentinel);

static void __atxn_destroy_stub(struct atxn_stub *stub) {
    arcp_store(&stub->prev, NULL);
    arcp_store(&stub->value, NULL);
    afree(stub, sizeof(struct atxn_stub));
}

static inline struct atxn_stub *create_stub(struct arcp_region *prev, struct arcp_region *value) {
    struct atxn_stub *stub = amalloc(sizeof(struct atxn_stub));
    if(stub == NULL) {
	return NULL;
    }
    arcp_init(&stub->prev, prev);
    arcp_init(&stub->value, value);
    atomic_store_explicit(&stub->clock, 0, memory_order_release);
    arcp_region_init(stub, (void (*)(struct arcp_region *)) __atxn_destroy_stub);
    return stub;
}

int atxn_init(atxn_t *txn, struct arcp_region *region) {
    struct atxn_stub *stub = create_stub(NULL, region);
    if(stub == NULL) {
	return -1;
    }
    arcp_init(&txn->rcp, stub);
    arcp_release(stub);
    return 0;
}

struct arcp_region *atxn_load1(atxn_t *txn) {
    unsigned int clock;
    struct atxn_stub *stub;
    struct arcp_region *ret;
    stub = (struct atxn_stub *) arcp_load(&txn->rcp);
    clock = atomic_load_explicit(&atxn_clock, memory_order_acquire);
    if(atomic_load_explicit(&stub->clock, memory_order_acquire) == clock) {
	/* Load previous value */
	ret = arcp_load(&stub->prev);
	if(clock != atomic_load(&atxn_clock)) { /* Memory barrier */
	    /* Counter changed; return current value */
	    arcp_release(ret);
	    ret = arcp_load(&stub->value);
	}
    } else {
	/* Load current value */
	ret = arcp_load(&stub->value);
    }
    arcp_release(stub);
    return ret;
}

struct arcp_region *atxn_load_weak1(atxn_t *txn) {
    unsigned int clock;
    struct atxn_stub *stub;
    struct arcp_region *ret;
    stub = (struct atxn_stub *) arcp_load(&txn->rcp);
    clock = atomic_load_explicit(&atxn_clock, memory_order_acquire);
    if(atomic_load_explicit(&stub->clock, memory_order_acquire) == clock) {
	/* Load previous value */
	ret = arcp_load_weak(&stub->prev);
	if(clock != atomic_load(&atxn_clock)) { /* Memory barrier */
	    /* Counter changed; return current value */
	    ret = arcp_load_weak(&stub->value);
	}
    } else {
	/* Load current value */
	ret = arcp_load_weak(&stub->value);
    }
    arcp_release(stub);
    return ret;
}

static void __atxn_handle_destroy(struct atxn_handle *handle) {
    size_t i;
    for(i = 0; i < handle->nchecks; i++) {
	arcp_release(handle->check_list[i].value);
    }
    afree(handle->check_list, sizeof(struct atxn_check) * handle->nchecks);
    for(i = 0; i < handle->nupdates; i++) {
	arcp_store(&handle->update_list[i].stub->prev, NULL);
	arcp_release(handle->update_list[i].stub);
    }
    afree(handle->update_list, sizeof(struct atxn_update) * handle->nupdates);
    for(i = 0; i < handle->norphans; i++) {
	arcp_release(handle->orphan_list[i]);
    }
    afree(handle->orphan_list, sizeof(struct arcp_region *) * handle->norphans);
    afree(handle, sizeof(struct atxn_handle));
}

struct atxn_handle *atxn_start() {
    struct atxn_handle *ret = amalloc(sizeof(struct atxn_handle));
    if(ret == NULL) {
	return NULL;
    }
    atomic_init(&ret->procstatus, 0);
    atomic_init(&ret->clock, 0);
    atomic_init(&ret->status, ATXN_PENDING);
    ret->nchecks = 0;
    ret->nupdates = 0;
    ret->norphans = 0;
    ret->check_list = NULL;
    ret->update_list = NULL;
    ret->orphan_list = NULL;
    arcp_region_init(ret, (void (*)(struct arcp_region *)) __atxn_handle_destroy);
    return ret;
}

#define IF_BSEARCH(list, nitems, value, retidx)	do {	\
    (retidx) = 0;					\
    size_t __l = 0;					\
    size_t __u = (nitems);				\
    while(__l < __u) {					\
        (retidx) = (__l + __u) / 2;			\
	atxn_t *__location = (list)[retidx].location;	\
	if((value) < __location) {			\
	    __u = (retidx);				\
	} else if((value) > __location) {		\
	    __l = ++(retidx);				\
	} else

#define ENDIF_BSEARCH	\
    }			\
} while(0)

static int update_list_insert(struct atxn_handle *handle, size_t i,
			      atxn_t *location, struct arcp_region *region) {
    struct atxn_stub *stub = create_stub(NULL, region);
    if(stub == NULL) {
	return -1;
    }
    size_t length = handle->nupdates;
    struct atxn_update *list = handle->update_list;
    if(atryrealloc(list, sizeof(struct atxn_update) * length, sizeof(struct atxn_update) * (length + 1))) {
	memmove(&list[i + 1], &list[i], sizeof(struct atxn_update) * (length - i));
    } else {
	struct atxn_update *new_list = amalloc(sizeof(struct atxn_update) * (length + 1));
	if(new_list == NULL) {
	    arcp_release(stub);
	    return -1;
	}
	memcpy(new_list, list, sizeof(struct atxn_update) * i);
	memcpy(&new_list[i + 1], &list[i], sizeof(struct atxn_update) * (length - i));
	afree(list, sizeof(struct atxn_update) * length);
	list = new_list;
	handle->update_list = list;
    }
    list[i].location = location;
    list[i].stub = stub;
    handle->nupdates = length + 1;
    return 0;
}

static int check_list_insert(struct atxn_handle *handle, size_t i,
			     atxn_t *location, struct arcp_region *region) {
    size_t length = handle->nchecks;
    struct atxn_check *list = handle->check_list;
    if(atryrealloc(list, sizeof(struct atxn_check) * length, sizeof(struct atxn_check) * (length + 1))) {
	memmove(&list[i + 1], &list[i], sizeof(struct atxn_check) * (length - i));
    } else {
	struct atxn_check *new_list = amalloc(sizeof(struct atxn_check) * (length + 1));
	if(new_list == NULL) {
	    return -1;
	}
	memcpy(new_list, list, sizeof(struct atxn_check) * i);
	memcpy(&new_list[i + 1], &list[i], sizeof(struct atxn_check) * (length - i));
	afree(list, sizeof(struct atxn_check) * length);
	list = new_list;
	handle->check_list = list;
    }
    list[i].location = location;
    list[i].value = region;
    handle->nchecks = length + 1;
    return 0;
}

static int orphan_list_append(struct atxn_handle *handle, struct arcp_region *region) {
    size_t length = handle->norphans;
    struct arcp_region **list = handle->orphan_list;
    list = arealloc(list, sizeof(struct arcp_region *) * length, sizeof(struct arcp_region *) * (length + 1));
    if(list == NULL) {
	return -1;
    }
    handle->orphan_list = list;
    list[length] = region;
    handle->norphans = length + 1;
    return 0;
}

enum atxn_status atxn_load(struct atxn_handle *handle, atxn_t *txn, struct arcp_region **region) {
    /* First see if we already have this */
    size_t i;
    /* Check update list */
    struct atxn_update *update_list = handle->update_list;
    IF_BSEARCH(update_list, handle->nupdates, txn, i) {
	*region = arcp_load_weak(&update_list[i].stub->value);
	return ATXN_SUCCESS;
    } ENDIF_BSEARCH;
    /* Check check list */
    struct atxn_check *check_list = handle->check_list;
    IF_BSEARCH(check_list, handle->nchecks, txn, i) {
	*region = check_list[i].value;
	return ATXN_SUCCESS;
    } ENDIF_BSEARCH;
    /* Couldn't find it. The above code has the side effect of finding
     * the place in check list where the new value ought to have been
     * and storing it in i. */
    /* Fetch the value */
    *region = atxn_load1(txn);
    /* Now check that no previous values have changed */
    size_t nchecks = handle->nchecks;
    size_t j;
    for(j = 0; j < nchecks; j++) {
	if(atxn_load_weak1(check_list[j].location) != check_list[j].value) {
	    atomic_store_explicit(&handle->status, ATXN_FAILURE, memory_order_relaxed);
	    atxn_release1(*region);
	    return ATXN_FAILURE;
	}
    }
    int r = check_list_insert(handle, i, txn, *region);
    if(r != 0) {
	atxn_release1(*region);
	return ATXN_ERROR;
    }
    return ATXN_SUCCESS;
}

enum atxn_status atxn_store(struct atxn_handle *handle, atxn_t *txn, struct arcp_region *value) {
    int r;
    /* See if we've already set this previously */
    size_t i;
    struct atxn_update *update_list = handle->update_list;
    IF_BSEARCH(update_list, handle->nupdates, txn, i) {
	/* Add old value to orphan list*/
	r = orphan_list_append(handle, arcp_load(&update_list[i].stub->value));
	if(r != 0) {
	    return ATXN_ERROR;
	}
	/* Update the stub with the new value */
	arcp_store(&update_list[i].stub->value, value);
	return ATXN_SUCCESS;
    } ENDIF_BSEARCH;
    /* Couldn't find it. The above code has the side effect of finding
     * the place in update list where the new value ought to have been
     * and storing it in i. */
    r = update_list_insert(handle, i, txn, value);
    if(r != 0) {
	return ATXN_ERROR;
    }
    return ATXN_SUCCESS;
}

enum atxn_status atxn_commit(struct atxn_handle *handle) {
    if(handle->nupdates == 0) {
	arcp_release(handle);
	return ATXN_SUCCESS;
    }
    /* Enqueue handle */
    int r = aqueue_enq(&atxn_queue, handle);
    if(r != 0) {
	arcp_release(handle);
	return ATXN_ERROR;
    }
    enum atxn_status ret;
    do {
	/* Get next pending handle */
	struct atxn_handle *next = (struct atxn_handle *) aqueue_peek(&atxn_queue);
	if(next == NULL) {
	    /* Our handle must have finished before we could start working on it. */
	    break;
	}

	unsigned int clock = atomic_load_explicit(&next->clock, memory_order_acquire);
	if(clock == 0) {
	    /* 1. Set clock */
	    unsigned int newclock = atomic_load_explicit(&atxn_clock, memory_order_acquire);
	    if(atomic_compare_exchange_strong_explicit(&next->clock, &clock, newclock,
						       memory_order_acq_rel, memory_order_acquire)) {
		clock = newclock;
	    }
	}

	/* 2. Set up bins and procstatus */
	const size_t bins = ((sizeof(unsigned long) * 4) / 3);
	unsigned long procstatus = atomic_load_explicit(&next->procstatus, memory_order_acquire);

	unsigned long round;
	unsigned long donebit;
	unsigned long procbit;
	size_t i;

	/* 3. Check previously obtained values */
	if(next->nchecks != 0) {
	    size_t checks_per_bin = 1 + (next->nchecks - 1) / bins; /* ceil(next->nchecks/bins) */
	    size_t max_bin = 1 + (next->nchecks - 1) / checks_per_bin; /* ceil(next->nchecks/checks_per_bin) */
	    for(round = 0; round < 2; round++) {
		donebit = 2;
		procbit = (1 << round);
		for(i = 0; i < max_bin; procbit <<= 2, donebit <<= 2, i++) {
		    if(procstatus & procbit) {
			continue;
		    }
		    if(round == 0) {
			procstatus = atomic_fetch_or_explicit(&next->procstatus, procbit, memory_order_acq_rel);
			if(procstatus & procbit) {
			    continue;
			}
		    }
		    size_t j = i * checks_per_bin;
		    size_t max = j + checks_per_bin;
		    if(max > next->nchecks) {
			max = next->nchecks;
		    }
		    for(; j < max; j++) {
			struct atxn_stub *stub = (struct atxn_stub *) arcp_load(&next->check_list[j].location->rcp);
			/* Make sure this hasn't been processed to ensure stub is correct value */
			if((procstatus = atomic_load(&next->procstatus)) & donebit) { /* Full memory barrier */
			    /* Someone else did this step */
			    arcp_release(stub);
			    break;
			}
			/* Do the check */
			if(arcp_load_weak(&stub->value) != next->check_list[j].value) {
			    /* Something changed; transaction failed */
			    atomic_store_explicit(&next->status, ATXN_FAILURE, memory_order_release);
			    procstatus = (unsigned long) -1L;
			    atomic_store_explicit(&next->procstatus, procstatus, memory_order_release);
			    arcp_release(stub);
			    break;
			}
			arcp_release(stub);
		    }
		    procstatus = atomic_fetch_or_explicit(&next->procstatus, donebit, memory_order_acq_rel) | donebit;
		}
	    }
	    if(atomic_load_explicit(&next->status, memory_order_acquire) != ATXN_PENDING) {
		goto dequeue;
	    }
	}
	/* 4. Load stubs with prev value */
	size_t updates_per_bin = 1 + (next->nupdates - 1) / bins; /* ceil(next->nupdates/bins) */
	size_t max_bin = 1 + (next->nupdates - 1) / updates_per_bin; /* ceil(next->nupdates/updates_per_bin) */
	for(round = 0; round < 2; round++) {
	    donebit = 2 << bins;
	    procbit = (1 << round) << bins;
	    for(i = 0; i < max_bin; procbit <<= 2, donebit <<= 2, i++) {
		if(procstatus & procbit) {
		    continue;
		}
		if(round == 0) {
		    procstatus = atomic_fetch_or_explicit(&next->procstatus, procbit, memory_order_acq_rel);
		    if(procstatus & procbit) {
			continue;
		    }
		}
		size_t j = i * updates_per_bin;
		size_t max = j + updates_per_bin;
		if(max > next->nupdates) {
		    max = next->nupdates;
		}
		for(; j < max; j++) {
		    struct atxn_stub *oldstub = (struct atxn_stub *) arcp_load(&next->update_list[j].location->rcp);
		    /* Make sure this hasn't been processed to ensure oldstub is correct value */
		    if((procstatus = atomic_load(&next->procstatus)) & donebit) { /* Full memory barrier */
			/* Someone else did this step */
			arcp_release(oldstub);
			break;
		    }
		    /* Get old value and store it in previous */
		    struct arcp_region *oldvalue = arcp_load_weak(&oldstub->value);
		    arcp_store(&next->update_list[j].stub->prev, oldvalue);
		    /* Store clock value */
		    atomic_store_explicit(&next->update_list[j].stub->clock, clock, memory_order_release);
		    arcp_release(oldstub);
		}
		procstatus = atomic_fetch_or_explicit(&next->procstatus, donebit, memory_order_acq_rel) | donebit;
	    }
	}
	if(atomic_load_explicit(&next->status, memory_order_acquire) != ATXN_PENDING) {
	    goto dequeue;
	}
	/* 5. Set the new stubs */
	for(round = 0; round < 2; round++) {
	    donebit = 2 << (bins * 2);
	    procbit = (1 << round) << (bins * 2);
	    for(i = 0; i < max_bin; procbit <<= 2, donebit <<= 2, i++) {
		if(procstatus & procbit) {
		    continue;
		}
		if(round == 0) {
		    procstatus = atomic_fetch_or_explicit(&next->procstatus, procbit, memory_order_acq_rel);
		    if(procstatus & procbit) {
			continue;
		    }
		}
		size_t j = i * updates_per_bin;
		size_t max = j + updates_per_bin;
		if(max > next->nupdates) {
		    max = next->nupdates;
		}
		for(; j < max; j++) {
		    struct atxn_stub *oldstub = (struct atxn_stub *) arcp_load(&next->update_list[j].location->rcp);
		    /* Make sure this hasn't been processed to ensure oldstub is correct value */
		    if((procstatus = atomic_load(&next->procstatus)) & donebit) { /* Full memory barrier */
			/* Someone else did this step */
			arcp_release(oldstub);
			break;
		    }
		    /* Set to the current stub */
		    arcp_compare_store(&next->update_list[j].location->rcp, oldstub,
					  next->update_list[j].stub);
		    arcp_release(oldstub);
		}
		procstatus = atomic_fetch_or_explicit(&next->procstatus, donebit, memory_order_acq_rel) | donebit;
	    }
	}
	if(atomic_load_explicit(&next->status, memory_order_release) != ATXN_PENDING) {
	    goto dequeue;
	}
	/* 6. Change clock to have stubs reflect new values */
	unsigned int newclock = clock + 1;
	if(newclock == 0) {
	    /* Disallow a zero-valued clock */
	    newclock++;
	}
	atomic_compare_exchange_strong_explicit(&atxn_clock, &clock, newclock,
						memory_order_acq_rel, memory_order_relaxed);
	/* Finished; mark success */
	atomic_store_explicit(&next->status, ATXN_SUCCESS, memory_order_release);
    dequeue:
	/* Dequeue the handle */
	aqueue_compare_deq(&atxn_queue, next);
	arcp_release(next);
    } while((ret = atomic_load_explicit(&handle->status, memory_order_acquire)) == ATXN_PENDING);
    arcp_release(handle);
    return ret;
}
