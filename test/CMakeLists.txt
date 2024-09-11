cmake_minimum_required (VERSION 3.8.2)

find_package (Threads REQUIRED)

if (CMAKE_TOOLCHAIN_FILE)
  find_package (LibtorrentRasterbar CONFIG REQUIRED)
  add_library (Libtorrent ALIAS LibtorrentRasterbar::torrent-rasterbar)
else ()
  message (STATUS "Toolchain option not found in environment. Using PkgConfig by default.")
  find_package (PkgConfig REQUIRED)

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

  add_library (Vendor::Vlc ALIAS PkgConfig::Vlc)
  add_library (Vendor::VlcPlugin ALIAS PkgConfig::VlcPlugin)
  add_library (Vendor::Libtorrent ALIAS PkgConfig::LibtorrentRasterbar)
endif ()

add_executable (
  vlcdummy
    "vlcdummy.c"
)

target_link_libraries (
  vlcdummy
    PUBLIC
      Vendor::Vlc
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
    Vendor::Libtorrent
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
      Vendor::VlcPlugin
      Vendor::Libtorrent
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
