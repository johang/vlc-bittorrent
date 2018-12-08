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
#include "libtorrent.h"
#include "vlc.h"

#include "download.h"
#include "metadata.h"
#include "magnetmetadata.h"
#include "data.h"

#define D(x)

struct access_sys_t {
	stream_t *stream;
};

static ssize_t
MagnetMetadataRead(stream_t *p_access, void *p_buffer, size_t i_len);

static int
MagnetMetadataControl(stream_t *access, int query, va_list args);

int
MagnetMetadataOpen(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	stream_t *p_access = (stream_t *) p_this;

	std::string name(p_access->psz_name ?: "");
	std::string filepath(p_access->psz_filepath ?: "");
	std::string location(p_access->psz_location ?: "");

	std::string magnet;

	if (name == "magnet") {
		magnet = "magnet:" + location;
	} else if (name == "file") {
		size_t index = filepath.rfind("magnet:?");

		if (index != std::string::npos) {
			magnet = filepath.substr(index);
		} else {
			return VLC_EGENERIC;
		}
	} else {
		return VLC_EGENERIC;
	}

	access_sys_t *p_sys = (access_sys_t *) malloc(sizeof (access_sys_t));

	Download d;

	try {
		// Parse metadata
		d.load(magnet, get_download_directory(p_this));

		msg_Dbg(p_access, "Added download");
	} catch(std::runtime_error& e) {
		msg_Err(p_access, "Failed to add download: %s", e.what());
		goto err;
	}

	try {
		// Wait for metadata and bencode it
		auto md = d.get_metadata();

		msg_Dbg(p_access, "Got metadata (%zu bytes)", md->size());

		p_sys->stream = vlc_stream_MemoryNew(
			p_this,
			(uint8_t *) memcpy(malloc(md->size()), md->data(), md->size()),
			md->size(),
			true);
	} catch (std::runtime_error& e) {
		msg_Err(p_access, "Failed to get metadata: %s", e.what());
		goto err;
	}

	p_access->p_sys = p_sys;
	p_access->pf_read = MagnetMetadataRead;
	p_access->pf_control = MagnetMetadataControl;

	return VLC_SUCCESS;

err:
	delete p_sys;

	return VLC_EGENERIC;
}

void
MagnetMetadataClose(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	stream_t *p_access = (stream_t *) p_this;

	if (!p_access->p_sys)
		return;

	access_sys_t *p_sys = (access_sys_t *) p_access->p_sys;

	if (p_sys->stream)
		vlc_stream_Delete(p_sys->stream);

	free(p_sys);
}

static ssize_t
MagnetMetadataRead(stream_t *p_access, void *p_buffer, size_t i_len)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	if (!p_access->p_sys)
		return -1;

	access_sys_t *p_sys = (access_sys_t *) p_access->p_sys;

	if (!p_sys->stream)
		return -1;

	ssize_t x = vlc_stream_Read(p_sys->stream, p_buffer, i_len);

	return x;
}

static int
MagnetMetadataControl(stream_t *access, int query, va_list args)
{
	D(printf("%s:%d: %s(0x%x)\n", __FILE__, __LINE__, __func__, query));

	switch (query) {
	case STREAM_GET_PTS_DELAY:
		*va_arg(args, int64_t *) = DEFAULT_PTS_DELAY;
		break;
	case STREAM_CAN_SEEK:
		*va_arg(args, bool *) = false;
		break;
	case STREAM_CAN_PAUSE:
		*va_arg(args, bool *) = false;
		break;
	case STREAM_CAN_CONTROL_PACE:
		*va_arg(args, bool *) = true;
		break;
	case STREAM_GET_CONTENT_TYPE:
		*va_arg(args, char **) = strdup("application/x-bittorrent");
		break;
	default:
		return VLC_EGENERIC;
	}

	return VLC_SUCCESS;
}
