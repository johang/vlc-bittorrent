/*
Copyright 2018 Johan Gunnarsson <johan.gunnarsson@gmail.com>

This file is part of vlc-bittorrent.

vlc-bittorrent is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

vlc-bittorrent is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with vlc-bittorrent.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VLC_BITTORRENT_LIBTORRENT_H
#define VLC_BITTORRENT_LIBTORRENT_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include <torrent_handle.hpp>
#include <torrent_info.hpp>
#include <session.hpp>
#include <magnet_uri.hpp>
#include <alert.hpp>
#include <alert_types.hpp>
#include <peer_request.hpp>
#include <create_torrent.hpp>
#include <version.hpp>
#if LIBTORRENT_VERSION_NUM >= 10100
#include <hex.hpp>
#endif
#include <sha1_hash.hpp>
#if LIBTORRENT_VERSION_NUM < 10100
#include <escape_string.hpp>
#endif
#pragma GCC diagnostic pop

class Download;

void
libtorrent_add_download(Download *dl, libtorrent::add_torrent_params& atp);

void
libtorrent_remove_download(Download *dl);

#endif
