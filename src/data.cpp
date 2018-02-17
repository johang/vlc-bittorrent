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
#include <algorithm>

#include "libtorrent.h"
#include "vlc.h"

#include "data.h"
#include "download.h"

#define MIN_CACHING_TIME (10000)

#define D(x)

using namespace libtorrent;

struct access_sys_t {
	DownloadSession *session;

	Download *download;

	/**
	 * Index of the opened file
	 */
	int index;
};

static int
DataSeek(access_t *p_access, uint64_t i_pos);

static ssize_t
DataRead(access_t *p_access, uint8_t *p_buffer, size_t i_len);

static int
DataControl(access_t *p_access, int i_query, va_list args);

static int
lookup(std::vector<std::string> files, std::string file)
{
	std::vector<std::string>::iterator it = std::find(files.begin(),
		files.end(), file);

	if (it == files.end())
		return -1;
	else
		return (int) (it - files.begin());
}

int
DataOpen(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	access_t *p_access = (access_t *) p_this;

	std::string access(p_access->psz_access);
	std::string location(p_access->psz_location);

	if (access != "bittorrent")
		return VLC_EGENERIC;

	size_t query = location.find("?");

	if (query == std::string::npos) {
		msg_Err(p_access, "Failed to find file");
		return VLC_EGENERIC;
	}

	std::string file(location.substr(query + 1));
	std::string metadata("file:///" + location.substr(0, query));

	access_sys_t *sys = (access_sys_t *) malloc(sizeof (struct access_sys_t));

	msg_Dbg(p_access, "Adding download");

	sys->session = new DownloadSession();

	std::string save_path;

	char *vlc_download_dir = config_GetUserDir(VLC_DOWNLOAD_DIR);

	save_path += vlc_download_dir;
	save_path += DIR_SEP;
	save_path += "vlc-bittorrent";

	vlc_mkdir(vlc_download_dir, 0777);
	vlc_mkdir(save_path.c_str(), 0777);

	free(vlc_download_dir);

	add_torrent_params params;

	params.flags &= ~add_torrent_params::flag_auto_managed;
	params.flags &= ~add_torrent_params::flag_paused;
	params.save_path = save_path;
	params.url = metadata;

	sys->download = sys->session->add(params);

	if (!sys->download) {
		msg_Err(p_access, "Add download");
		goto err;
	}

	sys->index = lookup(sys->download->list(), file);

	if (sys->index < 0) {
		msg_Err(p_access, "Failed to lookup file");
		goto err;
	}

	msg_Dbg(p_access, "Opening %s in %s", file.c_str(), metadata.c_str());

	p_access->p_sys = sys;
	p_access->pf_block = NULL;
	p_access->pf_seek = DataSeek;
	p_access->pf_read = DataRead;
	p_access->pf_control = DataControl;

	return VLC_SUCCESS;

err:
	delete sys->download;
	delete sys->session;

	free(sys);

	return VLC_EGENERIC;
}

void
DataClose(vlc_object_t *p_this)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	access_t *p_access = (access_t *) p_this;

	if (!p_access)
		return;

	delete p_access->p_sys->download;
	delete p_access->p_sys->session;

	free(p_access->p_sys);
}

static int
DataSeek(access_t *p_access, uint64_t i_pos)
{
	D(printf("%s:%d: %s(%lu)\n", __FILE__, __LINE__, __func__, i_pos));

	if (!p_access)
		return VLC_EGENERIC;

	p_access->info.i_pos = i_pos;
	p_access->info.b_eof = false;

	return VLC_SUCCESS;
}

static ssize_t
DataRead(access_t *p_access, uint8_t *p_buffer, size_t i_len)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	if (!p_access)
		return -1;

	if (!p_access->p_sys)
		return -1;

	if (!p_access->p_sys->download)
		return -1;

	ssize_t size = p_access->p_sys->download->read(
		p_access->p_sys->index,
		p_access->info.i_pos,
		(char *) p_buffer,
		i_len);

	if (size > 0)
		p_access->info.i_pos += (uint64_t) size;
	else
		p_access->info.b_eof = true;

	return size;
}

static int
DataControl(access_t *p_access, int i_query, va_list args)
{
	D(printf("%s:%d: %s()\n", __FILE__, __LINE__, __func__));

	switch (i_query) {
	case ACCESS_CAN_SEEK:
		*va_arg(args, bool *) = true;
		break;
	case ACCESS_CAN_FASTSEEK:
		*va_arg(args, bool *) = true;
		break;
	case ACCESS_CAN_PAUSE:
		*va_arg(args, bool *) = true;
		break;
	case ACCESS_CAN_CONTROL_PACE:
		*va_arg(args, bool *) = true;
		break;
	case ACCESS_GET_PTS_DELAY:
		*va_arg(args, int64_t *) = INT64_C(1000) * __MAX(
			MIN_CACHING_TIME,
			var_InheritInteger(p_access, "network-caching"));
		break;
	case ACCESS_SET_PAUSE_STATE:
		break;
	// TODO
	case ACCESS_GET_SIZE:
	case ACCESS_SET_SEEKPOINT:
	case ACCESS_GET_SEEKPOINT:
	default:
		return VLC_EGENERIC;
	}

	return VLC_SUCCESS;
}
