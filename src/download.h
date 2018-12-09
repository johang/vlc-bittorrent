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
#include <forward_list>

#include "libtorrent.h"
#include "vlc.h"

#define HIGHEST_PRIORITY 7
#define HIGH_PRIORITY 6
#define SLIDING_WINDOW_SIZE 20

namespace lt = libtorrent;

class Request;

class Queue {

public:

	Queue()
	{
		vlc_mutex_init(&m_mutex);
	}

	void
	process_alert(lt::alert *alert);

	void
	add(Request *req);

	void
	remove(Request *req);

private:

	std::forward_list<Request *> m_requests;

	/**
	 * Protects m_requests.
	 */
	vlc_mutex_t m_mutex;
};

class Request {

public:

	Request(Queue& q) : m_queue(q)
	{
		vlc_sem_init(&sem, 0);

		// Queue this item so it gets all callbacks
		m_queue.add(this);
	}

	~Request()
	{
		m_queue.remove(this);
	}

	void
	wait()
	{
		while (!is_complete()) {
			if (vlc_sem_wait_i11e(&sem) != 0)
				throw std::runtime_error("Request aborted");
		}
	}

	virtual bool
	is_complete()
	{
		return false;
	}

	virtual void
	handle_alert(lt::alert *alert)
	{
	}

protected:

	void
	complete()
	{
		vlc_sem_post(&sem);
	}

private:

	Queue& m_queue;

	vlc_sem_t sem;
};

class Add_Request : public Request {

public:

	Add_Request(Queue& q, lt::torrent_handle h) : Request(q), m_handle(h)
	{
	}

	bool
	is_complete();

	void
	handle_alert(lt::alert *a);

private:

	lt::torrent_handle m_handle;
};

class Read_Request : public Request {

public:

	Read_Request(Queue& q, lt::torrent_handle& h, lt::peer_request& p,
			char *b, size_t bl) : Request(q), handle(h), part(p), buf(b),
				buflen(bl)
	{
		// TODO: bounds check piece

		if (!handle.have_piece(part.piece))
			throw std::runtime_error("Can't read a piece we don̈́'t have");

		handle.read_piece(part.piece);
	}

	bool
	is_complete();

	void
	handle_alert(lt::alert *a);

	ssize_t
	get_size() {
		return size;
	}

private:

	lt::torrent_handle handle;

	lt::peer_request part;

	char *buf;

	size_t buflen;

	// Number of bytes copied
	ssize_t size = 0;
};

class Download_Request : public Request {

public:

	Download_Request(Queue& q, lt::torrent_handle& h, lt::peer_request& p) :
			Request(q), handle(h), part(p)
	{
		handle.piece_priority(part.piece, HIGHEST_PRIORITY);
	}

	bool
	is_complete();

	void
	handle_alert(lt::alert *a);

private:

	lt::torrent_handle handle;

	lt::peer_request part;
};

class Download {

public:

	Download();
	~Download();

	void
	load(std::string uri, std::string save_path);

	void
	load(char *metadata, size_t metadatalen, std::string save_path);

	/**
	 * Get a part of the data of this download. If the data is not available,
	 * it will download it and wait for it to become available.
	 */
	ssize_t
	read(int file, uint64_t off, char *buf, size_t buflen);

	void
	move_window(int piece);

	void
	move_window();

	std::vector<std::pair<std::string,uint64_t> >
	get_files();

	uint64_t
	get_file_size_by_index(int index);

	int
	get_file_index_by_path(std::string path);

	std::string
	get_name();

	std::string
	get_infohash();

	std::shared_ptr<std::vector<char> >
	get_metadata();

	void
	handle_alert(lt::alert *alert);

	friend void
	libtorrent_add_download(Download *dl, lt::add_torrent_params& atp);

	friend void
	libtorrent_remove_download(Download *dl);

private:

	/**
	 * First unfinished piece.
	 */
	int m_window_start = 0;

	lt::session *m_session;

	/**
	 * Active download. May be invalid.
	 */
	lt::torrent_handle m_torrent_handle;

	Queue m_queue;

	void
	add(lt::add_torrent_params& atp);
};

#endif
