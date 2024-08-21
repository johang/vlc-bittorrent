# check cxx_std_14
cmake_minimum_required (VERSION 3.8.2)

project (
  "vlc-plugin-bittorrent"
  VERSION 2.15
  DESCRIPTION "A bittorrent plugin for VLC."
  HOMEPAGE_URL "https://github.com/johang/vlc-bittorrent"
  LANGUAGES C CXX
)

add_subdirectory (src)

# Can't use tests now... need more time
option (ENABLE_TESTS "Testing" OFF)

if (ENABLE_TESTS)
  enable_testing()
  add_subdirectory (test)
endif (ENABLE_TESTS)

