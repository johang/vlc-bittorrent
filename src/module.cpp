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

#include "libtorrent.h"
#include "vlc.h"

#include "metadata.h"
#include "magnetmetadata.h"
#include "data.h"

vlc_module_begin()
	set_shortname("bittorrent")
	set_category(CAT_INPUT)
	set_subcategory(SUBCAT_INPUT_ACCESS)
	/* This module takes a special URL in the form
	   bittorrent:///path/to/metadata.torrent?file.ext and downloads and
	   outputs the contents of the file on demand. */
	set_description("Bittorrent data access")
	set_capability("access", 1)
	add_shortcut("bittorrent")
	set_callbacks(DataOpen, DataClose)
	add_directory("bittorrent-download-path", NULL, "Downloads",
		"Directory where VLC will put downloaded files.", false)
	add_bool("bittorrent-add-video-files", true, "Add video files",
		"Add video files to the playlist.", true)
	add_bool("bittorrent-add-audio-files", true, "Add audio files",
		"Add audio files to the playlist.", true)
	add_bool("bittorrent-add-image-files", false, "Add image files",
		"Add image files to the playlist.", true)
	add_bool("bittorrent-keep-files", false, "Don't delete files",
		"Don't delete files after download.", true)
	add_submodule()
		/* This module takes a metadata HTTP URL or a metadata file URI and
		   outputs a playlist containing special URLs in the form
		   bittorrent:///path/to/metadata.torrent?file.ext that the data
		   access submodule can handle. */
		set_description("Bittorrent file/HTTP/HTTPS metadata demux")
		set_capability("stream_filter", 50)
		set_callbacks(MetadataOpen, NULL)
	add_submodule()
		/* This module takes a metadata magnet URI and outputs a bittorrent
		   metadata that the metadata demux submodule can handle. */
		set_description("Bittorrent magnet metadata access")
		set_capability("access", 60)
		add_shortcut("file", "magnet")
		set_callbacks(MagnetMetadataOpen, MagnetMetadataClose)
vlc_module_end()
