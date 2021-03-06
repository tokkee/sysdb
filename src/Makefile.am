SUBDIRS = liboconfig

# coverage related flags are only set when gcov is enabled
AM_CFLAGS = @STRICT_CFLAGS@ @COVERAGE_CFLAGS@ @PROFILING_CFLAGS@
AM_CXXFLAGS = @STRICT_CXXFLAGS@ @COVERAGE_CFLAGS@ @PROFILING_CFLAGS@
AM_LDFLAGS = @COVERAGE_LDFLAGS@ @PROFILING_LDFLAGS@
AM_CPPFLAGS  = -I${top_builddir}/src/include -I${top_srcdir}/src/include
AM_CPPFLAGS += -DSYSCONFDIR='"${sysconfdir}"'
AM_CPPFLAGS += -DLOCALSTATEDIR='"${localstatedir}"'
AM_CPPFLAGS += -DPKGLIBDIR='"${pkglibdir}"'

V_LFLAGS = $(v_lflags_@AM_V@)
v_lflags_ = $(v_lflags_@AM_DEFAULT_V@)
v_lflags_1 = -v

AM_LFLAGS = $(V_LFLAGS)
AM_YFLAGS = -d

BUILT_SOURCES = include/client/sysdb.h include/sysdb.h \
		parser/grammar.h
EXTRA_DIST = include/client/sysdb.h.in include/sysdb.h.in

pkginclude_HEADERS = include/sysdb.h
pkgcoreincludedir = $(pkgincludedir)/core
pkgcoreinclude_HEADERS = \
		include/core/data.h \
		include/core/object.h \
		include/core/plugin.h \
		include/core/store.h \
		include/core/time.h \
		include/core/timeseries.h
pkgfeincludedir = $(pkgincludedir)/frontend
pkgfeinclude_HEADERS = \
		include/frontend/connection.h \
		include/frontend/proto.h \
		include/frontend/sock.h
pkgutilsincludedir = $(pkgincludedir)/utils
pkgutilsinclude_HEADERS = \
		include/utils/avltree.h \
		include/utils/channel.h \
		include/utils/dbi.h \
		include/utils/error.h \
		include/utils/llist.h \
		include/utils/os.h \
		include/utils/proto.h \
		include/utils/ssl.h \
		include/utils/strbuf.h \
		include/utils/strings.h \
		include/utils/unixsock.h

pkgclientincludedir = $(pkgincludedir)/client
pkgclientinclude_HEADERS = \
		include/client/sock.h \
		include/client/sysdb.h

noinst_LTLIBRARIES = libsysdbclient.la libsysdb.la

libsysdbclient_la_SOURCES = \
		core/data.c include/core/data.h \
		core/time.c include/core/time.h \
		client/client.c include/client/sysdb.h \
		client/sock.c include/client/sock.h \
		utils/error.c include/utils/error.h \
		utils/proto.c include/utils/proto.h \
		utils/ssl.c include/utils/ssl.h \
		utils/strbuf.c include/utils/strbuf.h \
		utils/strings.c include/utils/strings.h
libsysdbclient_la_CFLAGS = $(AM_CFLAGS) @OPENSSL_CFLAGS@
libsysdbclient_la_CPPFLAGS = $(AM_CPPFLAGS) $(LTDLINCL)
libsysdbclient_la_LDFLAGS = $(AM_LDFLAGS) -version-info 0:0:0 \
		-pthread -lm -lrt
libsysdbclient_la_LIBADD = $(LIBLTDL) @OPENSSL_LIBS@

# don't use strict CFLAGS for flex code
noinst_LTLIBRARIES += libsysdb_fe_parser.la
libsysdb_fe_parser_la_SOURCES = \
		parser/grammar.y parser/scanner.l
libsysdb_fe_parser_la_CFLAGS = @COVERAGE_CFLAGS@ @PROFILING_CFLAGS@ \
		-DBUILD_DATE="\"$$( date --utc '+%F %T' ) (UTC)\""
libsysdb_la_SOURCES = \
		sysdb.c include/sysdb.h \
		core/data.c include/core/data.h \
		core/memstore.c include/core/memstore.h \
		core/memstore-private.h \
		core/memstore_exec.c \
		core/memstore_expr.c \
		core/memstore_lookup.c \
		core/memstore_query.c \
		core/object.c include/core/object.h \
		core/plugin.c include/core/plugin.h \
		core/store_json.c include/core/store.h \
		core/time.c include/core/time.h \
		core/timeseries.c include/core/timeseries.h \
		frontend/connection.c include/frontend/connection.h \
		frontend/connection-private.h \
		frontend/sock.c include/frontend/sock.h \
		frontend/session.c \
		frontend/query.c \
		parser/analyzer.c \
		parser/ast.c include/parser/ast.h \
		parser/parser.c include/parser/parser.h \
		utils/avltree.c include/utils/avltree.h \
		utils/channel.c include/utils/channel.h \
		utils/error.c include/utils/error.h \
		utils/llist.c include/utils/llist.h \
		utils/os.c include/utils/os.h \
		utils/proto.c include/utils/proto.h \
		utils/ssl.c include/utils/ssl.h \
		utils/strbuf.c include/utils/strbuf.h \
		utils/strings.c include/utils/strings.h \
		utils/unixsock.c include/utils/unixsock.h
