# Makefile.am
# 
# Copyright 2011 Evan Buswell
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

MAINTAINERCLEANFILES = aclocal.m4 config.guess config.sub configure depcomp install-sh ltmain.sh Makefile.in missing
EXTRA_DIST = autogen.sh Doxyfile
AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libatomickit.la
libatomickit_la_SOURCES = src/atomic-list.c
libatomickit_la_LIBADD = $(LDFLAGS)
libatomickit_la_LDFLAGS = -export-dynamic -version_info @VERSION_INFO@
libatomickit_la_CFLAGS = $(CFLAGS) -Wall -Wextra -Iinclude -Iinclude/@HOST_ARCH_SRC@
libatomickitincludedir = $(pkgincludedir)
libatomickitarchincludedir = $(includedir)/@HOST_ARCH@/$(PACKAGE)/arch
libatomickitinclude_HEADERS = include/atomickit/atomic.h include/atomickit/atomic-float.h include/atomickit/atomic-long.h include/atomickit/atomic-ptr.h include/atomickit/atomic-types.h include/atomickit/atomic-list.h
libatomickitarchinclude_HEADERS = include/@HOST_ARCH_SRC@/atomickit/arch/atomic.h include/@HOST_ARCH_SRC@/atomickit/arch/atomic-float.h include/@HOST_ARCH_SRC@/atomickit/arch/atomic-types.h
nodist_libatomickitarchinclude_HEADERS = include/atomickit/arch/arch.h

pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = atomickit.pc
