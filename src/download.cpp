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

#include <chrono>
#include <fstream>
#include <future>
#include <limits>
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

#define kB (1024)
#define MB (1024 * kB)

#define PRIO_HIGHEST 7
#define PRIO_HIGHER 6
#define PRIO_HIGH 5

namespace lt = libtorrent;

static std::string
to_hex(const lt::sha1_hash& ih)
{
    static const char chars[] = "0123456789abcdef";

    std::string result;
    result.reserve(40);
    for (auto byte : ih.to_string()) {
        result += chars[(byte >> 4) & 0x0F];
        result += chars[byte & 0x0F];
    }
    return result;
}

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
    AlertSubscriber(std::shared_ptr<Session> dl, T* pr)
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
    std::shared_ptr<Session> m_session;

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
        if (auto* x = lt::alert_cast<lt::read_piece_alert>(a)) {
            if (x->handle.info_hash() != m_ih)
                return;

            if (x->piece != m_piece)
                return;

            if (x->error)
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
        if (auto* x = lt::alert_cast<lt::piece_finished_alert>(a)) {
            if (x->handle.info_hash() != m_ih)
                return;

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
        if (auto* x = lt::alert_cast<lt::torrent_error_alert>(a)) {
            if (x->handle.info_hash() != m_ih)
                return;

            set_exception(
                std::make_exception_ptr(std::runtime_error("metadata failed")));
        } else if (auto* x = lt::alert_cast<lt::metadata_failed_alert>(a)) {
            if (x->handle.info_hash() != m_ih)
                return;

            set_exception(
                std::make_exception_ptr(std::runtime_error("metadata failed")));
        } else if (auto* x = lt::alert_cast<lt::metadata_received_alert>(a)) {
            if (x->handle.info_hash() != m_ih)
                return;

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
        if (auto* x = lt::alert_cast<lt::torrent_removed_alert>(a)) {
            if (x->info_hashes.get_best() != m_ih)
                return;

            // Remove is done
            set_value();
        }
    }

private:
    lt::sha1_hash m_ih;
};

Download::Download(std::mutex& mtx, lt::add_torrent_params& atp, bool k)
    : m_lock(mtx)
    , m_keep(k)
    , m_session(Session::get())
{
    D(printf("%s:%d: %s (from atp)\n", __FILE__, __LINE__, __func__));

    // Doesn't matter if it's duplicate since we never remove torrents
    m_th = m_session->add_torrent(atp);
    if (!m_th.is_valid())
        throw std::runtime_error("Failed to add torrent");

    // Need to give libtorrent some time to breethe
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

Download::~Download()
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    if (m_th.is_valid()) {
        RemovePromise rmprom(m_th.info_hash());
        AlertSubscriber<RemovePromise> sub(m_session, &rmprom);

        auto f = rmprom.get_future();

        m_session->remove_torrent(m_th, m_keep);

        // Wait for remove to complete
        f.wait_for(std::chrono::seconds(5));
    }
}

ssize_t
Download::read(int file, int64_t fileoff, char* buf, size_t buflen,
    DataProgressCb progress_cb)
{
    D(printf("%s:%d: %s(%d, %lu, %p, %lu)\n", __FILE__, __LINE__, __func__,
        file, fileoff, buf, buflen));

    download_metadata();

    auto ti = m_th.torrent_file();

    auto fs = ti->files();

    if (file >= fs.num_files() || file < 0)
        throw std::runtime_error("File not found");

    if (fileoff < 0)
        throw std::runtime_error("File offset negative");

    int64_t filesz = fs.file_size(file);
    if (fileoff >= filesz)
        return 0;

    // Figure out what to read
    auto part = ti->map_file(file, fileoff,
        (int) std::min({ (int64_t) std::numeric_limits<int>::max(),
            (int64_t) buflen, filesz - fileoff }));
    if (part.length <= 0)
        return 0;

    // Set highest priority to the requested range
    set_piece_priority(file, fileoff, part.length, PRIO_HIGHEST);

    // Set second highest priority to the first and last 0.1% or 64 kB
    int64_t p01 = std::max(
        std::min((int64_t) std::numeric_limits<int>::max(), filesz / 1000),
        (int64_t) 128 * kB);
    set_piece_priority(file, 0, (int) p01, PRIO_HIGHER);
    set_piece_priority(file, filesz - p01, (int) p01, PRIO_HIGHER);

    // Set third highest priority to the next 5% or 64 MB
    int64_t p5 = std::max(
        std::min((int64_t) std::numeric_limits<int>::max(), 5 * filesz / 100),
        (int64_t) 32 * MB);
    set_piece_priority(file, fileoff, (int) p5, PRIO_HIGH);

    if (!m_th.have_piece(part.piece))
        download(part, progress_cb);

    return read(part, buf, buflen);
}

void
Download::set_piece_priority(int file, int64_t off, int size, libtorrent::download_priority_t prio)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    download_metadata();

    auto ti = m_th.torrent_file();

    auto fs = ti->files();

    // Make sure off + size <= file size
    int64_t filesz = fs.file_size(file);
    off = std::min(off, filesz);
    size = (int) std::min({ (int64_t) std::numeric_limits<int>::max(),
        (int64_t) size, filesz - off });

    auto part = ti->map_file(file, off, size);
    for (; part.length > 0; part.length -= ti->piece_size(part.piece++)) {
        if (!m_th.have_piece(part.piece)
            && m_th.piece_priority(part.piece) < prio)
            m_th.piece_priority(part.piece, prio);
    }
}

