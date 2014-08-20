# Atomic Kit is Copyright 2013 Evan Buswell
# 
# Atomic Kit is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation, version 2.
# 
# Atomic Kit is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
# 
# You should have received a copy of the GNU General Public License along with
# Atomic Kit.  If not, see <http://www.gnu.org/licenses/>.

.PHONY: shared static all install-headers install-pkgconfig install-shared \
        install-static install-static-strip install-shared-strip \
        install-all-static install-all-shared install-all-static-strip \
        install-all-shared-strip install install-strip uninstall clean \
        check-shared check-static check

.SUFFIXES: .o .pic.o

include config.mk

MAJOR=0
MINOR=3

SRCS=src/rcp.c src/queue.c src/malloc.c src/array.c src/string.c src/dict.c

TESTSRCS=test/main.c test/test_array_h.c test/test_float_h.c \
         test/test_atomic_h.c test/test_malloc_h.c \
         test/test_queue_h.c test/test_rcp_h.c test/test.c

HEADERS=include/atomickit/atomic.h \
        include/atomickit/float.h \
        include/atomickit/pointer.h \
        include/atomickit/rcp.h \
        include/atomickit/queue.h \
        include/atomickit/malloc.h \
        include/atomickit/txn.h \
        include/atomickit/array.h \
        include/atomickit/string.h

ARCHHEADERS=include/${ARCH}/atomickit/arch/atomic.h \
            include/${ARCH}/atomickit/arch/misc.h

OBJS=${SRCS:.c=.o}
PICOBJS=${SRCS:.c=.pic.o}
TESTOBJS=${TESTSRCS:.c=.o}

all: shared atomickit.pc

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

.c.pic.o:
	${CC} ${CFLAGS} -fPIC -c $< -o $@

libatomickit.so: ${PICOBJS}
	${CC} ${CFLAGS} -fPIC ${LDFLAGS} -shared ${PICOBJS} -o libatomickit.so

libatomickit.a: ${OBJS}
	rm -f libatomickit.a
	${AR} ${ARFLAGS}c libatomickit.a ${OBJS}

unittest-shared: libatomickit.so ${TESTOBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -L`pwd` -Wl,-rpath,`pwd` ${TESTOBJS} -latomickit -o unittest-shared

unittest-static: libatomickit.a ${TESTOBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -static -L`pwd` ${TESTOBJS} -latomickit -o unittest-static

atomickit.pc: atomickit.pc.in config.mk Makefile
	sed -e 's!@prefix@!${PREFIX}!g' \
	    -e 's!@libdir@!${LIBDIR}!g' \
	    -e 's!@includedir@!${INCLUDEDIR}!g' \
	    -e 's!@version@!${VERSION}!g' \
	    atomickit.pc.in >atomickit.pc

shared: libatomickit.so

static: libatomickit.a

install-headers:
	(umask 022; mkdir -p ${DESTDIR}${INCLUDEDIR}/atomickit/arch)
	install -m 644 -t ${DESTDIR}${INCLUDEDIR}/atomickit ${HEADERS}
	install -m 644 -t ${DESTDIR}${INCLUDEDIR}/atomickit/arch ${ARCHHEADERS}

install-pkgconfig: atomickit.pc
	(umask 022; mkdir -p ${DESTDIR}${PKGCONFIGDIR})
	install -m 644 atomickit.pc ${DESTDIR}${PKGCONFIGDIR}/atomickit.pc

install-shared: shared
	(umask 022; mkdir -p ${DESTDIR}${LIBDIR})
	install -m 755 libatomickit.so ${DESTDIR}${LIBDIR}/libatomickit.so.${MAJOR}.${MINOR}
	ln -frs ${DESTDIR}${LIBDIR}/libatomickit.so.${MAJOR}.${MINOR} ${DESTDIR}${LIBDIR}/libatomickit.so.${MAJOR}
	ln -frs ${DESTDIR}${LIBDIR}/libatomickit.so.${MAJOR}.${MINOR} ${DESTDIR}${LIBDIR}/libatomickit.so

install-static: static
	(umask 022; mkdir -p ${DESTDIR}${LIBDIR})
	install -m 644 libatomickit.a ${DESTDIR}${LIBDIR}/libatomickit.a

install-shared-strip: install-shared
	strip --strip-unneeded ${DESTDIR}${LIBDIR}/libatomickit.so.${VERSION}

install-static-strip: install-static
	strip --strip-unneeded ${DESTDIR}${LIBDIR}/libatomickit.a

install-all-static: static atomickit.pc install-static install-headers install-pkgconfig

install-all-shared: shared atomickit.pc install-shared install-headers install-pkgconfig

install-all-shared-strip: install-all-shared install-shared-strip

install-all-static-strip: install-all-static install-static-strip

install: install-all-shared

install-strip: install-all-shared-strip

uninstall: 
	rm -f ${DESTDIR}${LIBDIR}/libatomickit.so.${VERSION}
	rm -f ${DESTDIR}${LIBDIR}/libatomickit.so
	rm -f ${DESTDIR}${LIBDIR}/libatomickit.a
	rm -f ${DESTDIR}${PKGCONFIGDIR}/atomickit.pc
	rm -rf ${DESTDIR}${INCLUDEDIR}/atomickit

clean:
	rm -f atomickit.pc
	rm -f libatomickit.so
	rm -f libatomickit.a
	rm -f ${OBJS}
	rm -f ${PICOBJS}
	rm -f ${TESTOBJS}
	rm -f unittest-shared
	rm -f unittest-static

check-shared: unittest-shared
	./unittest-shared

check-static: unittest-static
	./unittest-static

check: check-shared
