TESTS = data.test metadata.test downloaddummy.test
TEST_LOG_COMPILE = $(SHELL)
TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/build-aux/tap-driver.sh
AUTOMAKE_OPTIONS = subdir-objects

# Great and cool CFLAGS
COOLCFLAGS = \
	-Wno-deprecated-declarations \
	-Wall \
	-Wextra \
	-Wconversion \
	-Wsign-compare \
	-Wsign-conversion \
	-Wno-unused-parameter \
	-Wno-unused

# Great and cool CXXFLAGS
COOLCXXFLAGS = \
	-std=c++11 \
	$(COOLCFLAGS)

# Support programs
check_PROGRAMS = vlcdummy miniclient downloaddummy
vlcdummy_SOURCES = vlcdummy.c
vlcdummy_CFLAGS = $(LIBVLC_CFLAGS) $(COOLCFLAGS)
vlcdummy_LDFLAGS =
vlcdummy_LDADD = $(LIBVLC_LIBS)
miniclient_SOURCES = miniclient.cpp
miniclient_CXXFLAGS = $(LIBTORRENT_CFLAGS) $(COOLCXXFLAGS)
miniclient_LDFLAGS =
miniclient_LDADD = $(LIBTORRENT_LIBS) -lpthread
downloaddummy_SOURCES = downloaddummy.cpp ../src/download.cpp ../src/libtorrent.cpp
downloaddummy_CXXFLAGS = -I../src $(LIBTORRENT_CFLAGS) $(VLC_PLUGIN_CFLAGS) $(COOLCXXFLAGS)
downloaddummy_LDFLAGS = -lpthread
downloaddummy_LDADD = $(LIBTORRENT_LIBS) $(VLC_PLUGIN_LIBS)
