add_library (
    "access_bittorrent_plugin"
    SHARED
    "module.cpp"
    "metadata.cpp"
    "magnetmetadata.cpp"
    "data.cpp"
    "download.cpp"
    "session.cpp"
    "vlc.cpp"
)

set_target_properties (
    "access_bittorrent_plugin"
    PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_definitions (
    "access_bittorrent_plugin"
    PUBLIC
    -D__PLUGIN__
    -DHAVE_CONFIG_H
    -DDLL_EXPORT
    -DMODULE_STRING="bittorrent"
)

target_include_directories (
    "access_bittorrent_plugin"
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${VlcPlugin_INCLUDE_DIRS}
    ${LibTorrent_INCLUDE_DIRS}
)

target_link_libraries (
    "access_bittorrent_plugin"
    ${VlcPlugin_LINK_LIBRARIES}
    ${LibTorrent_LINK_LIBRARIES}
    Threads::Threads
)

target_compile_options(
    "access_bittorrent_plugin"
    PRIVATE
    -Wall
    -Wextra
    -Wconversion
    -Wsign-compare
    -Wsign-conversion
    -Wno-unused-parameter
    -Wno-deprecated
	-Wno-unused
	-Wodr
)
