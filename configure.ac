AC_PREREQ([2.69])
AC_INIT(vlc-plugin-bittorrent, 2.15, johan.gunnarsson@gmail.com, vlc-bittorrent, https://github.com/johang/vlc-bittorrent)
AC_CONFIG_SRCDIR([src/module.cpp])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AC_REQUIRE_AUX_FILE([tap-driver.sh])

# Check programs
AC_PROG_CXX
AC_PROG_AWK

# Check automake
AM_INIT_AUTOMAKE

# Check libraries
PKG_CHECK_MODULES(VLC_PLUGIN, vlc-plugin >= 3.0.0)
PKG_CHECK_MODULES(LIBTORRENT, libtorrent-rasterbar >= 2.0.0)

# Check libvlc library only if --with-tests
AC_ARG_WITH(
  [tests],
  [AS_HELP_STRING([--with-tests], [enable tests])],
  [with_tests=yes])
AM_CONDITIONAL([WITH_TESTS], [test x$with_tests = xyes])
AM_COND_IF([WITH_TESTS], [PKG_CHECK_MODULES(LIBVLC, libvlc >= 3.0.0)])

# Compile with -std=c++14 or later
AX_CXX_COMPILE_STDCXX_14(noext, mandatory)

# Check libtool
LT_INIT

AC_CONFIG_HEADERS(src/config.h)
AC_CONFIG_FILES([Makefile src/Makefile test/Makefile data/Makefile])
AC_OUTPUT
