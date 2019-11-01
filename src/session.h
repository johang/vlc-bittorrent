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

#ifndef VLC_BITTORRENT_LIBTORRENT_H
#define VLC_BITTORRENT_LIBTORRENT_H

#include <forward_list>
#include <mutex>
#include <thread>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include <libtorrent/alert.hpp>
#include <libtorrent/session.hpp>
#pragma GCC diagnostic pop

struct Alert_Listener {
    virtual ~Alert_Listener() { }

    virtual void
    handle_alert(lt::alert* alert)
        = 0;
};

class Session {
public:
    Session();
    ~Session();

    void
    register_alert_listener(Alert_Listener* al);

    void
    unregister_alert_listener(Alert_Listener* al);

    std::unique_ptr<lt::session>&
    get()
    {
        return m_session;
    }

private:
    std::unique_ptr<lt::session> m_session;

    std::thread m_session_thread;

    std::atomic<bool> m_session_thread_quit;

    std::forward_list<Alert_Listener*> m_listeners;

    std::mutex m_listeners_mtx;
};

#endif
