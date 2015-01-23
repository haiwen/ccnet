
AM_CPPFLAGS = @GLIB2_CFLAGS@ -I$(top_srcdir)/include \
	-I$(top_srcdir)/include/ccnet \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/include \
	-DG_LOG_DOMAIN=\"Ccnet\" \
	@SEARPC_CFLAGS@ \
	@MSVC_CFLAGS@ \
	-Wall

BUILT_SOURCES = gensource

lib_LTLIBRARIES = libccnet.la

noinst_HEADERS = buffer.h \
	packet-io.h  \
	include.h \
	marshal.h \
	peer-common.h \
	string-util.h \
	libccnet_utils.h \
	ccnet-object.h \
	rpc-common.h \
	net.h \
	utils.h \
	bloom-filter.h \
	db.h \
	rsa.h

ccnetincludedir = $(includedir)/ccnet
ccnetinclude_DATA = ccnet-object.h

libccnet_la_CPPFLAGS = $(AM_CPPFLAGS) -DCCNET_LIB @PTHREAD_CFLAGS@

libccnet_la_SOURCES = ccnet-client.c packet-io.c libccnet_utils.c \
	message.c proc-factory.c \
	processor.c buffer.c \
	peer.c sendcmd-proc.c \
	mqclient-proc.c invoke-service-proc.c \
	marshal.c \
	mainloop.c cevent.c timer.c ccnet-session-base.c job-mgr.c \
	rpcserver-proc.c ccnetrpc-transport.c threaded-rpcserver-proc.c \
	ccnetobj.c \
	async-rpc-proc.c ccnet-rpc-wrapper.c \
	client-pool.c

EXTRA_DIST = ccnetobj.vala rpc_table.py

libccnet_la_LDFLAGS = -no-undefined -version-info 0:0:0
libccnet_la_LIBADD = @PTHREAD_LIBS@ @GLIB2_LIBS@ @GOBJECT_LIBS@ @SSL_LIBS@ @LIB_GDI32@ \
				     @LIB_UUID@ @LIB_WS32@ @LIB_PSAPI@ -lsqlite3 \
					 @LIBEVENT_LIBS@ @SEARPC_LIBS@ @LIB_SHELL32@


noinst_LTLIBRARIES = libccnetd.la

libccnetd_la_SOURCES = utils.c db.c job-mgr.c \
	rsa.c bloom-filter.c marshal.c net.c timer.c ccnet-session-base.c \
	ccnetobj.c

libccnetd_la_LDFLAGS = -no-undefined
libccnetd_la_LIBADD = @GLIB2_LIBS@  @GOBJECT_LIBS@ @SSL_LIBS@ @LIB_GDI32@ \
	                  -lsqlite3 @LIBEVENT_LIBS@ @LIB_WS32@ @LIB_UUID@ \
					  @LIB_SHELL32@ @LIB_PSAPI@ @SEARPC_LIBS@


ccnet_object_define = ccnetobj.vala

valac_gen = ccnetobj.c ccnet-object.h

ccnet-client.c: ccnet-object.h

ccnet-object.h: ${ccnet_object_define}
	rm -f $@
	valac --pkg posix ${ccnet_object_define} -C -H ccnet-object.h

ccnetobj.c: ${ccnet_object_define}
	rm -f $@
	valac -C --pkg posix ${ccnet_object_define}

searpc_gen = searpc-signature.h searpc-marshal.h

gensource: ${searpc_gen} ${valac_gen}

rpc_table.stamp: ${top_srcdir}/lib/rpc_table.py
	@rm -f rpc_table.tmp
	@touch rpc_table.tmp
	@echo "[libsearpc]: generating rpc header files"
	@PYTHON@ `which searpc-codegen.py` ${top_srcdir}/lib/rpc_table.py
	@echo "[libsearpc]: done"
	@mv -f rpc_table.tmp $@

${searpc_gen}: rpc_table.stamp

clean-local:
	rm -f ${searpc_gen}
	rm -f rpc_table.pyc
	rm -f rpc_table.stamp
	rm -f rpc_table.tmp
	rm -f ${valac_gen}

CLEANFILES = ${searpc_gen} ${valac_gen}
