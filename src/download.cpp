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

#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fstream>
#include <thread>
#include <chrono>
#include <stdexcept>

#include "libtorrent.h"
#include "vlc.h"

#include "download.h"

#define D(x)
#define DD(x)

#define HIGHEST_PRIORITY 7
#define HIGH_PRIORITY 6
#define LOW_PRIORITY 1
#define LOWEST_PRIORITY 0

namespace lt = libtorrent;

bool Add_Request::is_complete() {
	if (!m_handle.is_valid())
		return false;

	lt::torrent_status s = m_handle.status();

	if (s.errc)
		throw std::runtime_error("Failed to add: " + s.errc.message());

	return s.has_metadata;
}

void Add_Request::handle_alert(lt::alert *a) {
	if (lt::alert_cast<lt::state_changed_alert>(a)) {
		auto *x = lt::alert_cast<lt::state_changed_alert>(a);

		switch (x->state) {
		case lt::torrent_status::downloading:
		case lt::torrent_status::finished:
		case lt::torrent_status::seeding:
			complete();
			break;
		default:
			break;
		}
	} else if (lt::alert_cast<lt::torrent_error_alert>(a)) {
		complete();
	} else if (lt::alert_cast<lt::metadata_failed_alert>(a)) {
		complete();
	} else if (lt::alert_cast<lt::metadata_received_alert>(a)) {
		complete();
	}
}

bool Read_Request::is_complete() {
	return size != 0;
}

void Read_Request::handle_alert(lt::alert *a) {
	if (lt::alert_cast<lt::read_piece_alert>(a)) {
		auto *x = lt::alert_cast<lt::read_piece_alert>(a);

		if (x->piece != part.piece)
			return;

		if (x->buffer) {
			// Number of bytes to copy
			size = std::min<ssize_t>({
				(x->size - part.start),
				(ssize_t) buflen,
				part.length });

			// Copy part of the piece into the supplied buffer
			memcpy(buf, x->buffer.get() + part.start, (size_t) size);
		} else {
			// Signal some kind of error
			size = -EIO;
		}

		complete();
	}
}

bool Download_Request::is_complete() {
	return handle.have_piece(part.piece);
}

void Download_Request::handle_alert(lt::alert *a) {
	if (lt::alert_cast<lt::piece_finished_alert>(a)) {
		// XXX: This alert is unpredictable: Sometimes piece_finished_alert is
		//      posted before torrent_handle::have_piece will returns true)

		auto *x = lt::alert_cast<lt::piece_finished_alert>(a);

		if (x->piece_index == part.piece)
			complete();
	} else if (lt::alert_cast<lt::block_finished_alert>(a)) {
		// XXX: Listening for block_finished_alert seems more predictable than
		//      piece_finished_alert, but is posted more often

		auto *x = lt::alert_cast<lt::block_finished_alert>(a);

		if (x->piece_index == part.piece)
			complete();
	}
}

void Queue::process_alert(lt::alert *a) {
	vlc_mutex_locker lock(&m_mutex);

	for (Request *r : m_requests) {
		r->handle_alert(a);
	}
}

void Queue::add(Request *req) {
	vlc_mutex_locker lock(&m_mutex);

	m_requests.push_front(req);
}

void Queue::remove(Request *req) {
	vlc_mutex_locker lock(&m_mutex);

	m_requests.remove(req);
}

Download::Download()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));
}

Download::~Download()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	libtorrent_remove_download(this);
}

void
Download::add(lt::add_torrent_params& atp)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	atp.flags &= ~lt::add_torrent_params::flag_auto_managed;
	atp.flags &= ~lt::add_torrent_params::flag_paused;

	libtorrent_add_download(this, atp);

	Add_Request areq(m_queue, m_torrent_handle);
	areq.wait();

	auto info = m_torrent_handle.torrent_file();

#if 0
	for (int i = 0; i < info->num_files(); i++) {
		m_torrent_handle.file_priority(i, 0);
	}
#endif

	// Some files have an important index at the beginning or end of the file
	for (int i = 0; i < info->num_files(); i++) {
		download_range(i, 0, 128*1024);
		download_range(i, -128*1024, 128*1024);
	}
}

void
Download::load(std::string uri, std::string save_path)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	lt::add_torrent_params atp;

	if (uri.find("magnet:") == 0) {
		lt::error_code ec;

		// Attempt to parse this magnet URI
		lt::parse_magnet_uri(uri, atp, ec);

		if (ec)
			throw std::runtime_error("Failed to parse magnet");
	} else {
		// URI is probably a file or something
		atp.url = uri;
	}

	atp.save_path = save_path;

	add(atp);
}

void
Download::load(char *buf, size_t buflen, std::string save_path)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	lt::add_torrent_params atp;

	lt::error_code ec;

