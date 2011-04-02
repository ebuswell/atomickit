# configure.ac
# 
# Process this file with autoconf to produce a configure script.
#
# Copyright 2011 Evan Buswell
# 
# This file is part of Atomic Kit.
# 
# Atomic Kit is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
# 
# Atomic Kit is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.

# define version information
m4_define(ATOMICKIT_VERSION, [0.1])

m4_define(ATOMICKIT_INTERFACE_REVISION, 0)
m4_define(ATOMICKIT_INTERFACE_BACK_COMPATABILITY, 0)
m4_define(ATOMICKIT_SOURCE_REVISION, 0)

AC_PREREQ([2.65])
AC_INIT([atomickit], ATOMICKIT_VERSION, [ebuswell@gmail.com])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR([src/atomic-list.c])
AC_CONFIG_HEADERS([include/config.h include/atomickit/arch/arch.h])
AC_CANONICAL_HOST

VERSION_INFO=ATOMICKIT_INTERFACE_REVISION:ATOMICKIT_SOURCE_REVISION:ATOMICKIT_INTERFACE_BACK_COMPATABILITY
AC_SUBST(VERSION_INFO)

[ATOMICKIT_VERSION]=ATOMICKIT_VERSION
AC_SUBST([ATOMICKIT_VERSION])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for language features.
AC_LANG([C])

# Checks for architecture.
AS_CASE([$host_cpu],
[i?86], [AC_DEFINE([ARCH_X86], [1], [Define if compiling for an x86 or x86_64/amd64 target.])
	 HOST_ARCH_SRC=i386
	 AC_SUBST(HOST_ARCH_SRC)],
[x86_64], [AC_DEFINE([ARCH_X86], [1], [Define if compiling for an x86 or x86_64/amd64 target.])
	   AC_DEFINE([ARCH_X86_64], [1], [Define if compiling for an x86_64/amd64 target.])
	   HOST_ARCH_SRC=x86_64
	   AC_SUBST(HOST_ARCH_SRC)],
[AC_MSG_ERROR([Only compiles for x86 and x86_64 cpus right now.  If you create atomic*
headers for your architecture you should be able to fix this.  The Linux Kernel is a good
place to start.])])

HOST_ARCH=$host
AC_SUBST(HOST_ARCH)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile atomickit.pc])

AC_OUTPUT
