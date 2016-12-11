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
	add_torrent_params params;

	demux_sys_t(add_torrent_params p) : params(p)
	{
	}
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

	error_code ec;

	add_torrent_params params;

	params.flags &= ~libtorrent::add_torrent_params::flag_auto_managed;
	params.flags &= ~libtorrent::add_torrent_params::flag_paused;
	// TODO: better save path
	params.save_path = "/tmp";

	if (access == "magnet") {
		parse_magnet_uri(location, params, ec);
	} else if (access == "file") {
		size_t index = file.rfind("magnet:?");

		if (index == std::string::npos)
			return VLC_EGENERIC;

		parse_magnet_uri(file.substr(index), params, ec);
	} else {
		return VLC_EGENERIC;
	}

	if (ec) {
		msg_Err(p_demux, "Invalid magnet: %s", ec.message().c_str());
		return VLC_EGENERIC;
	}

	p_demux->p_sys = new demux_sys_t(params);
	p_demux->pf_demux = MagnetMetadataDemux;
	p_demux->pf_control = MagnetMetadataControl;

	return VLC_SUCCESS;
}

void
MagnetMetadataClose(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));
}

static int
MagnetMetadataDemux(demux_t *p_demux)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	msg_Info(p_demux, "Starting session");

	DownloadSession session;

	msg_Info(p_demux, "Adding download");

	// Add download and block until metadata is downloaded
	Download *download = session.add(p_demux->p_sys->params);

	if (!download) {
		msg_Err(p_demux, "Failed to add download");
		return -1;
	}

	msg_Info(p_demux, "Added download");

	// TODO: better path name
	std::string path = "/tmp/vlc.torrent";

	download->dump(path);

	set_playlist(
		input_GetItem(p_demux->p_input),
		path,
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
