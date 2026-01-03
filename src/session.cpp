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

#include "session.h"

#define D(x)
#define DD(x)

#define LIBTORRENT_ADD_TORRENT_ALERTS \
    (lt::alert::storage_notification | lt::alert::piece_progress_notification \
        | lt::alert::status_notification | lt::alert::error_notification)

#define LIBTORRENT_DHT_NODES \
    ("router.bittorrent.com:6881," \
     "router.utorrent.com:6881," \
     "dht.transmissionbt.com:6881")

Session::Session(std::mutex& mtx)
    : m_lock(mtx)
    , m_session_thread_quit(false)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    lt::settings_pack sp = lt::default_settings();

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

    m_session = std::make_unique<lt::session>(sp);

    m_session_thread = std::thread([&] {
        while (!m_session_thread_quit) {
            m_session->wait_for_alert(std::chrono::seconds(1));

            std::vector<lt::alert*> alerts;

            // Get all pending requests
            m_session->pop_alerts(&alerts);

            for (auto* a : alerts) {
                std::unique_lock<std::mutex> lock(m_listeners_mtx);

                for (auto* h : m_listeners) {
                    try {
                        h->handle_alert(a);
                    } catch (...) {
                    }
                }
            }
        }
    });
}

Session::~Session()
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    m_session_thread_quit = true;

    if (m_session_thread.joinable())
        m_session_thread.join();
}

void
Session::register_alert_listener(Alert_Listener* al)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    std::unique_lock<std::mutex> lock(m_listeners_mtx);

    m_listeners.push_front(al);
}

void
Session::unregister_alert_listener(Alert_Listener* al)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    std::unique_lock<std::mutex> lock(m_listeners_mtx);

    m_listeners.remove(al);
}

lt::torrent_handle
Session::add_torrent(lt::add_torrent_params& atp)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    return m_session->add_torrent(atp);
}

void
Session::remove_torrent(lt::torrent_handle& th, bool k)
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    if (k)
        m_session->remove_torrent(th);
    else
        m_session->remove_torrent(th, lt::session::delete_files);
}

std::shared_ptr<Session>
Session::get()
{
    D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

    static std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);

    // Re-use Session instance if possible, else create new instance
    static std::weak_ptr<Session> session;
    static std::mutex session_mtx;
    std::shared_ptr<Session> s = session.lock();
    if (!s)
        session = s = std::make_shared<Session>(session_mtx);

    return s;
}
