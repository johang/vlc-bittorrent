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

#include <iostream>
#include <string>

#include "download.h"

static bool show_metadata = false;
static bool show_read = false;
static bool abort_read = false;
static bool abort_metadata = false;

static void
test_metadata(Download& d)
{
	std::cout << "DOWNLOADDUMMY NAME " << d.get_name() << std::endl;
	std::cout << "DOWNLOADDUMMY INFOHASH " << d.get_infohash() << std::endl;

	for (std::pair<std::string,uint64_t>& f : d.get_files()) {
		std::cout << "DOWNLOADDUMMY FILE " << f.first << " " << f.second
			<< std::endl;
	}
}

static void
test_read(Download& d)
{
	int i = 0;

	for (std::pair<std::string,uint64_t>& f : d.get_files()) {
		uint64_t total = 0;

		while (1) {
			char buf[1024];
			ssize_t r = d.read(i, total, buf, sizeof (buf));

			if (r <= 0)
				break;

			total += (uint64_t) r;
		}

		std::cout << "DOWNLOADDUMMY READ " << total << " " << i << std::endl;

		// File index
		i++;
	}
}

int
main(int argc, char *argv[])
{
	for (int i = 0; i < argc; i++) {
		std::string arg = argv[i];

		if (arg == "--show-metadata") {
			show_metadata = true;
		} else if (arg == "--show-read") {
			show_read = true;
		} else if (arg == "--abort-metadata") {
			abort_metadata = true;
		} else if (arg == "--abort-read") {
			abort_read = true;
		}
	}

	if (argc <= 1)
		return -1;

	Download d;

	try {
		d.load(argv[1], "/tmp/vlc-bittorrent");

		if (show_metadata) {
			test_metadata(d);
		}

		if (show_read) {
			test_read(d);
		}
	} catch (std::runtime_error& e) {
		std::cout << "DOWNLOADDUMMY FAIL " << e.what() << std::endl;
	}

	std::cout << "DOWNLOADDUMMY END" << std::endl;

	return 0;
}
