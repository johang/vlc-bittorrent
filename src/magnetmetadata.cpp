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

#include "download.h"
#include "magnetmetadata.h"
#include "vlc.h"

#define D(x)

struct access_sys_t {
    std::shared_ptr<std::vector<char>> p_metadata;

    // Current position within the metadata
    size_t i_pos;
};

static ssize_t
MagnetMetadataRead(stream_t* p_access, void* p_buffer, size_t i_len)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    if (!p_access->p_sys)
        return -1;

    access_sys_t* p_sys = (access_sys_t*) p_access->p_sys;

    if (!p_sys->p_metadata)
        return -1;

    ssize_t len
        = (ssize_t) std::min(i_len, p_sys->p_metadata->size() - p_sys->i_pos);
    if (len < 0)
        return -1;

    std::copy(p_sys->p_metadata->begin() + (ssize_t) p_sys->i_pos,
        p_sys->p_metadata->begin() + (ssize_t) p_sys->i_pos + len,
        (char*) p_buffer);

    p_sys->i_pos += (size_t) len;

    return len;
}

static int
MagnetMetadataControl(stream_t* access, int query, va_list args)
{
    D(printf("%s:%d: %s(0x%x)\n", __FILE__, __LINE__, __func__, query));

    switch (query) {
    case STREAM_GET_PTS_DELAY:
        *va_arg(args, int64_t*) = DEFAULT_PTS_DELAY;
        break;
    case STREAM_CAN_SEEK:
        *va_arg(args, bool*) = false;
        break;
    case STREAM_CAN_PAUSE:
        *va_arg(args, bool*) = false;
        break;
    case STREAM_CAN_CONTROL_PACE:
        *va_arg(args, bool*) = true;
        break;
    case STREAM_GET_CONTENT_TYPE:
        *va_arg(args, char**) = strdup("application/x-bittorrent");
        break;
    default:
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

int
MagnetMetadataOpen(vlc_object_t* p_this)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    stream_t* p_access = (stream_t*) p_this;

    std::string name(p_access->psz_name ?: "");
    std::string filepath(p_access->psz_filepath ?: "");
    std::string location(p_access->psz_location ?: "");

    std::string magnet;
    if (name == "magnet") {
        magnet = "magnet:" + location;
    } else if (name == "file") {
        size_t index = filepath.rfind("magnet:?");
        if (index != std::string::npos) {
            magnet = filepath.substr(index);
        } else {
            return VLC_EGENERIC;
        }
    } else {
        return VLC_EGENERIC;
    }

    auto p_sys = std::make_unique<access_sys_t>();

    try {
        p_sys->p_metadata = Download::get_metadata(magnet,
            get_download_directory(p_this), get_cache_directory(p_this));

        msg_Dbg(p_access, "Got %zu bytes metadata", p_sys->p_metadata->size());
    } catch (std::runtime_error& e) {
        msg_Err(p_access, "Failed to get metadata: %s", e.what());
        return VLC_EGENERIC;
    }

    p_access->p_sys = p_sys.release();
    p_access->pf_read = MagnetMetadataRead;
    p_access->pf_control = MagnetMetadataControl;

    return VLC_SUCCESS;
}

void
MagnetMetadataClose(vlc_object_t* p_this)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    stream_t* p_access = (stream_t*) p_this;

    if (!p_access->p_sys)
        return;

    access_sys_t* p_sys = (access_sys_t*) p_access->p_sys;

    delete p_sys;
}
