/** @file yield.h
 * yield.h
 *
 * Gives us cpu_relax()
 */
/*
 * Copyright 2013 Evan Buswell
 * Copyright 2013 Linus Torvalds et al.
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
#ifndef ATOMICKIT_ATOMIC_H
# error atomickit/arch/yield.h should not be included directly; include atomickit/atomic.h instead
#endif

#ifndef ATOMICKIT_ARCH_YIELD_H
#define ATOMICKIT_ARCH_YIELD_H 1

/* REP NOP (PAUSE) is a good thing to insert into busy-wait loops. */
static inline void cpu_relax(void) {
    asm volatile("rep; nop" ::: "memory");
}

#endif /* ! ATOMICKIT_ARCH_YIELD_H */
