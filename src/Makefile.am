lib_LTLIBRARIES = libaccess_bittorrent_plugin.la

# Great and cool CFLAGS
COOLCFLAGS = \
	-Wall \
	-Wextra \
	-Wconversion \
	-Wsign-compare \
	-Wsign-conversion \
	-Wno-unused-parameter \
	-Wno-unused \
	-Wodr

libaccess_bittorrent_plugin_la_SOURCES = \
	module.cpp \
	metadata.cpp \
	magnetmetadata.cpp \
	data.cpp \
	download.cpp \
	session.cpp \
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
