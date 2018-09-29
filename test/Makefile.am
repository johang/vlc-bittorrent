TESTS = metadata-file.test
TEST_LOG_COMPILE = $(SHELL)
TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/build-aux/tap-driver.sh

# Support programs
check_PROGRAMS = vlcdummy
vlcdummy_SOURCES = vlcdummy.c
vlcdummy_CFLAGS = $(LIBVLC_CFLAGS) -Wextra -Wconversion -Wsign-compare -Wsign-conversion
vlcdummy_LDADD = $(LIBVLC_LIBS)
