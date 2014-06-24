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
	python $(MAKE_DEMO) tests

EXTRA_DIST = install-sh libccnet.pc.in LICENCE.txt

DISTCHECK_CONFIGURE_FLAGS = --enable-server

ACLOCAL_AMFLAGS = -I m4

dist-hook:
	git log --format='%H' -1 > $(distdir)/latest_commit
