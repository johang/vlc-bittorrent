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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vlc_plugin.h>

#include "data.h"
#include "magnetmetadata.h"
#include "metadata.h"

// clang-format off

vlc_module_begin()
    set_shortname("bittorrent")
    set_category(CAT_INPUT)
    set_subcategory(SUBCAT_INPUT_STREAM_FILTER)
    set_description("Bittorrent metadata access")
    set_capability("stream_directory", 99)
    set_callbacks(MetadataOpen, NULL)

#if 1
    add_directory(DLDIR_CONFIG, NULL, "Downloads",
        "Directory where VLC will put downloaded files.", false)
    add_bool(KEEP_CONFIG, false, "Don't delete files",
        "Don't delete files after download.", true)
#else
    add_directory(DLDIR_CONFIG, NULL, "Downloads",
        "Directory where VLC will put downloaded files.")
    add_bool(KEEP_CONFIG, false, "Don't delete files",
        "Don't delete files after download.")
#endif

    add_submodule()
        set_description("Bittorrent data access")
        set_capability("stream_extractor", 99)
        set_callbacks(DataOpen, DataClose)

    add_submodule()
        set_description("Bittorrent magnet metadata access")
        set_capability("access", 60)
        add_shortcut("file", "magnet")
        set_callbacks(MagnetMetadataOpen, MagnetMetadataClose)
vlc_module_end()
