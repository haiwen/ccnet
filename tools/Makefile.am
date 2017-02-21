
AM_CPPFLAGS = -I$(top_srcdir)/include @GLIB2_CFLAGS@ -I$(top_srcdir)/lib

bin_PROGRAMS = ccnet-init

ccnet_init_SOURCES = ccnet-init.c

ccnet_init_LDADD = @SSL_LIBS@ @LIB_RESOLV@ @GLIB2_LIBS@ \
	$(top_builddir)/lib/libccnetd.la

