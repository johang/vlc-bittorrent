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

#include <memory>
#include <vector>

#include "download.h"
#include "metadata.h"
#include "vlc.h"

#define D(x)

static int
MetadataReadDir(stream_directory_t* p_directory, input_item_node_t* p_node)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    // Temporary buffer to hold metadata
    auto md = std::make_unique<char[]>(0x100000);

    ssize_t mdsz = vlc_stream_Read(p_directory->source, md.get(), 0x100000);
    if (mdsz < 0)
        return VLC_EGENERIC;

    std::vector<std::pair<std::string, uint64_t>> files;
    try {
        files = Download::get_files(md.get(), (size_t) mdsz);
    } catch (std::runtime_error& e) {
        msg_Err(p_directory, "Failed to parse metadata: %s", e.what());
        return VLC_EGENERIC;
    }

    struct vlc_readdir_helper rdh;
    vlc_readdir_helper_init(&rdh, p_directory, p_node);

    for (auto f : files) {
        std::unique_ptr<char> mrl(
            vlc_stream_extractor_CreateMRL(p_directory, f.first.c_str()));
        if (!mrl)
            continue;

        int ret = vlc_readdir_helper_additem(
            &rdh, mrl.get(), f.first.c_str(), NULL, ITEM_TYPE_FILE, ITEM_LOCAL);
        if (ret != VLC_SUCCESS)
            msg_Warn(p_directory, "Failed to add %s", mrl.get());
    }

    vlc_readdir_helper_finish(&rdh, true);

    return VLC_SUCCESS;
}

int
MetadataOpen(vlc_object_t* p_obj)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    stream_directory_t* p_directory = (stream_directory_t*) p_obj;

    if (!stream_IsMimeType(p_directory->source, "application/x-bittorrent")
        && !stream_HasExtension(p_directory->source, ".torrent"))
        return VLC_EGENERIC;

    // Attempt to read 1 byte of the metadata
    const uint8_t* data = NULL;
    ssize_t len = vlc_stream_Peek(p_directory->source, &data, 1);

    // All bittorrent metadata files starts with a 'd'
    if (len < 1 || data[0] != 'd')
        return VLC_EGENERIC;

    p_directory->pf_readdir = MetadataReadDir;

    return VLC_SUCCESS;
}