std::vector<std::pair<std::string, uint64_t>>
Download::get_files()
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    download_metadata();

    std::vector<std::pair<std::string, uint64_t>> files;

    const lt::file_storage& fs = m_th.torrent_file()->files();
    for (int i = 0; i < fs.num_files(); i++) {
        files.push_back(std::make_pair(fs.file_path(i), fs.file_size(i)));
    }

    return files;
}

// static
std::vector<std::pair<std::string, uint64_t>>
Download::get_files(char* metadata, size_t metadatasz)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

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

// static
std::shared_ptr<std::vector<char>>
Download::get_metadata(std::string url, std::string save_path,
    std::string cache_path, MetadataProgressCb cb)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    lt::add_torrent_params atp;
    atp.save_path = save_path;
    atp.flags &= ~lt::torrent_flags::auto_managed;
    atp.flags &= ~lt::torrent_flags::paused;

    lt::error_code ec;

    lt::parse_magnet_uri(url, atp, ec);
    if (ec) {
        lt::error_code ec2;
#if LIBTORRENT_VERSION_NUM < 10200
        atp.ti = boost::make_shared<lt::torrent_info>(url, boost::ref(ec2));
#else
        atp.ti = std::make_shared<lt::torrent_info>(url, std::ref(ec2));
#endif
        if (ec2)
            throw std::runtime_error("Failed to parse metadata");
    } else {
        std::string path = cache_path + DIR_SEP + to_hex(atp.info_hashes.get_best()) + ".torrent";

        // Try to read up cache
#if LIBTORRENT_VERSION_NUM < 10200
        atp.ti = boost::make_shared<lt::torrent_info>(path, boost::ref(ec));
#else
        atp.ti = std::make_shared<lt::torrent_info>(path, std::ref(ec));
#endif
        if (ec) {
            // The torrent_info from was is invalid
            atp.ti = NULL;

            // Dowload metadata
            auto metadata = Download::get_download(atp, true)->get_metadata(cb);

            // Write metadata to cache
            std::ofstream os(path, std::ios::binary);
            std::ostream_iterator<char> osi(os);
            std::copy(std::begin(*metadata), std::end(*metadata), osi);

            return metadata;
        }
    }

    // Add trackers from magnet URL to the cached metadata
    for (auto& tracker : atp.trackers) {
        atp.ti->add_tracker(tracker);
    }

    auto entry = lt::create_torrent(*atp.ti).generate();

    auto metadata = std::make_shared<std::vector<char>>();

    // Bencode metadata into vector
    lt::bencode(std::back_inserter(*metadata), entry);

    return metadata;
}

