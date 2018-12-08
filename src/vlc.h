/*
Copyright 2016 Johan Gunnarsson <johan.gunnarsson@gmail.com>

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
#include <vlc_fs.h>
#include <vlc_es.h>
#include <vlc_stream.h>
#include <vlc_interrupt.h>
#include <vlc_threads.h>
#pragma GCC diagnostic pop

std::string
get_download_directory(vlc_object_t *p_this);

std::string
get_cache_directory(vlc_object_t *p_this);

#endif
