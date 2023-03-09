# cmake version of vlc-bittorrent plugin by ori-d (https://github.com/ori-d)
# added support for building on windows system.
cmake_minimum_required (VERSION 3.25)

project ("vlc-bittorrent"
         LANGUAGES      C CXX
         VERSION        2.14
         HOMEPAGE_URL   "https://github.com/johang/vlc-bittorrent"
         DESCRIPTION    "VLC BitTorrent plugin")

list (APPEND CMAKE_MODULE_PATH
      "${CMAKE_CURRENT_LIST_DIR}/.cmake")

include (cm_cxx14_check)
include (config_checks)

# This value can be changed with CMake GUI mode.
set (VLC_PLUGIN_REQUIRED_MIN_VERSION "3.0.0"
     CACHE STRING "VLC Plugin required version")

# This value can be changed with CMake GUI mode.
set (LIB_BITTORRENT_REQUIRED_MIN_VERSION "1.0.0"
     CACHE STRING "LibTorrent required version")

# Try to find pkg-congig
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Check with PkgConfig if vlc-plugin
# are available and throw an error if not, or less version is installed.
pkg_check_modules (
    VlcPlugin
    REQUIRED
    "vlc-plugin>=${VLC_PLUGIN_REQUIRED_MIN_VERSION}"
)

# Check with PkgConfig if libtorrent-rasterbar
# are available and throw an error if not, or less version is installed.
pkg_check_modules (
    LibTorrent
    REQUIRED
    "libtorrent-rasterbar>=${LIB_BITTORRENT_REQUIRED_MIN_VERSION}"
)

configure_file (".cmake/config.h.in" "src/config.h" @ONLY)

add_subdirectory ("data")
add_subdirectory ("src")
