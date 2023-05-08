if (UNIX AND NOT APPLE)
    install (
        FILES "vlc-plugin-bittorrent.desktop"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/applications"
    )
endif (UNIX AND NOT APPLE)
