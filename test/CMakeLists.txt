find_program(
  BASH_EXECUTABLE
    NAMES bash bash.exe
)

if(NOT BASH_EXECUTABLE)
  message(FATAL_ERROR "bash not found; required to run *.test scripts")
endif()

#
# downloaddummy test app
#

add_executable(
  downloaddummy
    downloaddummy.cpp
    ${CMAKE_SOURCE_DIR}/src/download.cpp
    ${CMAKE_SOURCE_DIR}/src/session.cpp
)

target_include_directories(
  downloaddummy
    PRIVATE
      ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(
  downloaddummy
    PUBLIC
      cxx_std_14
)

target_link_libraries(
  downloaddummy
    PRIVATE
      PkgConfig::LibtorrentRasterbar
      PkgConfig::VlcPlugin
      Threads::Threads
)

# downloaddummy TAP test script
add_test(
  NAME downloaddummy.test
  COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/downloaddummy.test
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_tests_properties(
  downloaddummy.test
    PROPERTIES
      FAIL_REGULAR_EXPRESSION "not ok"
      ENVIRONMENT "DOWNLOADDUMMY_BIN=$<TARGET_FILE:downloaddummy>"
)

#
# vlcdummy test app
#

if(LibVlc_FOUND)

add_executable(vlcdummy vlcdummy.c)

target_link_libraries(
  vlcdummy
    PRIVATE
      PkgConfig::LibVlc
)

# vlcdummy TAP test script
add_test(
  NAME vlcdummy.test
  COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/vlcdummy.test
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_tests_properties(
  vlcdummy.test
    PROPERTIES
      FAIL_REGULAR_EXPRESSION "not ok"
      ENVIRONMENT "VLCDUMMY_BIN=$<TARGET_FILE:vlcdummy>;VLC_PLUGIN_PATH=$<TARGET_FILE_DIR:access_bittorrent_plugin>"
)

endif()

#
# miniclient test app
#

add_executable(miniclient miniclient.cpp)

target_compile_features(
  miniclient
    PUBLIC
      cxx_std_14
)

target_link_libraries(
  miniclient
    PRIVATE
      PkgConfig::LibtorrentRasterbar
      Threads::Threads
)
