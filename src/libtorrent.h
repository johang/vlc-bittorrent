#ifndef VLC_BITTORRENT_LIBTORRENT_H
#define VLC_BITTORRENT_LIBTORRENT_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/alert.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/peer_request.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/version.hpp>
#if LIBTORRENT_VERSION_NUM >= 10100
#include <libtorrent/hex.hpp>
#endif
#include <libtorrent/sha1_hash.hpp>
#if LIBTORRENT_VERSION_NUM < 10100
#include <libtorrent/escape_string.hpp>
#endif
#pragma GCC diagnostic pop

#endif
