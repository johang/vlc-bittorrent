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

#ifndef VLC_BITTORRENT_DOWNLOAD_H
#define VLC_BITTORRENT_DOWNLOAD_H

#include <atomic>
#include <forward_list>
#include <memory>
#include <mutex>
#include <thread>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include <libtorrent/alert.hpp>
#include <libtorrent/peer_request.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#pragma GCC diagnostic pop

#include "session.h"

namespace lt = libtorrent;

using MetadataProgressCb = std::function<void(float)>;
using DataProgressCb = std::function<void(float)>;

class Download {

public:
    Download(const Download&) = delete;
    Download&
    operator=(const Download&)
        = delete;
    Download(std::mutex& mtx, lt::add_torrent_params& atp, bool k);
    ~Download();

    static std::shared_ptr<Download>
    get_download(
        char* metadata, size_t metadatalen, std::string save_path, bool keep);

    /**
     * Get a part of the data of this download. If the data is not
     * available, it will download it and wait for it to become available.
     */
    ssize_t
    read(int file, int64_t off, char* buf, size_t buflen,
        DataProgressCb progress_cb);

    ssize_t
    read(int file, int64_t off, char* buf, size_t buflen)
    {
        return read(file, off, buf, buflen, nullptr);
    }

    static std::vector<std::pair<std::string, uint64_t>>
    get_files(char* metadata, size_t metadatalen);

    std::vector<std::pair<std::string, uint64_t>>
    get_files();

    static std::shared_ptr<std::vector<char>>
    get_metadata(std::string url, std::string save_path, std::string cache_path,
        MetadataProgressCb progress_cb);

    static std::shared_ptr<std::vector<char>>
    get_metadata(std::string url, std::string save_path, std::string cache_path)
    {
        return get_metadata(url, save_path, cache_path, nullptr);
    }

    std::shared_ptr<std::vector<char>>
    get_metadata(MetadataProgressCb progress_cb);

    std::shared_ptr<std::vector<char>>
    get_metadata()
    {
        return get_metadata(nullptr);
    }

    std::pair<int, uint64_t>
    get_file(std::string path);

    std::string
    get_name();

    std::string
    get_infohash();

private:
    static std::shared_ptr<Download>
    get_download(lt::add_torrent_params& atp, bool k);

    void
    download_metadata(MetadataProgressCb cb);

    void
    download_metadata()
    {
        download_metadata(nullptr);
    }

    void
    download(lt::peer_request part, DataProgressCb cb);

    void
    download(lt::peer_request part)
    {
        download(part, nullptr);
    }

    ssize_t
    read(lt::peer_request part, char* buf, size_t buflen);

    void
    set_piece_priority(int file, int64_t off, int size, int prio);

    // Locks mutex passed to constructor
    std::unique_lock<std::mutex> m_lock;

    bool m_keep;

    std::shared_ptr<Session> m_session;

    lt::torrent_handle m_th;
};

#endif
