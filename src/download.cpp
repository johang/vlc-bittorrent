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

/*
XXX: This file is basically just glue code so vlc can make interruptible
     blocking calls to libtorrent.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>

#include "download.h"
#include "session.h"
#include "vlc.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include <libtorrent/alert.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/hex.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/peer_request.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/sha1_hash.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/version.hpp>
#pragma GCC diagnostic pop

#define D(x)
#define DD(x)

#define MB (1024 * 1024)

namespace lt = libtorrent;

template <typename T> class vlc_interrupt_guard {
public:
    vlc_interrupt_guard(T& pr)
    {
        vlc_interrupt_register(abort, &pr);
    }

    ~vlc_interrupt_guard()
    {
        vlc_interrupt_unregister();
    }

private:
    static void
    abort(void* data)
    {
        try {
            // Interrupt get()/wait() on this promise
            static_cast<T*>(data)->set_exception(
                std::make_exception_ptr(std::runtime_error("vlc interrupted")));
        } catch (...) {
        }
    }
};

template <typename T> class AlertSubscriber {
public:
    AlertSubscriber(Session* dl, T* pr)
        : m_session(dl)
        , m_promise(pr)
    {
        m_session->register_alert_listener(m_promise);
    }

    ~AlertSubscriber()
    {
        m_session->unregister_alert_listener(m_promise);
    }

private:
    Session* m_session;

    T* m_promise;
};

using ReadValue = std::pair<boost::shared_array<char>, int>;

class ReadPiecePromise : public std::promise<ReadValue>, public Alert_Listener {
public:
    ReadPiecePromise(lt::sha1_hash ih, int p)
        : m_ih(ih)
        , m_piece(p)
    {
    }

    void
    handle_alert(lt::alert* a) override
    {
        if (lt::alert_cast<lt::torrent_alert>(a)) {
            auto* x = lt::alert_cast<lt::torrent_alert>(a);
            if (x->handle.info_hash() != m_ih)
                return;
        }

        if (lt::alert_cast<lt::read_piece_alert>(a)) {
            auto* x = lt::alert_cast<lt::read_piece_alert>(a);
            if (x->piece != m_piece)
                return;

            if (x->ec)
                set_exception(
                    std::make_exception_ptr(std::runtime_error("read failed")));
            else
                // Read is done
                set_value(std::make_pair(x->buffer, x->size));
        }
    }

private:
    lt::sha1_hash m_ih;

    int m_piece;
};

class DownloadPiecePromise : public std::promise<void>, public Alert_Listener {
public:
    DownloadPiecePromise(lt::sha1_hash ih, int p)
        : m_ih(ih)
        , m_piece(p)
    {
    }

    void
    handle_alert(lt::alert* a) override
    {
        if (lt::alert_cast<lt::torrent_alert>(a)) {
            auto* x = lt::alert_cast<lt::torrent_alert>(a);
            if (x->handle.info_hash() != m_ih)
                return;
        }

        if (lt::alert_cast<lt::piece_finished_alert>(a)) {
            auto* x = lt::alert_cast<lt::piece_finished_alert>(a);
            if (x->piece_index != m_piece)
                return;

            // Download is done
            set_value();
        }
    }

private:
    lt::sha1_hash m_ih;

    int m_piece;
};

class MetadataDownloadPromise : public std::promise<void>,
                                public Alert_Listener {
public:
    MetadataDownloadPromise(lt::sha1_hash ih)
        : m_ih(ih)
    {
    }

    void
    handle_alert(lt::alert* a) override
    {
        if (lt::alert_cast<lt::torrent_alert>(a)) {
            auto* x = lt::alert_cast<lt::torrent_alert>(a);
            if (x->handle.info_hash() != m_ih)
                return;
        }

        if (lt::alert_cast<lt::torrent_error_alert>(a)) {
            set_exception(
                std::make_exception_ptr(std::runtime_error("metadata failed")));
        } else if (lt::alert_cast<lt::metadata_failed_alert>(a)) {
            set_exception(
                std::make_exception_ptr(std::runtime_error("metadata failed")));
        } else if (lt::alert_cast<lt::metadata_received_alert>(a)) {
            // Metadata download is done
            set_value();
        }
    }

private:
    lt::sha1_hash m_ih;
};

class RemovePromise : public std::promise<void>, public Alert_Listener {
public:
    RemovePromise(lt::sha1_hash ih)
        : m_ih(ih)
    {
    }

    void
    handle_alert(lt::alert* a) override
    {
        if (lt::alert_cast<lt::torrent_removed_alert>(a)) {
            auto* x = lt::alert_cast<lt::torrent_removed_alert>(a);
            if (x->info_hash != m_ih)
                return;

            // Remove is done
            set_value();
        }
    }

private:
    lt::sha1_hash m_ih;
};

Download::Download(char* md, size_t mdsz, std::string save_path, bool keep)
    : m_keep(keep)
{
    D(printf("%s:%d: %s (from buf)\n", __FILE__, __LINE__, __func__));

    lt::add_torrent_params atp;
    atp.save_path = save_path;
    atp.flags &= ~lt::add_torrent_params::flag_auto_managed;
    atp.flags &= ~lt::add_torrent_params::flag_paused;
    atp.flags |= lt::add_torrent_params::flag_duplicate_is_error;

    lt::error_code ec;

#if LIBTORRENT_VERSION_NUM < 10200
    atp.ti = boost::make_shared<lt::torrent_info>(md, mdsz, boost::ref(ec));
#else
    atp.ti = std::make_shared<lt::torrent_info>(md, mdsz, std::ref(ec));
#endif
    if (ec)
        throw std::runtime_error("Failed to parse metadata");

    // Doesn't matter if it's duplicate since we never remove torrents
    m_th = m_session.get()->add_torrent(atp);
    if (!m_th.is_valid())
        throw std::runtime_error("Failed to add download by buffer");

    // Need to give libtorrent some time to breethe
    sleep(1);

    download_metadata();
}

Download::Download(std::string url, std::string save_path, bool keep)
    : m_keep(keep)
{
    D(printf("%s:%d: %s (from magnet)\n", __FILE__, __LINE__, __func__));

    lt::add_torrent_params atp;
    atp.save_path = save_path;
    atp.flags &= ~lt::add_torrent_params::flag_auto_managed;
    atp.flags &= ~lt::add_torrent_params::flag_paused;
    atp.flags |= lt::add_torrent_params::flag_duplicate_is_error;
    atp.url = url;

    // Doesn't matter if it's duplicate since we never remove torrents
    m_th = m_session.get()->add_torrent(atp);
    if (!m_th.is_valid())
        throw std::runtime_error("Failed to add download by URL");

    // Need to give libtorrent some time to breethe
    sleep(1);

    download_metadata();
}

Download::~Download()
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    if (m_th.is_valid()) {
        RemovePromise rmprom(m_th.info_hash());
        AlertSubscriber<RemovePromise> sub(&m_session, &rmprom);

        auto f = rmprom.get_future();

        if (m_keep)
            m_session.get()->remove_torrent(m_th);
        else
            m_session.get()->remove_torrent(m_th, lt::session::delete_files);

        // Wait for remove to complete
        f.wait_for(std::chrono::seconds(5));
    }
}

ssize_t
Download::read(int file, int64_t fileoff, char* buf, size_t buflen)
{
    D(printf("%s:%d: %s(%d, %lu, %p, %lu)\n", __FILE__, __LINE__, __func__,
        file, fileoff, buf, buflen));

    auto ti = m_th.torrent_file();

    auto fs = ti->files();

    if (file >= fs.num_files() || file < 0)
        throw std::runtime_error("File not found");

    if (fileoff < 0)
        throw std::runtime_error("File offset negative");

    auto filesz = fs.file_size(file);
    if (fileoff >= filesz)
        return 0;

    // Figure out what to read
    lt::peer_request part = m_th.torrent_file()->map_file(file, fileoff,
        std::max(
            0, std::min((int) buflen, (int) (fs.file_size(file) - fileoff))));
    if (part.length <= 0)
        return 0;

    int p = part.piece;
    int64_t o = fileoff - part.start;

    // Update piece priorities
    for (; o < std::min(filesz, fileoff + 128 * MB); o += ti->piece_size(p++)) {
        if (!m_th.have_piece(p)) {
            if (o < fileoff + 16 * MB && m_th.piece_priority(p) < 7)
                m_th.piece_priority(p, 7);
            else if (o < fileoff + 128 * MB && m_th.piece_priority(p) < 2)
                m_th.piece_priority(p, 2);
        }
    }

    if (!m_th.have_piece(part.piece))
        download(part);

    return read(part, buf, buflen);
}

std::vector<std::pair<std::string, uint64_t>>
Download::get_files()
{
    std::vector<std::pair<std::string, uint64_t>> files;

    const lt::file_storage& fs = m_th.torrent_file()->files();
    for (int i = 0; i < fs.num_files(); i++) {
        files.push_back(std::make_pair(fs.file_path(i), fs.file_size(i)));
    }

    return files;
}

std::vector<std::pair<std::string, uint64_t>>
Download::get_files(char* metadata, size_t metadatasz)
{
    lt::error_code ec;

    lt::torrent_info ti(metadata, (int) metadatasz, ec);
    if (ec)
        throw std::runtime_error("Failed to parse metadata");

    std::vector<std::pair<std::string, uint64_t>> files;

    const lt::file_storage& fs = ti.files();
    for (int i = 0; i < fs.num_files(); i++) {
        files.push_back(std::make_pair(fs.file_path(i), fs.file_size(i)));
    }

    return files;
}

std::shared_ptr<std::vector<char>>
Download::get_metadata(
    std::string url, std::string save_path, std::string cache_path)
{
    lt::add_torrent_params atp;
    atp.save_path = save_path;
    atp.flags &= ~lt::add_torrent_params::flag_auto_managed;
    atp.flags &= ~lt::add_torrent_params::flag_paused;

    lt::error_code ec;

    lt::parse_magnet_uri(url, atp, ec);
    if (ec) {
#if LIBTORRENT_VERSION_NUM < 10200
        atp.ti = boost::make_shared<lt::torrent_info>(url, boost::ref(ec));
#else
        atp.ti = std::make_shared<lt::torrent_info>(url, std::ref(ec));
#endif
        if (ec)
            throw std::runtime_error("Failed to parse metadata");
    } else {
        std::string path = cache_path + DIR_SEP
            + lt::to_hex(atp.info_hash.to_string()) + ".torrent";

        // Try to read up cache
#if LIBTORRENT_VERSION_NUM < 10200
        atp.ti = boost::make_shared<lt::torrent_info>(path, boost::ref(ec));
#else
        atp.ti = std::make_shared<lt::torrent_info>(path, std::ref(ec));
#endif
        if (ec) {
            // Download metadata
            Download dl(url, save_path, false);

            return dl.get_metadata_and_write_to_file(path);
        }
    }

    auto buffer = std::make_shared<std::vector<char>>();

    // Bencode metadata into buffer
    lt::bencode(
        std::back_inserter(*buffer), lt::create_torrent(*atp.ti).generate());

    return buffer;
}

std::pair<int, uint64_t>
Download::get_file(std::string path)
{
    const lt::file_storage& fs = m_th.torrent_file()->files();
    for (int i = 0; i < fs.num_files(); i++) {
        if (fs.file_path(i) == path)
            return std::make_pair(i, (uint64_t) fs.file_size(i));
    }

    throw std::runtime_error("Failed to find file");
}

std::string
Download::get_name()
{
    return m_th.torrent_file()->name();
}

std::string
Download::get_infohash()
{
    return lt::to_hex(m_th.torrent_file()->info_hash().to_string());
}

std::shared_ptr<std::vector<char>>
Download::get_metadata()
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    auto entry = lt::create_torrent(*m_th.torrent_file()).generate();

    auto buffer = std::make_shared<std::vector<char>>();

    // Bencode metadata into vector
    lt::bencode(std::back_inserter(*buffer), entry);

    return buffer;
}

std::shared_ptr<std::vector<char>>
Download::get_metadata_and_write_to_file(std::string path)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    auto entry = lt::create_torrent(*m_th.torrent_file()).generate();

    std::filebuf fb;
    fb.open(path, std::ios::out | std::ios::binary);
    std::ostream os(&fb);

    // Bencode metadata into file
    lt::bencode(std::ostream_iterator<char>(os), entry);

    auto buffer = std::make_shared<std::vector<char>>();

    // Bencode metadata into vector
    lt::bencode(std::back_inserter(*buffer), entry);

    return buffer;
}

void
Download::download_metadata()
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    if (m_th.status().has_metadata)
        return;

    MetadataDownloadPromise dlprom(m_th.info_hash());
    AlertSubscriber<MetadataDownloadPromise> sub(&m_session, &dlprom);
    vlc_interrupt_guard<MetadataDownloadPromise> intrguard(dlprom);

    auto f = dlprom.get_future();

    // Wait for metadata to download
    while (!m_th.status().has_metadata) {
        auto r = f.wait_for(std::chrono::seconds(1));
        if (r == std::future_status::ready)
            return f.get();
    }
}

void
Download::download(lt::peer_request part)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    if (m_th.have_piece(part.piece))
        return;

    DownloadPiecePromise dlprom(m_th.info_hash(), part.piece);
    AlertSubscriber<DownloadPiecePromise> sub(&m_session, &dlprom);
    vlc_interrupt_guard<DownloadPiecePromise> intrguard(dlprom);

    auto f = dlprom.get_future();

    // Wait for download
    while (!m_th.have_piece(part.piece)) {
        auto r = f.wait_for(std::chrono::seconds(1));
        if (r == std::future_status::ready)
            return f.get();
    }
}

ssize_t
Download::read(lt::peer_request part, char* buf, size_t buflen)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    ReadPiecePromise rdprom(m_th.info_hash(), part.piece);
    AlertSubscriber<ReadPiecePromise> sub(&m_session, &rdprom);
    vlc_interrupt_guard<ReadPiecePromise> intrguard(rdprom);

    auto f = rdprom.get_future();

    // Trigger read
    m_th.read_piece(part.piece);

    // Wait for read to complete
    boost::shared_array<char> piece_buffer;
    int piece_size;
    std::tie(piece_buffer, piece_size) = f.get();

    int len = std::min({ piece_size - part.start, (int) buflen, part.length });
    if (len < 0)
        return -1;

    // Copy from libtorrent buffer to VLC buffer
    memcpy(buf, piece_buffer.get() + part.start, (size_t) len);

    return (ssize_t) len;
}
