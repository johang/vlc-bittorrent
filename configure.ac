AC_PREREQ([2.69])
AC_INIT(vlc-plugin-bittorrent, 2.5, johan.gunnarsson@gmail.com, vlc-bittorrent, https://github.com/johang/vlc-bittorrent)
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
PKG_CHECK_MODULES(LIBTORRENT, libtorrent-rasterbar >= 1.0.0)
PKG_CHECK_MODULES(LIBVLC, libvlc >= 3.0.0)

# Check libtool
LT_INIT

AC_CONFIG_HEADERS(src/config.h)
AC_OUTPUT(Makefile src/Makefile test/Makefile data/Makefile)
