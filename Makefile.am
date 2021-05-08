AUTOMAKE_OPTIONS = foreign

SUBDIRS = lib

ACLOCAL_AMFLAGS = -I m4

composed_bin_scripts = clean-dupes.sh
composed_lib_scripts = 

bin_SCRIPTS = $(composed_bin_scripts) $(composed_lib_scripts)

edit = sed \
	-e 's|@PACKAGE\@|$(PACKAGE)|g' \
	-e 's|@PACKAGE_BUGREPORT\@|$(PACKAGE_BUGREPORT)|g' \
	-e 's|@PACKAGE_NAME\@|$(PACKAGE_NAME)|g' \
	-e 's|@PACKAGE_URL\@|$(PACKAGE_URL)|g' \
	-e 's|@PACKAGE_VERSION\@|$(PACKAGE_VERSION)|g' \
	-e 's|@datadir\@|$(pkgdatadir)|g' \
	-e 's|@libexecdir\@|$(libexecdir)|g' \
	-e 's|@prefix\@|$(prefix)|g'

$(composed_bin_scripts): % : Makefile $(srcdir)/%.in
	rm -f $@ $@.tmp
	mkdir -p $$(dirname $@)
	$(edit) $(srcdir)/$@.in > $@.tmp
	chmod +x $@.tmp
	mv $@.tmp $@

$(composed_lib_scripts): % : Makefile $(srcdir)/%.in
	rm -f $@ $@.tmp
	mkdir -p $$(dirname $@)
	$(edit) $(srcdir)/$@.in > $@.tmp
	chmod -x $@.tmp
	mv $@.tmp $@

LIBTOOL_DEPS = @LIBTOOL_DEPS@

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool

bin_PROGRAMS = find-dupes

find_dupes_DEPENDENCIES = Makefile Makefile.am configure.ac
find_dupes_SOURCES = \
	compare.c compare.h \
	find.c find.h \
	list-file.c list-file.h \
	find-dupes.c
find_dupes_LDADD = lib/libclean.la -lssl -lcrypto -lpthread $(MMHASH_LIBS)

AM_CPPFLAGS = -I$(srcdir)/lib $(DEFAULT_CPPFLAGS)
AM_CFLAGS = $(DEFAULT_CFLAGS)
AM_LDFLAGS = -Llib

EXTRA_DIST = bootstrap configure.ac COPYING m4 mit-plus-license.txt README.md \
	version.sh $(addsuffix .in, $(composed_bin_scripts)) \
	$(nobase_bin_SCRIPTS)

CLEANFILES = $(bin_SCRIPTS)

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