libsysdb_la_CFLAGS = $(AM_CFLAGS) @OPENSSL_CFLAGS@
libsysdb_la_CPPFLAGS = $(AM_CPPFLAGS) $(LTDLINCL)
libsysdb_la_LDFLAGS = $(AM_LDFLAGS) -version-info 0:0:0 \
		-pthread -lm -lrt
libsysdb_la_LIBADD = libsysdb_fe_parser.la \
		$(LIBLTDL) liboconfig/liboconfig.la @OPENSSL_LIBS@
libsysdb_la_DEPENDENCIES = libsysdb_fe_parser.la liboconfig/liboconfig.la

if BUILD_WITH_LIBDBI
libsysdb_la_SOURCES += \
		utils/dbi.c include/utils/dbi.h
libsysdb_la_LIBADD += -ldbi
endif

bin_PROGRAMS = sysdbd
if BUILD_CLIENT
bin_PROGRAMS += sysdb

# don't use strict CFLAGS for flex code
noinst_LTLIBRARIES += libsysdb_scanner.la
libsysdb_scanner_la_SOURCES = tools/sysdb/scanner.l
libsysdb_scanner_la_CFLAGS = @COVERAGE_CFLAGS@ @PROFILING_CFLAGS@ \
		-DBUILD_DATE="\"$$( date --utc '+%F %T' ) (UTC)\""
sysdb_SOURCES = tools/sysdb/main.c include/client/sysdb.h \
		tools/sysdb/command.c tools/sysdb/command.h \
		tools/sysdb/input.c tools/sysdb/input.h \
		tools/sysdb/json.c tools/sysdb/json.h \
		core/object.c include/core/object.h \
		utils/llist.c include/utils/llist.h \
		utils/os.c include/utils/os.h
sysdb_CFLAGS = -DBUILD_DATE="\"$$( date --utc '+%F %T' ) (UTC)\"" \
		$(AM_CFLAGS) @READLINE_CFLAGS@ @YAJL_CFLAGS@
sysdb_LDADD = libsysdb_scanner.la libsysdbclient.la \
		@READLINE_LIBS@ @YAJL_LIBS@ -lm
endif

sysdbd_SOURCES = tools/sysdbd/main.c include/sysdb.h \
		tools/sysdbd/configfile.c tools/sysdbd/configfile.h \
		$(libsysdb_la_SOURCES)
sysdbd_CFLAGS = $(AM_CFLAGS) @OPENSSL_CFLAGS@ \
		-DBUILD_DATE="\"$$( date --utc '+%F %T' ) (UTC)\""
sysdbd_CPPFLAGS = $(AM_CPPFLAGS) $(LTDLINCL)
sysdbd_LDFLAGS = $(AM_LDFLAGS) -export-dynamic -pthread -lm
sysdbd_LDADD = libsysdb_fe_parser.la liboconfig/liboconfig.la \
		$(LIBLTDL) -lrt @OPENSSL_LIBS@
sysdbd_DEPENDENCIES = libsysdb_fe_parser.la liboconfig/liboconfig.la

if BUILD_WITH_LIBDBI
sysdbd_LDADD += -ldbi
endif

sdbconfdir = $(sysconfdir)/sysdb
dist_sdbconf_DATA = tools/sysdbd/sysdbd.conf.sample

pkgbackendlibdir = $(pkglibdir)/backend
pkgbackendcollectdlibdir = $(pkgbackendlibdir)/collectd
pkgbackendpuppetlibdir = $(pkgbackendlibdir)/puppet
pkgcnamelibdir = $(pkglibdir)/cname
pkgtimeserieslibdir = $(pkglibdir)/timeseries
pkgstorelibdir = $(pkglibdir)/store

pkglib_LTLIBRARIES =
pkgbackendlib_LTLIBRARIES =
pkgbackendcollectdlib_LTLIBRARIES =
pkgbackendpuppetlib_LTLIBRARIES =
pkgcnamelib_LTLIBRARIES =
pkgtimeserieslib_LTLIBRARIES =
pkgstorelib_LTLIBRARIES =

if BUILD_PLUGIN_CNAMEDNS
pkgcnamelib_LTLIBRARIES += plugins/cname/dns.la
plugins_cname_dns_la_SOURCE = plugins/cname/dns.c
plugins_cname_dns_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version
sysdbd_LDADD += -dlopen plugins/cname/dns.la
sysdbd_DEPENDENCIES += plugins/cname/dns.la
endif

if BUILD_PLUGIN_COLLECTDUNIXSOCK
pkgbackendcollectdlib_LTLIBRARIES += plugins/backend/collectd/unixsock.la
plugins_backend_collectd_unixsock_la_SOURCES = plugins/backend/collectd/unixsock.c
plugins_backend_collectd_unixsock_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version
sysdbd_LDADD += -dlopen plugins/backend/collectd/unixsock.la
sysdbd_DEPENDENCIES += plugins/backend/collectd/unixsock.la
endif

