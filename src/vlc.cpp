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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cerrno>

#include <memory>
#include <stdexcept>
#include <string>

#include "vlc.h"

std::string
get_download_directory(vlc_object_t* p_this)
{
    std::string dldir;

    std::unique_ptr<char> dir(var_InheritString(p_this, DLDIR_CONFIG));
    if (!dir) {
        std::unique_ptr<char> dir(config_GetUserDir(VLC_DOWNLOAD_DIR));
        if (!dir)
            throw std::runtime_error("Failed to find download directory");

        dldir = std::string(dir.get());

        if (vlc_mkdir(dldir.c_str(), 0777)) {
            if (errno != EEXIST)
                throw std::runtime_error("Failed to create directory (" + dldir
                    + "): " + strerror(errno));
        }

        dldir += DIR_SEP;
        dldir += PACKAGE;
    } else {
        dldir = std::string(dir.get());
    }

    if (vlc_mkdir(dldir.c_str(), 0777)) {
        if (errno != EEXIST)
            throw std::runtime_error("Failed to create directory (" + dldir
                + "): " + strerror(errno));
    }

    return dldir;
}

std::string
get_cache_directory(vlc_object_t* p_this)
{
    std::string cachedir;

    std::unique_ptr<char> dir(config_GetUserDir(VLC_CACHE_DIR));
    if (!dir)
        throw std::runtime_error("Failed to find cache directory");

    cachedir = std::string(dir.get());

    if (vlc_mkdir(cachedir.c_str(), 0777)) {
        if (errno != EEXIST)
            throw std::runtime_error("Failed to create directory (" + cachedir
                + "): " + strerror(errno));
    }

    return cachedir;
}

bool
get_keep_files(vlc_object_t* p_this)
{
    return var_InheritBool(p_this, KEEP_CONFIG);
}
