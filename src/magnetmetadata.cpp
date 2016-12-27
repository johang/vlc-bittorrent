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
#include "playlist.h"

#define D(x)

using namespace libtorrent;

struct demux_sys_t {
	char *magnet;
};

static int
MagnetMetadataDemux(demux_t *p_demux);

static int
MagnetMetadataControl(demux_t *demux, int query, va_list args);

int
MagnetMetadataOpen(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	demux_t *p_demux = (demux_t *) p_this;

	std::string access(p_demux->psz_access ?: "");
	std::string location(p_demux->psz_location ?: "");
	std::string demux(p_demux->psz_demux ?: "");
	std::string file(p_demux->psz_file ?: "");

	demux_sys_t *p_sys = (demux_sys_t *) malloc(sizeof (struct demux_sys_t));

	size_t indexf = file.rfind("magnet:?");
	size_t indexl = location.rfind("magnet:?");

	if (indexf != std::string::npos) {
		p_sys->magnet = decode_URI_duplicate(location.substr(indexf).c_str());
	} else {
		if (indexl != std::string::npos) {
			p_sys->magnet = strdup(file.substr(indexl).c_str());
		} else {
			goto err;
		}
	}

	msg_Info(p_demux, "Magnet is %s", p_sys->magnet);

	p_demux->p_sys = p_sys;
	p_demux->pf_demux = MagnetMetadataDemux;
	p_demux->pf_control = MagnetMetadataControl;

	return VLC_SUCCESS;

err:
	free(p_sys);

	return VLC_EGENERIC;
}

void
MagnetMetadataClose(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	demux_t *p_demux = (demux_t *) p_this;

	free(p_demux->p_sys->magnet);
	free(p_demux->p_sys);
}

static int
MagnetMetadataDemux(demux_t *p_demux)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	msg_Info(p_demux, "Starting session");

	DownloadSession session;

	msg_Info(p_demux, "Adding download");

	std::string save_path;

	char *vlc_download_dir = config_GetUserDir(VLC_DOWNLOAD_DIR);

	save_path += vlc_download_dir;
	save_path += DIR_SEP;
	save_path += "vlc-bittorrent";

	free(vlc_download_dir);

	// Make sure directory exists. This assumes that the VLC_DOWNLOAD_DIR
	// already exists. Any error here is ignored. Save path error handling
	// is up to libtorrent to handle in a good way (for now.)
	vlc_mkdir(save_path.c_str(), 0777);

	add_torrent_params params;

	params.flags &= ~add_torrent_params::flag_auto_managed;
	params.flags &= ~add_torrent_params::flag_paused;
	params.save_path = save_path;

	error_code ec;

	parse_magnet_uri(p_demux->p_sys->magnet, params, ec);

	if (ec) {
		msg_Err(p_demux, "Failed to parse magnet");
		return -1;
	}

	msg_Info(p_demux, "magnet is %s", p_demux->p_sys->magnet);
	msg_Info(p_demux, "save_path is %s", save_path.c_str());

	// Add download and block until metadata is downloaded
	Download *download = session.add(params);

	if (!download) {
		msg_Err(p_demux, "Failed to add download");
		return -1;
	}

	msg_Info(p_demux, "Added download");

	std::string dump_path;

	char *vlc_cache_dir = config_GetUserDir(VLC_CACHE_DIR);

	dump_path += vlc_cache_dir;
	dump_path += DIR_SEP;
	dump_path += to_hex(params.info_hash.to_string());
	dump_path += ".torrent";

	free(vlc_cache_dir);

	msg_Info(p_demux, "dump_path is %s", dump_path.c_str());

	download->dump(dump_path);

	set_playlist(
		input_GetItem(p_demux->p_input),
		dump_path,
		download->name(),
		download->list());

	msg_Info(p_demux, "Stopping session");

	delete download;

	return 1;
}

static int
MagnetMetadataControl(demux_t *demux, int query, va_list args)
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