if BUILD_PLUGIN_FACTER
pkgbackendlib_LTLIBRARIES += plugins/backend/facter.la
plugins_backend_facter_la_SOURCES = plugins/backend/facter.cc
plugins_backend_facter_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version -lfacter
sysdbd_LDADD += -dlopen plugins/backend/facter.la
sysdbd_DEPENDENCIES += plugins/backend/facter.la
endif

if BUILD_PLUGIN_MKLIVESTATUS
pkgbackendlib_LTLIBRARIES += plugins/backend/mk-livestatus.la
plugins_backend_mk_livestatus_la_SOURCES = plugins/backend/mk-livestatus.c
plugins_backend_mk_livestatus_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version
sysdbd_LDADD += -dlopen plugins/backend/mk-livestatus.la
sysdbd_DEPENDENCIES += plugins/backend/mk-livestatus.la
endif

if BUILD_PLUGIN_PUPPETSTORECONFIGS
pkgbackendpuppetlib_LTLIBRARIES += plugins/backend/puppet/store-configs.la
plugins_backend_puppet_store_configs_la_SOURCES = plugins/backend/puppet/store-configs.c
plugins_backend_puppet_store_configs_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version
sysdbd_LDADD += -dlopen plugins/backend/puppet/store-configs.la
sysdbd_DEPENDENCIES += plugins/backend/puppet/store-configs.la
endif

if BUILD_PLUGIN_SYSLOG
pkglib_LTLIBRARIES += plugins/syslog.la
plugins_syslog_la_SOURCE = plugins/syslog.c
plugins_syslog_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version
sysdbd_LDADD += -dlopen plugins/syslog.la
sysdbd_DEPENDENCIES += plugins/syslog.la
endif

if BUILD_PLUGIN_TIMESERIESRRDTOOL
pkgtimeserieslib_LTLIBRARIES += plugins/timeseries/rrdtool.la
plugins_timeseries_rrdtool_la_SOURCES = plugins/timeseries/rrdtool.c
plugins_timeseries_rrdtool_la_CFLAGS = $(AM_CFLAGS) @RRD_CFLAGS@
plugins_timeseries_rrdtool_la_LDFLAGS = $(AM_LDFLAGS) @RRD_LIBS@ -module -avoid-version
sysdbd_LDADD += -dlopen plugins/timeseries/rrdtool.la
sysdbd_DEPENDENCIES += plugins/timeseries/rrdtool.la
endif

if BUILD_PLUGIN_STOREMEMORY
pkgstorelib_LTLIBRARIES += plugins/store/memory.la
plugins_store_memory_la_SOURCES = plugins/store/memory.c
plugins_store_memory_la_LDFLAGS = $(AM_LDFLAGS) -module -avoid-version
sysdbd_LDADD += -dlopen plugins/store/memory.la
sysdbd_DEPENDENCIES += plugins/store/memory.la
endif

if BUILD_PLUGIN_STORENETWORK
pkgstorelib_LTLIBRARIES += plugins/store/network.la
plugins_store_network_la_SOURCES = plugins/store/network.c
plugins_store_network_la_LDFLAGS = $(AM_LDFLAGS) libsysdbclient.la -module -avoid-version
sysdbd_LDADD += -dlopen plugins/store/network.la
sysdbd_DEPENDENCIES += plugins/store/network.la
endif

include/client/sysdb.h: include/client/sysdb.h.in $(abs_top_builddir)/version
	@mkdir -p include/client
	$(AM_V_GEN)source $(abs_top_builddir)/version; sed \
	    -e "s/@SDB_VERSION_MAJOR@/$$VERSION_MAJOR/g" \
	    -e "s/@SDB_VERSION_MINOR@/$$VERSION_MINOR/g" \
	    -e "s/@SDB_VERSION_PATCH@/$$VERSION_PATCH/g" \
	    -e "s/@SDB_VERSION_EXTRA@/$$VERSION_EXTRA/g" \
	    -e "s/@SDB_VERSION_STRING@/$$VERSION_STRING/g" \
	    $(srcdir)/include/client/sysdb.h.in \
	    > include/client/sysdb.h

include/sysdb.h: include/sysdb.h.in $(abs_top_builddir)/version
	@mkdir -p include
	$(AM_V_GEN)source $(abs_top_builddir)/version; sed \
	    -e "s/@SDB_VERSION_MAJOR@/$$VERSION_MAJOR/g" \
	    -e "s/@SDB_VERSION_MINOR@/$$VERSION_MINOR/g" \
	    -e "s/@SDB_VERSION_PATCH@/$$VERSION_PATCH/g" \
	    -e "s/@SDB_VERSION_EXTRA@/$$VERSION_EXTRA/g" \
	    -e "s/@SDB_VERSION_STRING@/$$VERSION_STRING/g" \
	    $(srcdir)/include/sysdb.h.in \
	    > include/sysdb.h

$(abs_top_builddir)/version:
	$(AM_V_GEN)cd .. && make version
