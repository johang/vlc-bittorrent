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

#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fstream>

#include "libtorrent.h"
#include "vlc.h"

#include "download.h"
#include "metadata.h"
#include "magnetmetadata.h"
#include "data.h"

#define D(x)
#define DD(x)

#define LIBTORRENT_ADD_TORRENT_FLAGS ( \
	libtorrent::session::add_default_plugins | \
	libtorrent::session::start_default_features)

#define LIBTORRENT_ADD_TORRENT_ALERTS ( \
	libtorrent::alert::storage_notification | \
	libtorrent::alert::progress_notification | \
	libtorrent::alert::status_notification | \
	libtorrent::alert::error_notification)

#define SLIDING_WINDOW_SIZE (8 * 1024 * 1024) /* 8 MB */

using namespace libtorrent;

SlidingWindowStrategy::SlidingWindowStrategy(std::shared_ptr<AFIFO> q,
			torrent_handle h) :
	m_queue(q),
	m_handle(h),
	m_thread(&SlidingWindowStrategy::loop, this),
	m_first(-1)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

#if LIBTORRENT_VERSION_NUM < 10100
	boost::intrusive_ptr<torrent_info const> ti = m_handle.torrent_file();
#else
	boost::shared_ptr<const torrent_info> ti = m_handle.torrent_file();
#endif

	// TODO: variable window size (small right after seek, but growing)
	m_download_size = ti->num_pieces();
	m_window_size = std::max(SLIDING_WINDOW_SIZE / ti->piece_length(), 1);
}

SlidingWindowStrategy::~SlidingWindowStrategy()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	// Close the queue (this will cause m_thread to terminate)
	m_queue->close();

	// Wait for the thread to terminate
	m_thread.join();
}

void
SlidingWindowStrategy::loop()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	while (!m_queue->is_closed()) {
		try {
			std::shared_ptr<alert> a = m_queue->remove();

			if (alert_cast<piece_finished_alert>(a.get())) {
				auto *b = alert_cast<piece_finished_alert>(a.get());

				if (b->handle == m_handle)
					move();
			}
		} catch (QueueClosedException& e) {
			return;
		}
	}
}

void
SlidingWindowStrategy::move()
{
	DD(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	std::lock_guard<std::recursive_mutex> lock(m_mutex_first);

	// Try to move the window one or more steps
	move(m_first);
}

void
SlidingWindowStrategy::move(int piece)
{
	DD(printf("%s:%d: %s(%d)\n", __FILE__, __LINE__, __func__, piece));

	std::lock_guard<std::recursive_mutex> lock(m_mutex_first);

	// Scroll past all finished pieces
	for (; m_handle.have_piece(piece) && piece < m_download_size; piece++);

	if (m_first == piece)
		// Window was not changed
		return;

	m_first = piece;

	D(printf("actually moved window to %d\n", m_first));

	for (int p = m_first; p < m_first + m_window_size &&
			p < m_download_size; p++) {
		// Set highest priority (even if already finished)
		m_handle.piece_priority(p, 7);
	}

}

Download::Download(DownloadSession *s, torrent_handle h) :
	m_session(s),
	m_handle(h),
	m_strategy(m_session->subscribe(), m_handle)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	int num_files = m_handle.torrent_file()->num_files();

	for (int i = 0; i < num_files; ++i) {
		// Disable downloading of all files until a read happens
		m_handle.file_priority(i, 0);
	}
}

Download::~Download()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));
}

void
Download::download_piece(int piece)
{
	DD(printf("%s:%d: %s(%d)\n", __FILE__, __LINE__, __func__, piece));

	std::shared_ptr<AFIFO> queue = m_session->subscribe();

	// Move sliding window (this will trigger download)
	m_strategy.move(piece);

	if (m_handle.have_piece(piece))
		return;

	while (!queue->is_closed()) {
		try {
			std::shared_ptr<alert> a = queue->remove();

			if (alert_cast<piece_finished_alert>(a.get())) {
				auto *b = alert_cast<piece_finished_alert>(a.get());

				if (b->handle == m_handle && b->piece_index == piece) {
					// Piece has been downloaded -- return
					return;
				}
			}
		} catch (QueueClosedException& e) {
			return;
		}
	}
}

ssize_t
Download::read_piece(peer_request req, char *buf, size_t buflen)
{
	DD(printf("%s:%d: %s(%d)\n", __FILE__, __LINE__, __func__, req.piece));

	// Make sure it's downloaded
	download_piece(req.piece);

	std::shared_ptr<AFIFO> queue = m_session->subscribe();

	// Trigger read
	m_handle.read_piece(req.piece);

	while (!queue->is_closed()) {
		try {
			std::shared_ptr<alert> a = queue->remove();

			if (alert_cast<read_piece_alert>(a.get())) {
				auto *b = alert_cast<read_piece_alert>(a.get());

				if (b->handle == m_handle && b->piece == req.piece) {
					// Number of bytes to copy
					size_t size = std::min((size_t) (b->size - req.start),
						buflen);

					// Copy part of the piece into the supplied buffer
					memcpy(buf, b->buffer.get() + req.start, size);

					// Piece has been read and copied -- return
					return (ssize_t) size;
				}
			}
		} catch (QueueClosedException& e) {
			return -1;
		}
	}

	return -1;
}

