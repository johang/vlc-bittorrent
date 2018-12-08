lib_LTLIBRARIES = libaccess_bittorrent_plugin.la

# Great and cool CFLAGS
COOLCFLAGS = \
	-std=c++11 \
	-Wno-deprecated-declarations \
	-Wall \
	-Wextra \
	-Wconversion \
	-Wsign-compare \
	-Wsign-conversion \
	-Wno-unused-parameter \
	-Wno-unused

libaccess_bittorrent_plugin_la_SOURCES = \
	module.cpp \
	metadata.cpp \
	magnetmetadata.cpp \
	data.cpp \
	download.cpp \
	libtorrent.cpp \
	vlc.cpp
libaccess_bittorrent_plugin_la_CXXFLAGS = \
	$(COOLCFLAGS) \
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
