lib_LTLIBRARIES = libaccess_bittorrent_plugin.la

libaccess_bittorrent_plugin_la_SOURCES = \
	module.cpp \
	metadata.cpp \
	magnetmetadata.cpp \
	data.cpp \
	download.cpp \
	playlist.cpp
libaccess_bittorrent_plugin_la_CXXFLAGS = \
	-std=c++11 \
	-Wno-deprecated-declarations \
	-Wall \
	-Wextra \
	-Wconversion \
	-Wsign-compare \
	-Wsign-conversion \
	-Wno-unused-parameter \
	-Wno-unused \
	$(VLC_PLUGIN_CFLAGS) \
	$(LIBTORRENT_CFLAGS) \
	-DMODULE_STRING=\"bittorrent\"
libaccess_bittorrent_plugin_la_LIBADD = \
	$(VLC_PLUGIN_LIBS) \
	$(LIBTORRENT_LIBS)
libaccess_bittorrent_plugin_la_LDFLAGS = \
	-avoid-version \
	-module \
	-export-symbol-regex ^vlc_entry
