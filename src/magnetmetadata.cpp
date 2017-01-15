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

#include "download.h"
#include "metadata.h"
#include "magnetmetadata.h"
#include "data.h"

#define D(x)

using namespace libtorrent;

struct access_sys_t {
	stream_t *stream;
};

static ssize_t
MagnetMetadataRead(access_t *p_access, uint8_t *p_buffer, size_t i_len);

static int
MagnetMetadataControl(access_t *access, int query, va_list args);

int
MagnetMetadataOpen(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	access_t *p_access = (access_t *) p_this;

	std::string access(p_access->psz_access ?: "");
	std::string demux(p_access->psz_demux ?: "");
	std::string filepath(p_access->psz_filepath ?: "");
	std::string location(p_access->psz_location ?: "");

	std::string magnet;

	if (access == "magnet" && demux == "any") {
		magnet = "magnet:" + location;
	} else if (access == "file" && demux == "any") {
		size_t index = filepath.rfind("magnet:?");

		if (index != std::string::npos) {
			magnet = filepath.substr(index);
		} else {
			return VLC_EGENERIC;
		}
	} else {
		return VLC_EGENERIC;
	}

	error_code ec;

	add_torrent_params params;

	parse_magnet_uri(magnet, params, ec);

	if (ec) {
		msg_Err(p_access, "Found magnet URI but couldn't parse it");
		return VLC_EGENERIC;
	}

	std::string save_path;

	char *vlc_download_dir = config_GetUserDir(VLC_DOWNLOAD_DIR);

	save_path += vlc_download_dir;
	save_path += DIR_SEP;
	save_path += "vlc-bittorrent";

	free(vlc_download_dir);

	vlc_mkdir(vlc_download_dir, 0777);
	vlc_mkdir(save_path.c_str(), 0777);

	params.flags &= ~add_torrent_params::flag_auto_managed;
	params.flags &= ~add_torrent_params::flag_paused;
	params.save_path = save_path;

	DownloadSession session;

	msg_Dbg(p_access, "Adding download");

	// Add download and block until metadata is downloaded
	Download *download = session.add(params);

	if (!download) {
		msg_Err(p_access, "Failed to add download");
		return VLC_EGENERIC;
	}

	msg_Dbg(p_access, "Added download");

	std::vector<char> md = download->get_metadata();

	msg_Dbg(p_access, "Got metadata (%zu bytes)", md.size());

	delete download;

	access_sys_t *p_sys = (access_sys_t *) malloc(sizeof (access_sys_t));

	p_sys->stream = stream_MemoryNew(
		p_this,
		(uint8_t *) memcpy(
			malloc(md.size()),
			md.data(),
			md.size()),
		md.size(),
		true);

	p_access->p_sys = p_sys;
	p_access->pf_read = MagnetMetadataRead;
	p_access->pf_control = MagnetMetadataControl;

	return VLC_SUCCESS;
}

void
MagnetMetadataClose(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	access_t *p_access = (access_t *) p_this;

	if (!p_access->p_sys)
		return;

	if (!p_access->p_sys->stream)
		return;

    stream_Delete(p_access->p_sys->stream);

	free(p_access->p_sys);
}

static ssize_t
MagnetMetadataRead(access_t *p_access, uint8_t *p_buffer, size_t i_len)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	if (!p_access->p_sys)
		return -1;

	if (!p_access->p_sys->stream)
		return -1;

	return stream_Read(p_access->p_sys->stream, p_buffer, (int) i_len);
}

static int
MagnetMetadataControl(access_t *access, int query, va_list args)
{
	D(printf("%s:%d: %s(0x%x)\n", __FILE__, __LINE__, __func__, query));

	switch (query) {
	case ACCESS_GET_PTS_DELAY:
		*va_arg(args, int64_t *) = DEFAULT_PTS_DELAY;
		break;
	case ACCESS_CAN_SEEK:
		*va_arg(args, bool *) = false;
		break;
	case ACCESS_CAN_PAUSE:
		*va_arg(args, bool *) = false;
		break;
	case ACCESS_CAN_CONTROL_PACE:
		*va_arg(args, bool *) = true;
		break;
	case ACCESS_GET_CONTENT_TYPE:
		*va_arg(args, char **) = strdup("application/x-bittorrent");
		break;
	default:
		return VLC_EGENERIC;
	}

	return VLC_SUCCESS;
}
