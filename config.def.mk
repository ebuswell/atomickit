PREFIX?=/usr/local
INCLUDEDIR?=${PREFIX}/include
LIBDIR?=${PREFIX}/lib
PKGCONFIGDIR?=${LIBDIR}/pkgconfig
ARCH?=$(shell uname -m)
# ARCH?=x86_64
# ARCH?=i686

CC?=gcc
CFLAGS?=-Wall -Wextra -Wmissing-prototypes -Wredundant-decls -Wdeclaration-after-statement \
       	-Og -g3
LDFLAGS?=
ARFLAGS?=rv

CFLAGS+=-fplan9-extensions
CFLAGS+=-Iinclude -Iinclude/${ARCH}
