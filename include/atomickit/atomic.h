/*
 * atomic.h
 *
 * Copyright 2012 Evan Buswell
 * Copyright 2012 Linus Torvalds et al.
 *
 * This file is a redaction of arch/x86/include/compiler.h and some
 * other files from the Linux Kernel.
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
#define ATOMICKIT_ATOMIC_H 1

#ifndef likely
# define likely(x)	__builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
# define unlikely(x)	__builtin_expect(!!(x), 0)
#endif

#ifdef HAVE_STDATOMIC_H
# include <stdatomic.h>
#else
# include <atomickit/arch/compat.h>
#endif

#endif /* ! ATOMICKIT_ATOMIC_H */
