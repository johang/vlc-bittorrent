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

#include <fstream>
#include <string>
#include <vector>

#include "libtorrent.h"
#include "vlc.h"

#define D(x)

void
set_playlist(input_item_t *p_input_item, std::string path, std::string name,
		std::vector<std::string> files)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	// How many characters to remove from the beginning of each title
	size_t offset = (files.size() > 1) ? name.length() : 0;

	input_item_node_t *p_subitems = input_item_node_Create(p_input_item);

	for (std::vector<std::string>::iterator i = files.begin();
			i != files.end(); ++i) {
		std::string item_path;

		item_path += "bittorrent://";
		item_path += path;
		item_path += "?";
		item_path += *i;

		std::string item_title((*i).substr(offset));

		// Create an item for each file
		input_item_t *p_input = input_item_New(
			item_path.c_str(),
			item_title.c_str());

		// Add the item to the playlist
		input_item_node_AppendItem(p_subitems, p_input);

		vlc_gc_decref(p_input);
	}

	input_item_node_PostAndDelete(p_subitems);
}
