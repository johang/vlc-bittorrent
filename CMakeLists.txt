# cmake version of vlc-bittorrent plugin by ori-d (https://github.com/ori-d)
# added support for building on windows system.
cmake_minimum_required (VERSION 3.25)

project ("vlc-bittorrent"
         LANGUAGES      C CXX
         VERSION        2.14.0.1
         HOMEPAGE_URL   "https://github.com/johang/vlc-bittorrent"
         DESCRIPTION    "VLC BitTorrent plugin")

list (APPEND CMAKE_MODULE_PATH
      "${CMAKE_CURRENT_LIST_DIR}/.cmake")

include (config_checks)
include (cm_cxx14_check)

# This value can be changed with CMake GUI mode.
set (VLC_PLUGIN_REQUIRED_MIN_VERSION "3.0.0"
     CACHE STRING "VLC Plugin required version")

# This value can be changed with CMake GUI mode.
set (LIB_BITTORRENT_REQUIRED_MIN_VERSION "1.0.0"
     CACHE STRING "LibTorrent required version")

# Try to find pkg-congig
find_package(PkgConfig REQUIRED)

# Check with PkgConfig if vlc-plugin
# are available and throw an error if not, or less version is installed.
pkg_check_modules (
    VlcPlugin
    REQUIRED IMPORTED_TARGET
    "vlc-plugin>=${VLC_PLUGIN_REQUIRED_MIN_VERSION}"
)

# Check with PkgConfig if libtorrent-rasterbar
# are available and throw an error if not, or less version is installed.
pkg_check_modules (
    LibTorrent
    REQUIRED IMPORTED_TARGET
    "libtorrent-rasterbar>=${LIB_BITTORRENT_REQUIRED_MIN_VERSION}"
)

configure_file (".cmake/config.h.in" ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)
