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
#include <algorithm>

#include "libtorrent.h"
#include "vlc.h"

#include "data.h"
#include "download.h"

#define MIN_CACHING_TIME (10000)

#define D(x)

struct access_sys_t {
	Download *download;

	/**
	 * Index of the opened file
	 */
	int index;

	/**
	 * Current position within the file.
	 */
	uint64_t i_pos;
};

static int
DataSeek(stream_t *p_access, uint64_t i_pos);

static ssize_t
DataRead(stream_t *p_access, void *p_buffer, size_t i_len);

static int
DataControl(stream_t *p_access, int i_query, va_list args);

int
DataOpen(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	stream_t *p_access = (stream_t *) p_this;

	std::string location(p_access->psz_location);

	size_t query = location.find("?");

	if (query == std::string::npos) {
		msg_Err(p_access, "Failed to find file");
		return VLC_EGENERIC;
	}

	std::string file(location.substr(query + 1));
	std::string metadata("file://" + location.substr(0, query));

	msg_Dbg(p_access, "Opening %s in %s", file.c_str(), metadata.c_str());

	access_sys_t *sys = (access_sys_t *) malloc(sizeof (access_sys_t));

	sys->download = new Download(get_keep_files(p_this));

	try {
		// Parse metadata
		sys->download->load(metadata, get_download_directory(p_this));

		msg_Dbg(p_access, "Added download");
	} catch(std::runtime_error& e) {
		msg_Err(p_access, "Failed to add download: %s", e.what());
		goto err;
	}

	try {
		// Wait for metadata and lookup file
		sys->index = sys->download->get_file_index_by_path(file);

		// Assume start from beginning of file
		sys->i_pos = 0;

		msg_Dbg(p_access, "Found file (index %d)", sys->index);
	} catch (std::runtime_error& e) {
		msg_Err(p_access, "Failed find file: %s", e.what());
		goto err;
	}

	p_access->p_sys = sys;
	p_access->pf_block = NULL;
	p_access->pf_seek = DataSeek;
	p_access->pf_read = DataRead;
	p_access->pf_control = DataControl;

	return VLC_SUCCESS;

err:
	delete sys->download;

	free(sys);

	return VLC_EGENERIC;
}

void
DataClose(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	stream_t *p_access = (stream_t *) p_this;

	if (!p_access)
		return;

	access_sys_t *p_sys = (access_sys_t *) p_access->p_sys;

	delete p_sys->download;

	free(p_sys);
}

static int
DataSeek(stream_t *p_access, uint64_t i_pos)
{
	D(printf("%s:%d: %s(%lu)\n", __FILE__, __LINE__, __func__, i_pos));

	if (!p_access)
		return VLC_EGENERIC;

	access_sys_t *p_sys = (access_sys_t *) p_access->p_sys;

	p_sys->i_pos = i_pos;

	return VLC_SUCCESS;
}

static ssize_t
DataRead(stream_t *p_access, void *p_buffer, size_t i_len)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	if (!p_access)
		return -1;

	access_sys_t *p_sys = (access_sys_t *) p_access->p_sys;

	if (!p_sys)
		return -1;

	if (!p_sys->download)
		return -1;

	try {
		ssize_t size = p_sys->download->read(
			p_sys->index,
			p_sys->i_pos,
			(char *) p_buffer,
			i_len);

		if (size > 0)
			p_sys->i_pos += (uint64_t) size;
		else if (size < 0)
			return 0;

		return size;
	} catch (std::runtime_error& e) {
		msg_Dbg(p_access, "Read failed: %s", e.what());
	}

	return -1;
}

static int
DataControl(stream_t *p_access, int i_query, va_list args)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	if (!p_access)
		return VLC_EGENERIC;

	access_sys_t *p_sys = (access_sys_t *) p_access->p_sys;

	if (!p_sys)
		return VLC_EGENERIC;

	if (!p_sys->download)
		return VLC_EGENERIC;

	switch (i_query) {
	case STREAM_CAN_SEEK:
		*va_arg(args, bool *) = true;
		break;
	case STREAM_CAN_FASTSEEK:
		*va_arg(args, bool *) = true;
		break;
	case STREAM_CAN_PAUSE:
		*va_arg(args, bool *) = true;
		break;
	case STREAM_CAN_CONTROL_PACE:
		*va_arg(args, bool *) = true;
		break;
	case STREAM_GET_PTS_DELAY:
		*va_arg(args, int64_t *) =
			INT64_C(1000) * __MAX(
				MIN_CACHING_TIME,
				var_InheritInteger(p_access, "network-caching"));
		break;
	case STREAM_SET_PAUSE_STATE:
		break;
	case STREAM_GET_SIZE:
		*va_arg(args, uint64_t *) =
			p_sys->download->get_file_size_by_index(p_sys->index);
		break;
	default:
		return VLC_EGENERIC;
	}

	return VLC_SUCCESS;
}
