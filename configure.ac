AC_PREREQ([2.69])
AC_INIT(vlc-plugin-bittorrent, 2.0, johan.gunnarsson@gmail.com, vlc-bittorrent, https://github.com/johang/vlc-bittorrent)
AC_CONFIG_SRCDIR([src/module.cpp])

# Check programs
AC_PROG_CXX

# Check automake
AM_INIT_AUTOMAKE

# Check libraries
PKG_CHECK_MODULES(VLC_PLUGIN, [vlc-plugin >= 3.0.0])
PKG_CHECK_MODULES(LIBTORRENT, libtorrent-rasterbar >= 1.0.0)

# Check libtool
LT_INIT

AC_OUTPUT(Makefile src/Makefile)
