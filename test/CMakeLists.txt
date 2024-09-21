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

function (create_vlc_test TEST_NAME TEST_FILES)
  add_executable (${TEST_NAME})
  target_sources (${TEST_NAME} PRIVATE ${TEST_FILES})
  target_include_directories (${TEST_NAME} PRIVATE "../src")
  target_link_libraries (${TEST_NAME}
    PRIVATE
      GTest::gtest
      GTest::gtest_main
      Threads::Threads
      PkgConfig::LibVlc
      PkgConfig::LibVlcPlugin
      PkgConfig::LibtorrentRasterbar)
  gtest_discover_tests (${TEST_NAME} TEST_LIST VLC_TESTS_WITH_ARGS WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data/")
  get_target_property (PLUGIN_OUTPUT_DIRECTORY access_bittorrent_plugin RUNTIME_OUTPUT_DIRECTORY)
  
  set_tests_properties (${VLC_TESTS_WITH_ARGS}
    PROPERTIES
      ENVIRONMENT "VLC_PLUGIN_PATH=${PLUGIN_OUTPUT_DIRECTORY}"
      TIMEOUT 1
      WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data/")
endfunction (create_vlc_test TEST_NAME TEST_FILES)

create_vlc_test (dummydownload "download_dummy.cpp;../src/download.cpp;../src/session.cpp")
# create_vlc_test (miniclient miniclient.cpp)
# create_vlc_test (vlcdummy vlcdummy.c)
