.PHONY: shared static all install-headers install-pkgconfig install-shared install-static install-all-static install-all-shared install install-strip uninstall clean

.SUFFIXES: .o .pic.o

include config.mk

VERSION=0.3

OBJS=src/rcp.o src/queue.o src/malloc.o src/txn.o src/array.o src/string.o
PICOBJS=src/rcp.pic.o src/queue.pic.o src/malloc.pic.o src/txn.pic.o \
        src/array.pic.o src/string.pic.o
TESTOBJS=test/main.o test/test_atomic_array_h.o test/test_atomic_float_h.o \
         test/test_atomic_h.o test/test_atomic_malloc_h.o \
         test/test_atomic_pointer_h.o test/test_atomic_queue_h.o \
         test/test_atomic_rcp_h.o test/test_atomic_txn_h.o test/test.o
HEADERS=include/atomickit/atomic.h \
        include/atomickit/atomic-float.h \
        include/atomickit/atomic-pointer.h \
        include/atomickit/atomic-rcp.h \
        include/atomickit/atomic-queue.h \
        include/atomickit/atomic-malloc.h \
        include/atomickit/atomic-txn.h \
        include/atomickit/atomic-array.h \
        include/atomickit/atomic-string.h
ARCHHEADERS=include/${ARCH}/atomickit/arch/atomic.h \
            include/${ARCH}/atomickit/arch/misc.h

all: shared atomickit.pc

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

.c.pic.o:
	${CC} ${CFLAGS} -fPIC -c $< -o $@

libatomickit.so: ${PICOBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -fPIC -shared ${PICOBJS} -o libatomickit.so

libatomickit.a: ${OBJS}
	rm -f libatomickit.a
	${AR} ${ARFLAGS}c libatomickit.a ${OBJS}

unittest-shared: libatomickit.so ${TESTOBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -L`pwd` -Wl,-rpath,`pwd` -latomickit ${TESTOBJS} -o unittest-shared

unittest-static: libatomickit.a ${TESTOBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -static -L`pwd` ${TESTOBJS} -o unittest-static -latomickit

atomickit.pc: atomickit.pc.in config.mk Makefile
	sed -e 's!@prefix@!${PREFIX}!g' \
	    -e 's!@libdir@!${LIBDIR}!g' \
	    -e 's!@includedir@!${INCLUDEDIR}!g' \
	    -e 's!@version@!${VERSION}!g' \
	    atomickit.pc.in >atomickit.pc

shared: libatomickit.so

static: libatomickit.a

install-headers:
	mkdir -p ${INCLUDEDIR}/atomickit/arch
	install -m 644 -t ${INCLUDEDIR}/atomickit ${HEADERS}
	install -m 644 -t ${INCLUDEDIR}/atomickit/arch ${ARCHHEADERS}

install-pkgconfig: atomickit.pc
	mkdir -p ${PKGCONFIGDIR}
	install -m 644 atomickit.pc ${PKGCONFIGDIR}/atomickit.pc

install-shared: shared
	mkdir -p ${LIBDIR}
	install -m 755 libatomickit.so ${LIBDIR}/libatomickit.so.${VERSION}
	ln -fs ${LIBDIR}/libatomickit.so.${VERSION} ${LIBDIR}/libatomickit.so

install-static: static
	mkdir -p ${LIBDIR}
	install -m 644 libatomickit.a ${LIBDIR}/libatomickit.a

install-all-static: static atomickit.pc install-static install-headers install-pkgconfig

install-all-shared: shared atomickit.pc install-shared install-headers install-pkgconfig

install: install-all-shared

install-strip: install
	strip --strip-unneeded ${LIBDIR}/libatomickit.so.${VERSION}

uninstall: 
	rm -f ${LIBDIR}/libatomickit.so.${VERSION}
	rm -f ${LIBDIR}/libatomickit.so
	rm -f ${LIBDIR}/libatomickit.a
	rm -f ${PKGCONFIGDIR}/atomickit.pc
	rm -rf ${INCLUDEDIR}/atomickit

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