// static
std::shared_ptr<Download>
Download::get_download(lt::add_torrent_params& atp, bool k)
{
    D(printf("%s:%d: %s (from atp)\n", __FILE__, __LINE__, __func__));

    lt::sha1_hash ih = atp.ti ? atp.ti->info_hash() : atp.info_hashes.get_best();

    static std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);

    // Re-use Download instance if possible, else create new instance
    static std::map<lt::sha1_hash, std::weak_ptr<Download>> dls;
    static std::map<lt::sha1_hash, std::mutex> dls_mtx;
    std::shared_ptr<Download> dl = dls[ih].lock();
    if (!dl)
        dls[ih] = dl = std::make_shared<Download>(dls_mtx[ih], atp, k);

    return dl;
}

// static
std::shared_ptr<Download>
Download::get_download(char* md, size_t mdsz, std::string sp, bool k)
{
    D(printf("%s:%d: %s (from buf)\n", __FILE__, __LINE__, __func__));

    lt::add_torrent_params atp;
    atp.save_path = sp;
    atp.flags &= ~lt::torrent_flags::auto_managed;
    atp.flags &= ~lt::torrent_flags::paused;
    atp.flags &= ~lt::torrent_flags::duplicate_is_error;

    lt::error_code ec;
#if LIBTORRENT_VERSION_NUM < 10200
    atp.ti = boost::make_shared<lt::torrent_info>(md, mdsz, boost::ref(ec));
#else
    atp.ti = std::make_shared<lt::torrent_info>(md, mdsz, std::ref(ec));
#endif
    if (ec)
        throw std::runtime_error("Failed to parse metadata");

    return Download::get_download(atp, k);
}

std::pair<int, uint64_t>
Download::get_file(std::string path)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    download_metadata();

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
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    download_metadata();

    return m_th.torrent_file()->name();
}

std::string
Download::get_infohash()
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    download_metadata();

    return to_hex(m_th.torrent_file()->info_hashes().get_best());
}

std::shared_ptr<std::vector<char>>
Download::get_metadata(MetadataProgressCb cb)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    download_metadata(cb);

    auto entry = lt::create_torrent(*m_th.torrent_file()).generate();

    auto buffer = std::make_shared<std::vector<char>>();

    // Bencode metadata into vector
    lt::bencode(std::back_inserter(*buffer), entry);

    return buffer;
}

void
Download::download_metadata(MetadataProgressCb cb)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    if (m_th.status().has_metadata)
        return;

    MetadataDownloadPromise dlprom(m_th.info_hash());
    AlertSubscriber<MetadataDownloadPromise> sub(m_session, &dlprom);
    vlc_interrupt_guard<MetadataDownloadPromise> intrguard(dlprom);

    auto f = dlprom.get_future();

    if (cb)
        cb(0.0);

    // Wait for metadata to download
    while (!m_th.status().has_metadata) {
        auto r = f.wait_for(std::chrono::seconds(1));
        if (r == std::future_status::ready)
            // At this point, we know either metadata download is done and we
            // can return early, or there was error and get() will throw and
            // exception.
            return f.get();
    }

    if (cb)
        cb(100.0);
}

void
Download::download(lt::peer_request part, DataProgressCb cb)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    download_metadata();

    if (m_th.have_piece(part.piece))
        return;

    DownloadPiecePromise dlprom(m_th.info_hash(), part.piece);
    AlertSubscriber<DownloadPiecePromise> sub(m_session, &dlprom);
    vlc_interrupt_guard<DownloadPiecePromise> intrguard(dlprom);

    auto f = dlprom.get_future();

    if (cb)
        cb(0.0);

    // Wait for download
    while (!m_th.have_piece(part.piece)) {
        auto r = f.wait_for(std::chrono::seconds(1));
        if (r == std::future_status::ready)
            // At this point, we know either download is done and we can return
            // early, or there was error and get() will throw and exception.
            return f.get();
    }

    if (cb)
        cb(100.0);
}

ssize_t
Download::read(lt::peer_request part, char* buf, size_t buflen)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    download_metadata();

    ReadPiecePromise rdprom(m_th.info_hash(), part.piece);
    AlertSubscriber<ReadPiecePromise> sub(m_session, &rdprom);
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