#if LIBTORRENT_VERSION_NUM < 10100
	atp.ti = new libtorrent::torrent_info(buf, buflen, ec);
#elif LIBTORRENT_VERSION_NUM < 10200
	atp.ti = boost::make_shared<libtorrent::torrent_info>(buf, buflen, boost::ref(ec));
#else
	atp.ti = std::make_shared<libtorrent::torrent_info>(buf, buflen, std::ref(ec));
#endif

	if (ec)
		throw std::runtime_error("Failed to parse metadata");

	atp.save_path = save_path;

	add(atp);
}

ssize_t
Download::read(int file, uint64_t off, char *buf, size_t buflen)
{
	D(printf("%s:%d: %s(%d, %lu, %lu)\n", __FILE__, __LINE__, __func__,
		file, off, buflen));

	auto ti = m_torrent_handle.torrent_file();

	if (file >= ti->files().num_files())
		throw std::runtime_error("File not found");

	if (off >= (uint64_t) ti->files().file_size(file))
		return 0;

	download_range(file, (int64_t) off, (int64_t) buflen);

	lt::peer_request part = m_torrent_handle.torrent_file()->map_file(
		file,
		(int64_t) off,
		(int) std::min(
			ti->files().file_size(file) - (int64_t) off,
			(int64_t) buflen));

	// Move sliding window to this new position
	move_window(part.piece);

	Download_Request dlreq(m_queue, m_torrent_handle, part);
	dlreq.wait();

	Read_Request rreq(m_queue, m_torrent_handle, part, buf, buflen);
	rreq.wait();

	return (ssize_t) rreq.get_size();
}

void
Download::download_range(int file, int64_t offset, int64_t size)
{
	if (!m_torrent_handle.is_valid())
		return;

	auto ti = m_torrent_handle.torrent_file();

	// Translate negative offsets to positive
	if (offset < 0)
		offset = std::max((int64_t) 0, ti->files().file_size(file) + offset);

	// Clamp offset and size so it's within the file
	int64_t o = std::min(offset, ti->files().file_size(file));
	int64_t s = std::min(size, ti->files().file_size(file) - o);

	while (s > 0) {
		lt::peer_request part = ti->map_file(file, o, (int) s);

		m_torrent_handle.piece_priority(part.piece, HIGHEST_PRIORITY);

		// Advance offset and size
		o += std::min(part.length, ti->piece_size(part.piece) - part.start);
		s -= std::min(part.length, ti->piece_size(part.piece) - part.start);
	}
}

void
Download::move_window(int piece)
{
	if (!m_torrent_handle.is_valid())
		return;

	// Number of pieces in this download
	int np = m_torrent_handle.torrent_file()->num_pieces();

	if (piece >= np)
		return;

	// Move to first unfinished piece
	for (; m_torrent_handle.have_piece(piece) && piece < np; piece++);

	m_window_start = piece;

	for (int p = 0; p < std::max(10, (np + 1) / 20) && piece + p < np; p++) {
		m_torrent_handle.piece_priority(piece + p, HIGHEST_PRIORITY);
	}
}

void
Download::move_window()
{
	move_window(m_window_start);
}

void
Download::handle_alert(lt::alert *alert)
{
	m_queue.process_alert(alert);

	if (lt::alert_cast<lt::piece_finished_alert>(alert)) {
		move_window();
	}
}

std::vector<std::pair<std::string,uint64_t> >
Download::get_files()
{
	const lt::file_storage& fs = m_torrent_handle.torrent_file()->files();

	std::vector<std::pair<std::string,uint64_t> > files;

	for (int i = 0; i < fs.num_files(); i++) {
		files.push_back(std::make_pair(fs.file_path(i), fs.file_size(i)));
	}

	return files;
}

uint64_t
Download::get_file_size_by_index(int index)
{
	return (uint64_t) m_torrent_handle.torrent_file()->files().file_size(index);
}

int
Download::get_file_index_by_path(std::string path)
{
	const lt::file_storage& fs = m_torrent_handle.torrent_file()->files();

	std::vector<std::pair<std::string,uint64_t> > files;

	for (int i = 0; i < fs.num_files(); i++) {
		if (fs.file_path(i) == path)
			return i;
	}

	throw std::runtime_error("Path " + path + " not found");
}

std::string
Download::get_name()
{
	return m_torrent_handle.torrent_file()->name();
}

std::string
Download::get_infohash()
{
	return lt::to_hex(m_torrent_handle.torrent_file()->info_hash().to_string());
}

std::shared_ptr<std::vector<char> >
Download::get_metadata()
{
	auto buffer = std::make_shared<std::vector<char> >();

	// Bencode metadata into buffer
	lt::bencode(
		std::back_inserter(*buffer),
		lt::create_torrent(*m_torrent_handle.torrent_file()).generate());

	return buffer;
}
