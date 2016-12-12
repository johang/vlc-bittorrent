#ifndef VLC_BITTORRENT_VLC_H
#define VLC_BITTORRENT_VLC_H

// Workaround because VLC's vlc_atomic.h is borked for VLC 2.2.x.
#ifndef VLC_ATOMIC_H
#define VLC_ATOMIC_H
#include <atomic>
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_input.h>
#include <vlc_demux.h>
#include <vlc_access.h>
#include <vlc_stream.h>
#include <vlc_url.h>
#include <vlc_variables.h>
#pragma GCC diagnostic pop

#endif
