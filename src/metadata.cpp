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

#include "config.h"

#include <string>
#include <fstream>
#include <vector>

#include "libtorrent.h"
#include "vlc.h"

#include "download.h"
#include "metadata.h"

#define D(x)

#define STREAM_BLOCK_MAX_SIZE (1 * 1024)
#define STREAM_METADATA_MAX_SIZE (1 * 1024 * 1024)

static int
MetadataDemux(demux_t *p_demux);

static int
MetadataReadDir(stream_t *p_demux, input_item_node_t *p_subitems);

int
MetadataOpen(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	stream_t *p_stream = (stream_t *) p_this;

	p_stream->pf_readdir = MetadataReadDir;
	p_stream->pf_control = access_vaDirectoryControlHelper;

	bool match = false;

	if (stream_HasExtension(p_stream->p_source, ".torrent"))
		match = true;

	if (stream_IsMimeType(p_stream->p_source, "application/x-bittorrent"))
		match = true;

	const uint8_t *data = NULL;

	// Attempt to read 1 byte of the metadata
	ssize_t len = vlc_stream_Peek(p_stream->p_source, &data, 1);

	// All bittorrent metadata files starts with a 'd'
	if (len < 1 || memcmp(data, "d", 1))
		return VLC_EGENERIC;

	return match ? VLC_SUCCESS : VLC_EGENERIC;
}

static bool
read_stream(stream_t *p_stream, char **buf, size_t *buf_sz)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	*buf_sz = 0;
	*buf = NULL;

	p_stream = p_stream->p_source;

	while (!vlc_stream_Eof(p_stream)) {
		block_t *b = vlc_stream_Block(p_stream, STREAM_BLOCK_MAX_SIZE);

		// That function can return NULL spuriously
		if (!b)
			continue;

		*buf_sz = *buf_sz + b->i_buffer;
		*buf = (char *) realloc(*buf, *buf_sz);

		// Enlarge the buffer and copy new contents to it
		memcpy(*buf + *buf_sz - b->i_buffer, b->p_buffer, b->i_buffer);

		block_Release(b);
	}

	return *buf_sz > 0;
}

static bool
has_extension(std::string file, std::string ext)
{
	auto filei = file.crbegin();
	auto exti = ext.crbegin();

	while (filei != file.crend() && exti != ext.crend()) {
		if (*filei != *exti)
			return false;

		filei++;
		exti++;
	}

	if (filei == file.crend() || exti != ext.crend())
		return false;

	return *filei == '.';
}

static void
build_playlist(stream_t *p_demux, input_item_node_t *p_subitems, Download& d)
{
	std::string path = get_cache_directory((vlc_object_t *) p_demux) +
		DIR_SEP + PACKAGE_NAME + "-" + d.get_infohash() + ".torrent";

	// Stream to output metadata to
	std::ofstream out(path, std::ios_base::binary);

	// The metadata in bencoded form
	std::shared_ptr<std::vector<char> > md = d.get_metadata();

	// Dump metadata to file
	std::copy(md->begin(), md->end(), std::ostreambuf_iterator<char>(out));

	input_item_t *p_current_input = input_GetItem(p_demux->p_input);

	// How many characters to remove from the beginning of each title
	size_t offset = (d.get_files().size() > 1) ? d.get_name().length() : 0;

	// Valid file extensions
	std::vector<std::string> video_ext{ EXTENSIONS_VIDEO_CSV };
	std::vector<std::string> audio_ext{ EXTENSIONS_AUDIO_CSV };
	std::vector<std::string> image_ext{ EXTENSIONS_IMAGE_CSV };

	std::vector<std::string> ext;

	if (get_add_video_files((vlc_object_t *) p_demux))
		ext.insert(ext.end(), video_ext.begin(), video_ext.end());

	if (get_add_audio_files((vlc_object_t *) p_demux))
		ext.insert(ext.end(), audio_ext.begin(), audio_ext.end());

	if (get_add_image_files((vlc_object_t *) p_demux))
		ext.insert(ext.end(), image_ext.begin(), image_ext.end());

	for (auto f : d.get_files()) {
		bool add = false;

		for (auto e : ext) {
			if (has_extension(f.first, e))
				add = true;
		}

		if (!add)
			continue;

		std::string mrl = "bittorrent://" + path + "?" + f.first;

		std::string title(f.first.substr(offset));

		input_item_t *p_input = input_item_New(mrl.c_str(), title.c_str());

		input_item_CopyOptions(p_input, p_current_input);
		input_item_node_AppendItem(p_subitems, p_input);
		input_item_Release(p_input);
	}
}

int
MetadataReadDir(stream_t *p_demux, input_item_node_t *p_subitems)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	char *buf = NULL;
	size_t buf_sz = 0;

	if (!read_stream(p_demux, &buf, &buf_sz)) {
		msg_Err(p_demux, "Stream was empty");
		return -1;
	}

	Download d;

	try {
		// Parse metadata
		d.load(buf, buf_sz, get_download_directory((vlc_object_t *) p_demux));

		// Build playlist
		build_playlist(p_demux, p_subitems, d);
	} catch (std::runtime_error& e) {
		msg_Err(p_demux, "Failed to parse metadata: %s", e.what());
		return -1;
	}

	free(buf);

	return 0;
}
