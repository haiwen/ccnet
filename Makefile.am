## Process this file with automake to produce Makefile.in


pcfiles = libccnet.pc
pkgconfig_DATA = $(pcfiles)
pkgconfigdir = $(libdir)/pkgconfig

MAKE_CLI = cli
MAKE_TOOLS = tools

if COMPILE_DEMO
  MAKE_DEMO = demo
endif


SUBDIRS = include lib net $(MAKE_CLI) $(MAKE_TOOLS) \
	$(MAKE_DEMO) tests

if COMPILE_PYTHON
  SUBDIRS += python
endif

EXTRA_DIST = install-sh libccnet.pc.in LICENCE.txt

DISTCHECK_CONFIGURE_FLAGS = --enable-server

ACLOCAL_AMFLAGS = -I m4

install-data-local:
if MACOS
	sed -i '' -e "s|(DESTDIR)|${DESTDIR}|g" $(pcfiles)
else
	sed -i "s|(DESTDIR)|${DESTDIR}|g" $(pcfiles)
endif

dist-hook:
	git log -1 > $(distdir)/latest_commit
