TESTS = metadata.test data.test
TEST_LOG_COMPILE = $(SHELL)
TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/build-aux/tap-driver.sh

# Support programs
check_PROGRAMS = vlcdummy miniclient
vlcdummy_SOURCES = vlcdummy.c
vlcdummy_CFLAGS = $(LIBVLC_CFLAGS) -Wextra -Wconversion -Wsign-compare -Wsign-conversion
vlcdummy_LDADD = $(LIBVLC_LIBS)
miniclient_SOURCES = miniclient.cpp
miniclient_CXXFLAGS = $(LIBTORRENT_CFLAGS) -Wextra -Wconversion -Wsign-compare -Wsign-conversion
miniclient_LDADD = $(LIBTORRENT_LIBS)
