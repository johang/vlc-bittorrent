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
#include <atomic>

#include "libtorrent.h"
#include "vlc.h"

#include "metadata.h"
#include "download.h"
#include "playlist.h"

#define D(x)

#define STREAM_BLOCK_MAX_SIZE (32 * 1024)
#define STREAM_METADATA_MAX_SIZE (1 * 1024 * 1024)

using namespace libtorrent;

struct demux_sys_t {
	DownloadSession *session;

	demux_sys_t(DownloadSession *s) : session(s)
	{
	}

	~demux_sys_t()
	{
		delete session;
	}
};

static int
MetadataDemux(demux_t *p_demux);

static int
MetadataControl(demux_t *demux, int query, va_list args);

int
MetadataOpen(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	demux_t *p_demux = (demux_t *) p_this;

	std::string location(p_demux->psz_location ?: "");
	std::string access(p_demux->psz_access ?: "");

	if (!(access == "file" || access == "http" || access == "https"))
		return VLC_EGENERIC;

	if (location.find(".torrent") == std::string::npos)
		return VLC_EGENERIC;

	// TODO: check content-type also

	p_demux->p_sys = new demux_sys_t(new DownloadSession());
	p_demux->pf_demux = MetadataDemux;
	p_demux->pf_control = MetadataControl;

	return VLC_SUCCESS;
}

void
MetadataClose(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	demux_t *p_demux = (demux_t *) p_this;

	delete p_demux->p_sys;
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

	// TODO: just use torrent_info to parse (no need for a session)
	Download *download = p_demux->p_sys->session->add(buf, buf_sz, false);

	if (!download) {
		msg_Err(p_demux, "Failed to parse");
		return -1;
	}

	// TODO: make better path name
	std::string path = "/tmp/vlc.torrent";

	download->dump(path);

	set_playlist(
		input_GetItem(p_demux->p_input),
		path,
		download->name(),
		download->list());

	delete download;

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
