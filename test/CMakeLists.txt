cmake_minimum_required (VERSION 3.14)

find_package (GTest)

if (NOT GTest_FOUND)
  message (FATAL_ERROR "Please run `sudo apt install libgtest-dev` on Ubuntu.")
endif (NOT GTest_FOUND)

find_package (PkgConfig REQUIRED)
find_package (Threads REQUIRED)

pkg_check_modules (LibVlcPlugin REQUIRED IMPORTED_TARGET "vlc-plugin>=3.0.0")
pkg_check_modules (LibVlc REQUIRED IMPORTED_TARGET "libvlc>=3.0.0")
pkg_check_modules (LibtorrentRasterbar REQUIRED IMPORTED_TARGET "libtorrent-rasterbar >= 1.2.0")

include (GoogleTest)

function (create_dummy_download_tests TEST_FILES)
  add_executable (dummydownload)
  target_sources (dummydownload PRIVATE ${TEST_FILES})
  target_include_directories (dummydownload PRIVATE "../src")
  target_link_libraries (dummydownload
    PRIVATE
      GTest::gtest
      GTest::gtest_main
      Threads::Threads
      PkgConfig::LibVlc
      PkgConfig::LibVlcPlugin
      PkgConfig::LibtorrentRasterbar)

  get_target_property (PLUGIN_OUTPUT_DIRECTORY access_bittorrent_plugin RUNTIME_OUTPUT_DIRECTORY)

  gtest_discover_tests (dummydownload
    LIST_TESTS DUMMY_DOWNLOAD_TESTS
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data/"
    PROPERTIES
      ENVIRONMENT VLC_PLUGIN_PATH=${PLUGIN_OUTPUT_DIRECTORY}
      TIMEOUT 60)
endfunction (create_dummy_download_tests TEST_FILES)

create_dummy_download_tests ("download_dummy.cpp;../src/download.cpp;../src/session.cpp")
# create_vlc_test (miniclient miniclient.cpp)
# create_vlc_test (vlcdummy vlcdummy.c)
