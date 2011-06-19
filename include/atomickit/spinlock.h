/*
 * spinlock.h
 * 
 * Copyright 2011 Evan Buswell
 *
 * This file is part of Atomic Kit.
 * 
 * Atomic Kit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 * 
 * Atomic Kit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _ATOMICKIT_SPINLOCK_H
#define _ATOMICKIT_SPINLOCK_H

#include <atomickit/atomic.h>
#include <sched.h>
#include <errno.h>

#ifndef likely
# define likely(x)	__builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
# define unlikely(x)	__builtin_expect(!!(x), 0)
#endif

typedef atomic_t spinlock_t;

#define SPINLOCK_INIT() ATOMIC_INIT(0)

static inline void spinlock_init(spinlock_t *spinlock) {
    atomic_set(spinlock, 0);
}

static inline void spinlock_lock(spinlock_t *spinlock) {
    while(1) {
       	if(likely(atomic_cmpxchg(spinlock, 0, -1) == 0)) {
	    return;
	} else {
	    sched_yield(); /* don't check return value; spin on error */
	}
    }
}

static inline void spinlock_multilock(spinlock_t *spinlock) {
    while(1) {
	int val = atomic_read(spinlock);
	if(likely(val == 0)) {
	    /* No locks */
	    if(likely(atomic_cmpxchg(spinlock, 0, 1) == 0)) {
		return;
	    }
	} else if(unlikely(val == -1)) {
	    /* exclusive lock; wait */
	    sched_yield();
	} else {
	    /* nonexclusive lock; increment */
	    if(likely(atomic_cmpxchg(spinlock, val, val + 1) == val)) {
		return;
	    }
	}
    }
}

static inline int spinlock_trylock(spinlock_t *spinlock) {
    if(likely(atomic_cmpxchg(spinlock, 0, -1) == 0)) {
	return 0;
    } else {
	return EBUSY;
    }
}

static inline int spinlock_trymultilock(spinlock_t *spinlock) {
    while(1) {
	int val = atomic_read(spinlock);
	if(likely(val == 0)) {
	    /* No locks */
	    if(likely(atomic_cmpxchg(spinlock, 0, 1) == 0)) {
		return 0;
	    }
	} else if(unlikely(val == -1)) {
	    /* exclusive lock; return */
	    return EBUSY;
	} else {
	    /* nonexclusive lock; increment */
	    if(likely(atomic_cmpxchg(spinlock, val, val + 1) == val)) {
		return 0;
	    }
	}
    }
    return 0;
}

static inline void spinlock_unlock(spinlock_t *spinlock) {
    if(atomic_read(spinlock) == -1) {
	atomic_set(spinlock, 0);
    } else {
	atomic_dec(spinlock);
    }
}

#endif /* #ifndef _ATOMICKIT_ATOMIC_PTR_H */
