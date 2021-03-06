SUBDIRS = . tests

dhcp_data_dir = @localstatedir@/@PACKAGE@

AM_CPPFLAGS = -I$(top_builddir)/src/lib -I$(top_srcdir)/src/lib -DDHCP_DATA_DIR="\"$(dhcp_data_dir)\""
AM_CPPFLAGS += $(BOOST_INCLUDES)
if HAVE_MYSQL
AM_CPPFLAGS += $(MYSQL_CPPFLAGS)
endif

AM_CXXFLAGS = $(B10_CXXFLAGS)

# Define rule to build logging source files from message file
dhcpsrv_messages.h dhcpsrv_messages.cc: s-messages

s-messages: dhcpsrv_messages.mes
	$(top_builddir)/src/lib/log/compiler/message $(top_srcdir)/src/lib/dhcpsrv/dhcpsrv_messages.mes
	touch $@

# Tell Automake that the dhcpsrv_messages.{cc,h} source files are created in the
# build process, so it must create these before doing anything else.  Although
# they are a dependency of the library (so will be created from the message file
# anyway), there is no guarantee as to exactly _when_ in the build they will be
# created.  As the .h file is included in other sources file (so must be
# present when they are compiled), the safest option is to create it first.
BUILT_SOURCES = dhcpsrv_messages.h dhcpsrv_messages.cc

# Some versions of GCC warn about some versions of Boost regarding
# missing initializer for members in its posix_time.
# https://svn.boost.org/trac/boost/ticket/3477
# But older GCC compilers don't have the flag.
AM_CXXFLAGS += $(WARNING_NO_MISSING_FIELD_INITIALIZERS_CFLAG)

# Make sure the generated files are deleted in a "clean" operation
CLEANFILES = *.gcno *.gcda dhcpsrv_messages.h dhcpsrv_messages.cc s-messages

lib_LTLIBRARIES = libb10-dhcpsrv.la
libb10_dhcpsrv_la_SOURCES  =
libb10_dhcpsrv_la_SOURCES += addr_utilities.cc addr_utilities.h
libb10_dhcpsrv_la_SOURCES += alloc_engine.cc alloc_engine.h
libb10_dhcpsrv_la_SOURCES += callout_handle_store.h
libb10_dhcpsrv_la_SOURCES += d2_client_cfg.cc d2_client_cfg.h
libb10_dhcpsrv_la_SOURCES += d2_client_mgr.cc d2_client_mgr.h
libb10_dhcpsrv_la_SOURCES += dbaccess_parser.cc dbaccess_parser.h
libb10_dhcpsrv_la_SOURCES += dhcpsrv_log.cc dhcpsrv_log.h
libb10_dhcpsrv_la_SOURCES += cfgmgr.cc cfgmgr.h
libb10_dhcpsrv_la_SOURCES += dhcp_config_parser.h
libb10_dhcpsrv_la_SOURCES += dhcp_parsers.cc dhcp_parsers.h 
libb10_dhcpsrv_la_SOURCES += key_from_key.h
libb10_dhcpsrv_la_SOURCES += lease.cc lease.h
libb10_dhcpsrv_la_SOURCES += lease_mgr.cc lease_mgr.h
libb10_dhcpsrv_la_SOURCES += lease_mgr_factory.cc lease_mgr_factory.h
libb10_dhcpsrv_la_SOURCES += memfile_lease_mgr.cc memfile_lease_mgr.h
if HAVE_MYSQL
libb10_dhcpsrv_la_SOURCES += mysql_lease_mgr.cc mysql_lease_mgr.h
endif
libb10_dhcpsrv_la_SOURCES += option_space_container.h
libb10_dhcpsrv_la_SOURCES += pool.cc pool.h
libb10_dhcpsrv_la_SOURCES += subnet.cc subnet.h
libb10_dhcpsrv_la_SOURCES += triplet.h
libb10_dhcpsrv_la_SOURCES += utils.h

nodist_libb10_dhcpsrv_la_SOURCES = dhcpsrv_messages.h dhcpsrv_messages.cc

libb10_dhcpsrv_la_CXXFLAGS = $(AM_CXXFLAGS)
libb10_dhcpsrv_la_CPPFLAGS = $(AM_CPPFLAGS) $(LOG4CPLUS_INCLUDES)
libb10_dhcpsrv_la_LIBADD   = $(top_builddir)/src/lib/asiolink/libb10-asiolink.la
libb10_dhcpsrv_la_LIBADD  += $(top_builddir)/src/lib/dhcp/libb10-dhcp++.la
libb10_dhcpsrv_la_LIBADD  += $(top_builddir)/src/lib/dhcp_ddns/libb10-dhcp_ddns.la
libb10_dhcpsrv_la_LIBADD  += $(top_builddir)/src/lib/hooks/libb10-hooks.la
libb10_dhcpsrv_la_LIBADD  += $(top_builddir)/src/lib/log/libb10-log.la
libb10_dhcpsrv_la_LIBADD  += $(top_builddir)/src/lib/util/libb10-util.la
libb10_dhcpsrv_la_LIBADD  += $(top_builddir)/src/lib/cc/libb10-cc.la
libb10_dhcpsrv_la_LIBADD  += $(top_builddir)/src/lib/hooks/libb10-hooks.la

libb10_dhcpsrv_la_LDFLAGS  = -no-undefined -version-info 3:0:0
if HAVE_MYSQL
libb10_dhcpsrv_la_LDFLAGS += $(MYSQL_LIBS)
endif

if USE_CLANGPP
# Disable unused parameter warning caused by some of the
# Boost headers when compiling with clang.
libb10_dhcpsrv_la_CXXFLAGS += -Wno-unused-parameter
endif

# The message file should be in the distribution
EXTRA_DIST = dhcpsrv_messages.mes

# Distribute MySQL schema creation script and backend documentation
EXTRA_DIST += dhcpdb_create.mysql database_backends.dox libdhcpsrv.dox
dist_pkgdata_DATA = dhcpdb_create.mysql

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(dhcp_data_dir)


