AM_CPPFLAGS  = -I$(top_builddir)/src/lib -I$(top_srcdir)/src/lib
AM_CPPFLAGS += $(BOOST_INCLUDES)
AM_CPPFLAGS += -I$(top_srcdir)/src/lib/server_common
AM_CPPFLAGS += -I$(top_builddir)/src/lib/server_common
AM_CXXFLAGS = $(B10_CXXFLAGS)

AM_LDFLAGS =
if USE_STATIC_LINK
AM_LDFLAGS += -static
endif

# Some versions of GCC warn about some versions of Boost regarding
# missing initializer for members in its posix_time.
# https://svn.boost.org/trac/boost/ticket/3477
# But older GCC compilers don't have the flag.     
AM_CXXFLAGS += $(WARNING_NO_MISSING_FIELD_INITIALIZERS_CFLAG)

if USE_CLANGPP
# see ../Makefile.am
AM_CXXFLAGS += -Wno-unused-parameter
endif

CLEANFILES = *.gcno *.gcda

TESTS =
if HAVE_GTEST
TESTS += run_unittests
run_unittests_SOURCES  = run_unittests.cc
run_unittests_SOURCES += portconfig_unittest.cc

run_unittests_CPPFLAGS = $(AM_CPPFLAGS) $(GTEST_INCLUDES)
run_unittests_LDFLAGS = $(AM_LDFLAGS) $(GTEST_LDFLAGS)
run_unittests_LDADD = $(GTEST_LDADD)

run_unittests_LDADD += $(top_builddir)/src/lib/server_common/libserver_common.la
endif

noinst_PROGRAMS = $(TESTS)