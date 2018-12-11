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

#ifndef VLC_BITTORRENT_METADATA_H
#define VLC_BITTORRENT_METADATA_H

#include "libtorrent.h"
#include "vlc.h"

#ifndef EXTENSIONS_IMAGE_CSV
#define EXTENSIONS_IMAGE_CSV "png", "jpg", "jpeg", "gif"
#endif

#ifndef EXTENSIONS_AUDIO_CSV
#define EXTENSIONS_AUDIO_CSV "aac", "flac", "mp3", "oga", "opus", "wav", "wma"
#endif

#ifndef EXTENSIONS_VIDEO_CSV
#define EXTENSIONS_VIDEO_CSV "avi", "mkv", "mov", "mp4", "ogv", "webm", "wmv"
#endif

int
MetadataOpen(vlc_object_t *);

void
MetadataClose(vlc_object_t *);

#endif
