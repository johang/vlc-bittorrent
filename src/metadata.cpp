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

#include <string>
#include <fstream>
#include <vector>

#include "libtorrent.h"
#include "vlc.h"

#include "metadata.h"

#define D(x)

#define STREAM_BLOCK_MAX_SIZE (32 * 1024)
#define STREAM_METADATA_MAX_SIZE (1 * 1024 * 1024)

using namespace libtorrent;

static int
MetadataDemux(demux_t *p_demux);

static int
MetadataControl(demux_t *demux, int query, va_list args);

int
MetadataOpen(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	demux_t *p_demux = (demux_t *) p_this;

	bool match = false;

	if (demux_IsPathExtension(p_demux, ".torrent"))
		match = true;

	char *type = stream_ContentType(p_demux->s);

	if (type && !strcmp(type, "application/x-bittorrent"))
		match = true;

	free(type);

	if (!match)
		return VLC_EGENERIC;

	p_demux->pf_demux = MetadataDemux;
	p_demux->pf_control = MetadataControl;

	return VLC_SUCCESS;
}

void
MetadataClose(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));
}

static bool
read_stream(stream_t *p_stream, char **buf, size_t *buf_sz)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	*buf_sz = 0;
	*buf = NULL;

	block_t *block = stream_Block(p_stream, STREAM_BLOCK_MAX_SIZE);

	for (; block && *buf_sz < STREAM_METADATA_MAX_SIZE;
			block = stream_Block(p_stream, STREAM_BLOCK_MAX_SIZE)) {
		D(printf("%s:%d: %s(): read %lu bytes\n", __FILE__, __LINE__,
			__func__, block->i_buffer));

		*buf_sz = *buf_sz + block->i_buffer;
		*buf = (char *) realloc(*buf, *buf_sz);

		// Enlarge the buffer and copy new contents to it
		memcpy(
			*buf + *buf_sz - block->i_buffer,
			block->p_buffer,
			block->i_buffer);

		block_Release(block);
	}

	return *buf_sz > 0;
}

static void
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

static int
MetadataDemux(demux_t *p_demux)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	char *buf = NULL;
	size_t buf_sz = 0;

	if (!read_stream(p_demux->s, &buf, &buf_sz)) {
		msg_Err(p_demux, "Stream was empty");
		return -1;
	}

	error_code ec;

	torrent_info metadata(buf, (int) buf_sz, ec, 0);

	if (ec) {
		msg_Err(p_demux, "Failed to parse");
		return -1;
	}

	std::string path;

	char *vlc_cache_dir = config_GetUserDir(VLC_CACHE_DIR);

	path += vlc_cache_dir;
	path += DIR_SEP;
	path += to_hex(metadata.info_hash().to_string());
	path += ".torrent";

	free(vlc_cache_dir);

	create_torrent t(metadata);

	t.set_comment("vlc metadata dump");
	t.set_creator("vlc");

	// Stream to output metadata to
	std::ofstream out(path, std::ios_base::binary);

	// Bencode metadata and dump it to file
	bencode(std::ostream_iterator<char>(out), t.generate());

	std::vector<std::string> files;

	for (int i = 0; i < metadata.num_files(); i++) {
		files.push_back(metadata.file_at(i).path);
	}

	set_playlist(
		input_GetItem(p_demux->p_input),
		path,
		metadata.name(),
		files);

	return 0;
}

static int
MetadataControl(demux_t *demux, int query, va_list args)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	switch (query) {
	case DEMUX_GET_PTS_DELAY:
		*va_arg(args, int64_t *) = DEFAULT_PTS_DELAY;
		break;
	case DEMUX_CAN_SEEK:
		*va_arg(args, bool *) = false;
		break;
	case DEMUX_CAN_PAUSE:
		*va_arg(args, bool *) = false;
		break;
	case DEMUX_CAN_CONTROL_PACE:
		*va_arg(args, bool *) = false;
		break;
	default:
		return VLC_EGENERIC;
	}

	return VLC_SUCCESS;
}
