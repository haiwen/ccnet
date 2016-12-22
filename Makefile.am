## Process this file with automake to produce Makefile.in


pcfiles = libccnet.pc
pkgconfig_DATA = $(pcfiles)
pkgconfigdir = $(libdir)/pkgconfig

SUBDIRS = include lib net

EXTRA_DIST = install-sh libccnet.pc.in LICENSE.txt

ACLOCAL_AMFLAGS = -I m4

install-data-local:
if MACOS
	sed -i '' -e "s|(DESTDIR)|${DESTDIR}|g" $(pcfiles)
else
	${SED} -i "s|(DESTDIR)|${DESTDIR}|g" $(pcfiles)
endif

dist-hook:
	git log --format='%H' -1 > $(distdir)/latest_commit
