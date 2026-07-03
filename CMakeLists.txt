cmake_minimum_required(VERSION 3.14)

project(
  "vlc-plugin-bittorrent"
  VERSION 2.16
  DESCRIPTION "A bittorrent plugin for VLC."
  HOMEPAGE_URL "https://github.com/johang/vlc-bittorrent"
  LANGUAGES C CXX
)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_search_module(
  VlcPlugin
    REQUIRED IMPORTED_TARGET
      "vlc-plugin >= 3.0.0"
)

pkg_search_module(
  LibtorrentRasterbar
    REQUIRED IMPORTED_TARGET
      "libtorrent-rasterbar >= 2.0.0"
)

# For tests only
pkg_search_module(
  LibVlc
    IMPORTED_TARGET
      "libvlc >= 3.0.0"
)

add_subdirectory(src)

include(CTest)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
