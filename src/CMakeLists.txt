# check cxx_std_14
cmake_minimum_required (VERSION 3.8.2)

include (CheckIncludeFile)

check_include_file ("dlfcn.h" HAVE_DLFCN_H)
check_include_file ("inttypes.h" HAVE_INTTYPES_H)
check_include_file ("stdint.h" HAVE_STDINT_H)
check_include_file ("stdio.h" HAVE_STDIO_H)
check_include_file ("stdlib.h" HAVE_STDLIB_H)
check_include_file ("strings.h" HAVE_STRINGS_H)
check_include_file ("string.h" HAVE_STRING_H)
check_include_file ("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file ("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file ("unistd.h" HAVE_UNISTD_H)

if (WIN32)
  file (DOWNLOAD "https://get.videolan.org/vlc/last/win64/vlc-3.0.21-win64.7z" "vendor/vlc.7z")
  file (ARCHIVE_EXTRACT INPUT "vendor/vlc.7z" "vendor/vlc")
endif (WIN32)

# config file options
set (PACKAGE "vlc-bittorrent")
set (PLIGIN_PACKAGE "${PACKAGE}")
set (PLIGIN_PACKAGE_BUGREPORT "johan.gunnarsson@gmail.com")
set (PLIGIN_PACKAGE_NAME "${PROJECT_NAME}")
set (PLIGIN_PACKAGE_STRING "${PACKAGE_NAME} ${PROJECT_VERSION}")
set (PLIGIN_PACKAGE_TARNAME "${PACKAGE}")
set (PLIGIN_PACKAGE_VERSION "${PROJECT_VERSION}")
set (PLIGIN_VERSION "${PROJECT_VERSION}")
set (PLIGIN_PACKAGE_URL "${PROJECT_HOMEPAGE_URL}")

configure_file (
  "${CMAKE_SOURCE_DIR}/.cmake/config.h.in"
  "${CMAKE_BINARY_DIR}/config.h"
)

if (CMAKE_TOOLCHAIN_FILE)
  find_package (LibtorrentRasterbar CONFIG REQUIRED)
  add_library (Libtorrent ALIAS LibtorrentRasterbar::torrent-rasterbar)
else ()
  message (STATUS "Toolchain option not found in environment. Using PkgConfig by default.")
  find_package (PkgConfig REQUIRED)
  find_package (Threads REQUIRED)

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

  add_library (Vendor::Libtorrent ALIAS PkgConfig::LibtorrentRasterbar)
endif ()

# add shared library
add_library (
  access_bittorrent_plugin
    MODULE
      module.cpp
      metadata.cpp
      magnetmetadata.cpp
      data.cpp
      download.cpp
      session.cpp
      vlc.cpp)

# compile definitions
target_compile_definitions (
  access_bittorrent_plugin
    PUBLIC
      -DMODULE_STRING=\"bittorrent\"
      -D__PLUGIN__
      -DHAVE_CONFIG_H
)

# link with libraries
target_link_libraries (
  access_bittorrent_plugin
    PUBLIC
      PkgConfig::VlcPlugin
      Vendor::Libtorrent
)

# add binary dir as include dir with config.h file.
target_include_directories (
  access_bittorrent_plugin
    PRIVATE
      ${CMAKE_BINARY_DIR}
)

# set properties like cxx standard, library output, archive output
set_target_properties (
  access_bittorrent_plugin
    PROPERTIES
      CXX_STANDARD 14
      CXX_STANDARD_REQUIRED YES
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# set cxx 14 as required
target_compile_features (
  access_bittorrent_plugin
    PUBLIC
      cxx_std_14
)
