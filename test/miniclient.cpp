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

#include <chrono> // for std::chrony
#include <iostream>
#include <utility> // for std::move

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/alert.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/version.hpp>
#include <libtorrent/add_torrent_params.hpp>
#pragma GCC diagnostic pop

namespace lt = libtorrent;

#define ALERTS (lt::alert::status_notification | \
                lt::alert::progress_notification | \
                lt::alert::error_notification | \
                lt::alert::peer_notification)

int
main(int argc, char const* argv[]) {
	try {
		lt::settings_pack p;
		p.set_int(lt::settings_pack::alert_mask, ALERTS);
		p.set_bool(lt::settings_pack::enable_lsd, true);
		p.set_bool(lt::settings_pack::enable_upnp, false);
		p.set_bool(lt::settings_pack::enable_natpmp, false);
		p.set_bool(lt::settings_pack::enable_dht, false);
		p.set_bool(lt::settings_pack::broadcast_lsd, true);

		lt::session ses(p);

		for (int i = 1; i < argc; i++) {
			try {
				lt::add_torrent_params atp;
				atp.save_path = ".";
#if LIBTORRENT_VERSION_NUM < 10100
				atp.ti = new libtorrent::torrent_info(argv[i]);
#elif LIBTORRENT_VERSION_NUM < 10200
				atp.ti = boost::make_shared<libtorrent::torrent_info>(argv[i]);
#else
				atp.ti = std::make_shared<libtorrent::torrent_info>(argv[i]);
#endif

				lt::torrent_handle h = ses.add_torrent(std::move(atp));
			} catch(const std::exception& e) {
				std::cerr << argv[0] << ": Failed to add " << argv[i] <<
					": " << e.what() << std::endl;
			}
		}

		for (;;) {
			std::vector<lt::alert*> alerts;

			ses.wait_for_alert(std::chrono::seconds(1));
			ses.pop_alerts(&alerts);

			for (const lt::alert *a : alerts) {
				std::cout << a->message() << std::endl;
			}
		}
	} catch (const std::exception& e) {
		std::cerr << argv[0] << ": Error: " << e.what() << std::endl;
	}
}
