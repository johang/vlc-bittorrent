cmake_minimum_required (VERSION 3.8.2)

find_package (PkgConfig REQUIRED)
find_package (Threads REQUIRED)

pkg_check_modules (
  Vlc
    REQUIRED IMPORTED_TARGET
      "libvlc >= 3.0.0"
)

pkg_check_modules (
  VlcPlugin
    REQUIRED IMPORTED_TARGET
      "vlc-plugin >= 3.0.0"
)

pkg_check_modules (
  LibtorrentRasterbar
    REQUIRED
    IMPORTED_TARGET
      "libtorrent-rasterbar >= 1.2.0"
)

add_executable (
  vlcdummy
    "vlcdummy.c"
)

target_link_libraries (
  vlcdummy
    PUBLIC
      PkgConfig::Vlc
)

set_target_properties (
  vlcdummy
    PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

add_executable (
  miniclient
    "miniclient.cpp"
)

target_link_libraries (
  miniclient
    PkgConfig::LibtorrentRasterbar
    Threads::Threads
)

add_executable (
  downloaddummy
    "downloaddummy.cpp"
    "../src/download.cpp"
    "../src/session.cpp"
)

set_target_properties (
  downloaddummy
    PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

target_include_directories (
  downloaddummy
    PRIVATE
      "../src/"
)

target_link_libraries (
  downloaddummy
    PRIVATE
      PkgConfig::VlcPlugin
      PkgConfig::LibtorrentRasterbar
)

find_program (BASH_PROGRAM sh)

add_test (
  vlcdummy
  ${BASH_PROGRAM}
    "${CMAKE_CURRENT_LIST_DIR}/vlcdummy.test"
)

add_test (
  downloaddummy
  ${BASH_PROGRAM}
    "${CMAKE_CURRENT_LIST_DIR}/downloaddummy.test"
)
