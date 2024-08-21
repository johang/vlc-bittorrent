cmake_minimum_required(VERSION 3.14)

project(
  "vlc-plugin-bittorrent"
  VERSION 2.15
  DESCRIPTION "A bittorrent plugin for VLC."
  HOMEPAGE_URL "https://github.com/johang/vlc-bittorrent"
  LANGUAGES C CXX
)

add_subdirectory(src)
