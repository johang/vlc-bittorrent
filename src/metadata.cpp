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

#define STREAM_BLOCK_MAX_SIZE (1 * 1024)
#define STREAM_METADATA_MAX_SIZE (1 * 1024 * 1024)

using namespace libtorrent;

static int
MetadataDemux(demux_t *p_demux);

static int
MetadataReadDir(stream_t *p_demux, input_item_node_t *p_subitems);

static int ReadDVD( stream_t *, input_item_node_t * );
static int ReadDVD_VR( stream_t *, input_item_node_t * );

int MetadataOpen(vlc_object_t *p_this)
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

    const uint8_t *p_peek = NULL;
    ssize_t i_peek = vlc_stream_Peek(p_stream->p_source, &p_peek, 1);

	/* All bittorrent metadata files starts with a 'd' */
    if (i_peek < 1 || memcmp(p_peek, "d", 1)) {
        return VLC_EGENERIC;
	}

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
		block_t *block = vlc_stream_Block(p_stream, STREAM_BLOCK_MAX_SIZE);

		// That function can return NULL spuriously
		if (!block)
			continue;

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

static bool
build_playlist(stream_t *p_demux, input_item_node_t *p_subitems, char *buf,
		size_t buf_sz)
{
	error_code ec;

	torrent_info metadata(buf, (int) buf_sz, ec, 0);

	if (ec) {
		return false;
	}

	std::string path;

	char *vlc_cache_dir = config_GetUserDir(VLC_CACHE_DIR);

	path += vlc_cache_dir;
	path += DIR_SEP;
	//path += to_hex(metadata.info_hash().to_string());
	path += "metadata";
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

	input_item_t *p_current_input = input_GetItem(p_demux->p_input);

	// How many characters to remove from the beginning of each title
	size_t offset = (files.size() > 1) ? metadata.name().length() : 0;

	for (std::vector<std::string>::iterator i = files.begin();
			i != files.end(); ++i) {
		std::string item_path;

		item_path += "bittorrent://";
		item_path += path;
		item_path += "?";
		item_path += *i;

		std::cout << "XXXXXXXXXXXXXXXXXX" << item_path << std::endl;

		std::string item_title((*i).substr(offset));

		// Create an item for each file
		input_item_t *p_input = input_item_New(
			item_path.c_str(),
			item_title.c_str());

        input_item_CopyOptions(p_input, p_current_input);

		// Add the item to the playlist
		input_item_node_AppendItem(p_subitems, p_input);

		input_item_Release(p_input);
	}

	return true;
}

static int
MetadataReadDir(stream_t *p_demux, input_item_node_t *p_subitems)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	char *buf = NULL;
	size_t buf_sz = 0;

	if (!read_stream(p_demux, &buf, &buf_sz)) {
		msg_Err(p_demux, "Stream was empty");
		return -1;
	}

	if (!build_playlist(p_demux, p_subitems, buf, buf_sz)) {
		msg_Err(p_demux, "Failed to parse stream");
		return -1;
	}

	free(buf);

	return 0;
}
