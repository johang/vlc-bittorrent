cmake_minimum_required(VERSION 3.14)

project(vlc-bittorrent)

add_library(
  access_bittorrent_plugin
    MODULE
      module.cpp
      metadata.cpp
      magnetmetadata.cpp
      data.cpp
      download.cpp
      session.cpp
      vlc.cpp
)

target_include_directories(
  access_bittorrent_plugin
    PRIVATE
      ${CMAKE_BINARY_DIR}
)

target_compile_definitions(
  access_bittorrent_plugin
    PRIVATE
      -DMODULE_STRING=\"bittorrent\"
      -D__PLUGIN__
      -DPACKAGE=\"vlc-bittorrent\"
)

target_compile_features(
  access_bittorrent_plugin
    PUBLIC
      cxx_std_14
)

target_link_libraries(
  access_bittorrent_plugin
    PUBLIC
      PkgConfig::VlcPlugin
    PRIVATE
      PkgConfig::LibtorrentRasterbar
      Threads::Threads
)

set_target_properties(
  access_bittorrent_plugin
    PROPERTIES
      CXX_STANDARD 14
      CXX_STANDARD_REQUIRED YES
      CXX_VISIBILITY_PRESET hidden
)
