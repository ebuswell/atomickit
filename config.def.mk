PREFIX?=/usr/local
INCLUDEDIR?=${PREFIX}/include
LIBDIR?=${PREFIX}/lib
DESTDIR?=
PKGCONFIGDIR?=${LIBDIR}/pkgconfig
ARCH?=$(shell uname -m)
# ARCH?=x86_64
# ARCH?=i686

CC?=gcc
CFLAGS?=-Og -g3
LDFLAGS?=
AR?=ar
ARFLAGS?=rv

CFLAGS+=-Wall -Wextra -Wmissing-prototypes -Wredundant-decls -Wdeclaration-after-statement
CFLAGS+=-fplan9-extensions
CFLAGS+=-Iinclude -Iinclude/${ARCH}