ssize_t
Download::read(int file, uint64_t pos, char *buf, size_t buflen)
{
	D(printf("%s:%d: %s(%d, %lu)\n", __FILE__, __LINE__, __func__, file, pos));

#if LIBTORRENT_VERSION_NUM < 10100
	boost::intrusive_ptr<torrent_info const> ti = m_handle.torrent_file();
#else
	boost::shared_ptr<const torrent_info> ti = m_handle.torrent_file();
#endif

	if (file >= ti->num_files())
		return 0;

	if (pos >= (uint64_t) ti->file_at(file).size)
		return 0;

#if LIBTORRENT_VERSION_NUM < 10100
	peer_request req = ti->map_file(file, (size_type) pos, (int) buflen);
#else
	peer_request req = ti->map_file(file, (boost::int64_t) pos, (int) buflen);
#endif

	if (req.piece >= ti->num_pieces())
		return 0;

	// Download, read and copy this part into buf
	return read_piece(req, buf, buflen);
}

uint64_t
Download::file_size(int file)
{
	return (uint64_t) m_handle.torrent_file()->files().file_size(file);
}

std::string
Download::name()
{
	return m_handle.torrent_file()->name();
}

std::vector<char>
Download::get_metadata()
{
	std::vector<char> md;

	create_torrent t(*m_handle.torrent_file());

	// Stream out metadata to vector
	bencode(std::back_inserter(md), t.generate());

	return md;
}

std::vector<std::string>
Download::list()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	std::vector<std::string> files;

#if LIBTORRENT_VERSION_NUM < 10100
	boost::intrusive_ptr<torrent_info const> ti = m_handle.torrent_file();
#else
	boost::shared_ptr<const torrent_info> ti = m_handle.torrent_file();
#endif

	for (int i = 0; i < ti->num_files(); i++) {
		files.push_back(ti->file_at(i).path);
	}

	return files;
}

DownloadSession::DownloadSession()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

#if LIBTORRENT_VERSION_NUM < 10100
	// TODO: proper version
	m_session = new session(
		fingerprint(
			"VL",
			1,
			0,
			0,
			0),
		std::make_pair(10000, 12000),
		"0.0.0.0",
		LIBTORRENT_ADD_TORRENT_FLAGS,
		LIBTORRENT_ADD_TORRENT_ALERTS);

	session_settings ss = m_session->settings();

	ss.strict_end_game_mode = false;
	ss.announce_to_all_trackers = true;
	ss.announce_to_all_tiers = true;

	m_session->set_settings(ss);
#else
	settings_pack p;

	p.set_int(settings_pack::alert_mask, LIBTORRENT_ADD_TORRENT_ALERTS);
	p.set_bool(settings_pack::strict_end_game_mode, false);
	p.set_bool(settings_pack::announce_to_all_trackers, true);
	p.set_bool(settings_pack::announce_to_all_tiers, true);
	p.set_int(settings_pack::stop_tracker_timeout, 1);
	p.set_int(settings_pack::piece_timeout, 20);

	m_session = new session(p, LIBTORRENT_ADD_TORRENT_FLAGS);
#endif

	m_session->add_dht_router(std::make_pair(
		"router.bittorrent.com", 6881));
	m_session->add_dht_router(std::make_pair(
		"router.utorrent.com", 6881));
	m_session->add_dht_router(std::make_pair(
		"dht.transmissionbt.com", 6881));

	m_session->set_alert_dispatch([&](std::auto_ptr<alert> a) {
		std::lock_guard<std::mutex> lock(m_mutex_queues);

		m_queues.remove_if([] (const std::weak_ptr<AFIFO>& f) {
			return f.expired();
		});

		for (std::list<std::weak_ptr<AFIFO>>::iterator i = m_queues.begin();
				i != m_queues.end(); ++i) {
			std::shared_ptr<AFIFO> f = i->lock();

			if (f)
				f->add(a->clone());
		}
	});
}

DownloadSession::~DownloadSession()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	delete m_session;
}

Download*
DownloadSession::add(add_torrent_params params)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	std::shared_ptr<AFIFO> queue = subscribe();

	error_code ec;

	torrent_handle handle = m_session->add_torrent(params, ec);

	if (ec) {
		std::cout << "Failed to add torrent" << std::endl;
		return NULL;
	}

	while (!queue->is_closed()) {
		try {
			std::shared_ptr<alert> a = queue->remove();

			if (alert_cast<state_changed_alert>(a.get())) {
				auto *b = alert_cast<state_changed_alert>(a.get());

				if (b->handle == handle) {
					switch (b->state) {
					case torrent_status::downloading:
					case torrent_status::finished:
					case torrent_status::seeding:
						return new Download(this, handle);
					default:
						break;
					}
				}
			} else if (alert_cast<metadata_failed_alert>(a.get())) {
				auto *b = alert_cast<metadata_failed_alert>(a.get());

				if (b->handle == handle) {
					return NULL;
				}
			}
		} catch (QueueClosedException& e) {
			return NULL;
		}
	}

	return NULL;
}

std::shared_ptr<AFIFO>
DownloadSession::subscribe()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	std::lock_guard<std::mutex> lock(m_mutex_queues);

	// Allocate a new queue
	std::shared_ptr<AFIFO> queue = std::make_shared<AFIFO>();

	// Put a weak pointer to it in the list of queues
	m_queues.push_back(queue);

	// Return the shared pointer to the user
	return queue;
}
