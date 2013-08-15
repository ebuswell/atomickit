# Makefile.am
# 
# Copyright 2013 Evan Buswell
# 
# This file is part of Atomic Kit.
# 
# Atomic Kit is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 2.
# 
# Atomic Kit is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.

MAINTAINERCLEANFILES = aclocal.m4 config.guess config.sub configure depcomp install-sh ltmain.sh Makefile.in missing include/config.h.in~ compile
EXTRA_DIST = autogen.sh Doxyfile atomickit.pc.in
AUTOMAKE_OPTIONS = foreign serial-tests subdir-objects

lib_LTLIBRARIES = libatomickit.la
libatomickit_la_SOURCES = src/queue.c src/malloc.c src/txn.c src/array.c
libatomickit_la_LDFLAGS = $(LDFLAGS) -L$(libdir) -export-dynamic -version_info @VERSION_INFO@
libatomickit_la_CFLAGS = $(CFLAGS) -Wall -Wextra -Iinclude -Iinclude/@HOST_ARCH_SRC@
libatomickitincludedir = $(pkgincludedir)
libatomickitarchincludedir = $(includedir)/@HOST_ARCH@/$(PACKAGE)/arch
libatomickitinclude_HEADERS = include/atomickit/atomic.h include/atomickit/atomic-float.h include/atomickit/atomic-pointer.h include/atomickit/atomic-rcp.h include/atomickit/atomic-queue.h include/atomickit/atomic-malloc.h include/atomickit/atomic-txn.h include/atomickit/atomic-array.h
libatomickitarchinclude_HEADERS = include/@HOST_ARCH_SRC@/atomickit/arch/compat.h include/@HOST_ARCH_SRC@/atomickit/arch/processor.h

pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = atomickit.pc

check_PROGRAMS = unittest
unittest_SOURCES = test/main.c test/test.c test/test_atomic_h.c test/test_atomic_float_h.c test/test_atomic_pointer_h.c test/test_atomic_rcp_h.c test/test_atomic_queue_h.c test/test_atomic_malloc_h.c test/test_atomic_txn_h.c test/test_atomic_array_h.c
unittest_LDADD = -latomickit
unittest_LDFLAGS = $(LD_FLAGS) -rpath $(libdir)
unittest_CFLAGS = $(CFLAGS) -O0 -g -Wall -Wextra -Iinclude -Iinclude/@HOST_ARCH_SRC@
TESTS = unittest
