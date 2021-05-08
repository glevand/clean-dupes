AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

LIBTOOL_DEPS = @LIBTOOL_DEPS@

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool

noinst_LTLIBRARIES = libclean.la

noinst_HEADERS = digest.h \
 hash-table.h \
 list.h \
 log.h \
 mmap.h \
 mem.h \
 thread-pool.h \
 timer.h \
 util.h \
 work-queue.h

# version-info rules (current:revision:age):
# * If the library source code has changed since the last release, then
#   increment revision.
# * If any interfaces have been added, removed, or changed since the
#   last release, then increment current and set revision to 0.
# * If any interfaces have been added since the last release, then
#   increment age.
# * If any interfaces have been removed since the last release, then
#   set age to 0.

libclean_la_DEPENDENCIES = Makefile Makefile.am configure.ac
libclean_la_SOURCES = \
 digest.c digest.h \
 hash-table.c hash-table.h \
 list.c list.h \
 log.c log.h \
 mem.c mem.h \
 mmap.c mmap.h \
 thread-pool.c thread-pool.h \
 timer.c timer.h \
 util.c util.h \
 work-queue.c work-queue.h
libclean_la_LDFLAGS = -version-info 1:0:0 ${EXTRA_LDFLAGS}

AM_CPPFLAGS = $(DEFAULT_CPPFLAGS)
AM_CFLAGS = $(DEFAULT_CFLAGS)

EXTRA_DIST = bootstrap configure.ac COPYING m4 mit-plus-license.txt README.md \
	version.sh

MAINTAINERCLEANFILES = autom4te.cache aclocal.m4 compile config.* configure \
 depcomp install-sh ltmain.sh m4/*.m4 Makefile.in missing $(PACKAGE)-*.gz

maintainer-clean-local:
	rm -rf m4

.PHONY: help

help:
	@echo "Targets:"
	@echo "  make install"
	@echo "  make dist"
	@echo "  make distcheck"
	@echo "  make distclean"
	@echo "  make maintainer-clean"
