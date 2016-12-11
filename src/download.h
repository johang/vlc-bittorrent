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

#include <queue>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

#include "libtorrent.h"
#include "vlc.h"

struct DownloadSession;

struct QueueClosedException : public std::runtime_error {
	QueueClosedException() :
		std::runtime_error("Queue is closed")
	{
	}
};

template <typename T>
struct FIFO {
	std::queue<T> m_elements;

	std::mutex m_mutex;

	std::condition_variable m_condition;

	bool m_closed;

	FIFO() :
		m_closed(false)
	{
	}

	void
	add(T a) {
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_closed)
			return;

		// Put item at end of queue
		m_elements.push(a);

		// Wake up one waiting thread
		m_condition.notify_one();
	}

	void
	close() {
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_closed)
			return;

		// Mark this queue as closed
		m_closed = true;

		// Wake up all waiting threads
		m_condition.notify_all();
	}

	T
	remove() {
		std::unique_lock<std::mutex> lock(m_mutex);

		while (m_elements.empty()) {
			if (m_closed)
				throw QueueClosedException();

			// Wait until queue is non-empty
			m_condition.wait(lock);
		}

		// Get oldest item in queue
		T item = m_elements.front();

		// Remove oldest item in queue
		m_elements.pop();

		return item;
	}

	bool
	is_closed() {
		std::lock_guard<std::mutex> lock(m_mutex);

		return m_closed;
	}
};

typedef FIFO<std::shared_ptr<libtorrent::alert>> AFIFO;

struct SlidingWindowStrategy {
	std::recursive_mutex m_mutex_first;

	std::shared_ptr<AFIFO> m_queue;

	libtorrent::torrent_handle m_handle;

	std::thread m_thread;

	/**
	 * First piece in the sliding window.
	 */
	int m_first;

	/**
	 * Number of pieces in the sliding window.
	 */
	int m_window_size;

	/**
	 * Number of piece in the download.
	 */
	int m_download_size;

	SlidingWindowStrategy(std::shared_ptr<AFIFO> q,
		libtorrent::torrent_handle h);
	~SlidingWindowStrategy();

	void
	loop();

	/**
	 * Move sliding window to start at a given piece.
	 */
	void
	move(int piece);

	/**
	 * Move sliding window one step forward.
	 */
	void
	move();
};

struct Download {
	DownloadSession *m_session;

	libtorrent::torrent_handle m_handle;

	SlidingWindowStrategy m_strategy;

	Download(DownloadSession *s, libtorrent::torrent_handle h);
	~Download();

	/**
	 * Write metadata to file.
	 */
	void
	dump(std::string path);

	/**
	 * Get a list of all files.
	 */
	std::vector<std::string>
	list();

	/**
	 * Get download name.
	 */
	std::string
	name();

	/**
	 * Start download of a piece and wait for it to complete.
	 */
	void
	download_piece(int piece);

	/**
	 * Start read of a piece and wait for it to complete.
	 */
	ssize_t
	read_piece(libtorrent::peer_request req, char *buf, size_t buflen);

	/**
	 * Start read of a part of a piece and wait for it to complete.
	 */
	ssize_t
	read(int file, uint64_t pos, char *buf, size_t buflen);
};

struct DownloadSession {
	std::list<std::weak_ptr<AFIFO>> m_queues;

	std::mutex m_mutex_queues;

	libtorrent::session *m_session;

	DownloadSession();
	~DownloadSession();

	Download*
	add(libtorrent::add_torrent_params params);

	Download*
	add(std::string url, bool paused = false);

	Download*
	add(char *buf, size_t buflen, bool paused = false);

	/**
	 * Subscribe to all events emitted from this session.
	 */
	std::shared_ptr<AFIFO>
	subscribe();
};

#endif
