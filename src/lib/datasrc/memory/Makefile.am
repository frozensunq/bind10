SUBDIRS = . benchmarks

AM_CPPFLAGS = -I$(top_srcdir)/src/lib -I$(top_builddir)/src/lib
AM_CPPFLAGS += -I$(top_srcdir)/src/lib/dns -I$(top_builddir)/src/lib/dns
AM_CPPFLAGS += $(BOOST_INCLUDES)

AM_CXXFLAGS = $(B10_CXXFLAGS)

CLEANFILES = *.gcno *.gcda

noinst_LTLIBRARIES = libdatasrc_memory.la

libdatasrc_memory_la_SOURCES = domaintree.h
libdatasrc_memory_la_SOURCES += rdataset.h rdataset.cc
libdatasrc_memory_la_SOURCES += treenode_rrset.h treenode_rrset.cc
libdatasrc_memory_la_SOURCES += rdata_serialization.h rdata_serialization.cc
libdatasrc_memory_la_SOURCES += zone_data.h zone_data.cc
libdatasrc_memory_la_SOURCES += rrset_collection.h rrset_collection.cc
libdatasrc_memory_la_SOURCES += segment_object_holder.h
libdatasrc_memory_la_SOURCES += segment_object_holder.cc
libdatasrc_memory_la_SOURCES += logger.h logger.cc
libdatasrc_memory_la_SOURCES += zone_table.h zone_table.cc
libdatasrc_memory_la_SOURCES += zone_finder.h zone_finder.cc
libdatasrc_memory_la_SOURCES += zone_table_segment.h zone_table_segment.cc
libdatasrc_memory_la_SOURCES += zone_table_segment_local.h zone_table_segment_local.cc

if USE_SHARED_MEMORY
libdatasrc_memory_la_SOURCES += zone_table_segment_mapped.h zone_table_segment_mapped.cc
endif

libdatasrc_memory_la_SOURCES += zone_data_updater.h zone_data_updater.cc
libdatasrc_memory_la_SOURCES += zone_data_loader.h zone_data_loader.cc
libdatasrc_memory_la_SOURCES += memory_client.h memory_client.cc
libdatasrc_memory_la_SOURCES += zone_writer.h zone_writer.cc
libdatasrc_memory_la_SOURCES += load_action.h
libdatasrc_memory_la_SOURCES += util_internal.h

nodist_libdatasrc_memory_la_SOURCES = memory_messages.h memory_messages.cc

EXTRA_DIST  = rdata_serialization_priv.cc

BUILT_SOURCES = memory_messages.h memory_messages.cc
memory_messages.h memory_messages.cc: s-messages

s-messages: Makefile memory_messages.mes
	$(top_builddir)/src/lib/log/compiler/message $(top_srcdir)/src/lib/datasrc/memory/memory_messages.mes
	touch $@

EXTRA_DIST += memory_messages.mes
CLEANFILES += memory_messages.h memory_messages.cc s-messages
