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

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <list>
#include <thread>

#include "libtorrent.h"
#include "download.h"

#define D(x)
#define DD(x)

#define LIBTORRENT_ADD_TORRENT_FLAGS ( \
	lt::session::add_default_plugins | \
	lt::session::start_default_features)

#define LIBTORRENT_ADD_TORRENT_ALERTS ( \
	lt::alert::storage_notification | \
	lt::alert::progress_notification | \
	lt::alert::status_notification | \
	lt::alert::error_notification)

#define LIBTORRENT_DHT_NODES ( \
	"router.bittorrent.com:6881," \
	"router.utorrent.com:6881," \
	"dht.transmissionbt.com:6881")

static lt::session *g_session;

static std::thread *g_session_thread;

static std::list<Download *> g_downloads;

static std::mutex g_mutex_session;

static std::mutex g_mutex_downloads;

static void
destroy_session(bool keep)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	for (auto th : g_session->get_torrents()) {
		if (keep)
			g_session->remove_torrent(th);
		else
			g_session->remove_torrent(th, libtorrent::session::delete_files);
	}

	// Free the session object -- this might block for a short while
	delete g_session;

	g_session = NULL;
}

static void
destroy_session_thread()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	g_session_thread->join();

	delete g_session_thread;

	g_session_thread = NULL;
}

static void
session_thread()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	while (1) {
		g_session->wait_for_alert(std::chrono::seconds(1));

		std::vector<lt::alert *> alerts;

		// Get all pending requests
		g_session->pop_alerts(&alerts);

		std::unique_lock<std::mutex> lock(g_mutex_downloads);

		for (auto *a : alerts) {
			DD(std::cout << "got alert (" << typeid(*a).name() << "): " <<
				a->message() << std::endl);

			for (Download *d : g_downloads) {
				d->handle_alert(a);
			}
		}

		if (g_downloads.size() == 0)
			break;
	}
}

static void
create_session()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	lt::settings_pack sp;

	sp.set_int(sp.alert_mask, LIBTORRENT_ADD_TORRENT_ALERTS);
	sp.set_str(sp.dht_bootstrap_nodes, LIBTORRENT_DHT_NODES);

	/* Really aggressive settings to optimize time-to-play */
	sp.set_bool(sp.strict_end_game_mode, false);
	sp.set_bool(sp.announce_to_all_trackers, true);
	sp.set_bool(sp.announce_to_all_tiers, true);
	sp.set_int(sp.stop_tracker_timeout, 1);
	sp.set_int(sp.request_timeout, 2);
	sp.set_int(sp.whole_pieces_threshold, 5);
	sp.set_int(sp.request_queue_time, 1);
	sp.set_int(sp.urlseed_pipeline_size, 2);
#if LIBTORRENT_VERSION_NUM >= 10102
	sp.set_int(sp.urlseed_max_request_bytes, 100 * 1024);
#endif

	g_session = new lt::session(sp, LIBTORRENT_ADD_TORRENT_FLAGS);
}

static void
create_session_thread()
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	g_session_thread = new std::thread(session_thread);
}

void
libtorrent_add_download(Download *dl, lt::add_torrent_params& atp)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	std::lock_guard<std::mutex> lock(g_mutex_session);

	{
		std::lock_guard<std::mutex> lock(g_mutex_downloads);

		// Add download to the list of downloads that gets alerts
		g_downloads.push_front(dl);
	}

	if (!g_session)
		create_session();

	if (!g_session_thread)
		create_session_thread();

	// Add torrent (possibly a duplicate)
	dl->m_torrent_handle = g_session->add_torrent(atp);
}

void
libtorrent_remove_download(Download *dl, bool keep)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	bool empty_session;

	std::lock_guard<std::mutex> lock(g_mutex_session);

	{
		std::lock_guard<std::mutex> lock(g_mutex_downloads);

		// Remove download from the list of downloads that gets alerts
		g_downloads.remove(dl);

		empty_session = g_downloads.size() == 0;
	}

	if (empty_session) {
		destroy_session_thread();

		destroy_session(keep);
	}
}
