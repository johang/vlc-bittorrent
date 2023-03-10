cmake_minimum_required (VERSION 3.2)

project ("vlc-bittorrent"
         LANGUAGES      C CXX
         VERSION        2.14
         HOMEPAGE_URL   "https://github.com/johang/vlc-bittorrent"
         DESCRIPTION    "VLC BitTorrent plugin")

list (APPEND CMAKE_MODULE_PATH
      "${CMAKE_CURRENT_LIST_DIR}/.cmake")

include (cm_cxx14_check)
include (config_checks)

option (VLC_BITTORRENT_BUILD_TESTS "" ON)

# This value can be changed with CMake GUI mode.
set (VLC_BITTORRENT_MIN_VERSION_VLC_PLUGIN "3.0.0"
     CACHE STRING "VLC Plugin required version")

# This value can be changed with CMake GUI mode.
set (VLC_BITTORRENT_MIN_VERSION_TORRENT "1.0.0"
     CACHE STRING "LibTorrent required version")

# Try to find pkg-congig
find_package (PkgConfig REQUIRED)
find_package (Threads REQUIRED)

# Check with PkgConfig if vlc-plugin
# are available and throw an error if not, or less version is installed.
pkg_check_modules (
    VlcPlugin
    REQUIRED
    "vlc-plugin>=${VLC_BITTORRENT_MIN_VERSION_VLC_PLUGIN}"
)

# Check with PkgConfig if libtorrent-rasterbar
# are available and throw an error if not, or less version is installed.
pkg_check_modules (
    LibTorrent
    REQUIRED
    "libtorrent-rasterbar>=${VLC_BITTORRENT_MIN_VERSION_TORRENT}"
)

configure_file (".cmake/config.h.in" "src/config.h" @ONLY)

add_subdirectory ("data")
add_subdirectory ("src")

if (VLC_BITTORRENT_BUILD_TESTS)
    add_subdirectory ("test")
endif (VLC_BITTORRENT_BUILD_TESTS)
