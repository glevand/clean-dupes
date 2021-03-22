AUTOMAKE_OPTIONS = foreign

SUBDIRS = lib

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I$(srcdir)/lib $(DEFAULT_CPPFLAGS)
AM_CFLAGS = $(DEFAULT_CFLAGS)
AM_LDFLAGS = -Llib

EXTRA_DIST = bootstrap version.sh configure.ac $(srcdir)/m4

MAINTAINERCLEANFILES = autom4te.cache aclocal.m4 compile config.* configure \
 depcomp install-sh ltmain.sh m4/*.m4 Makefile.in missing $(PACKAGE)-*.gz

maintainer-clean-local:
	rm -rf m4

LIBTOOL_DEPS = @LIBTOOL_DEPS@

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool

dist_bin_SCRIPTS = clean-dupes.sh

bin_PROGRAMS = find-dupes

find_dupes_DEPENDENCIES = Makefile Makefile.am configure.ac
find_dupes_SOURCES = \
	compare.c compare.h \
	find.c find.h \
	list-file.c list-file.h \
	find-dupes.c
find_dupes_LDADD = lib/libclean.la -lssl -lcrypto -lpthread $(MMHASH_LIBS)

.PHONY: help

help:
	@echo "Targets:"
	@echo "  make install"
	@echo "  make dist"
	@echo "  make distcheck"
	@echo "  make distclean"
	@echo "  make maintainer-clean"
