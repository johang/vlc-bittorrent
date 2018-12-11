/*
Copyright 2018 Johan Gunnarsson <johan.gunnarsson@gmail.com>

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
#include <stdexcept>

#include "vlc.h"

std::string
get_download_directory(vlc_object_t *p_this)
{
	char *dir = var_InheritString(p_this, "bittorrent-download-path");

	if (!dir)
		dir = config_GetUserDir(VLC_DOWNLOAD_DIR);

	if (vlc_mkdir(dir, 0777)) {
		if (errno != EEXIST)
			throw std::runtime_error(
				std::string("") +
				"Failed to create download directory (" + dir + "): " +
				strerror(errno));
	}

	std::string path;

	path += dir;
	path += DIR_SEP;
	path += PACKAGE;

	free(dir);

	if (vlc_mkdir(path.c_str(), 0777)) {
		if (errno != EEXIST)
			throw std::runtime_error(
				std::string("") +
				"Failed to create download directory (" + path + "): " +
				strerror(errno));
	}

	return path;
}

std::string
get_cache_directory(vlc_object_t *p_this)
{
	char *dir = config_GetUserDir(VLC_CACHE_DIR);

	if (vlc_mkdir(dir, 0777)) {
		if (errno != EEXIST)
			throw std::runtime_error(
				std::string("") +
				"Failed to create cache directory (" + dir + "): " +
				strerror(errno));
	}

	std::string path(dir);

	free(dir);

	return path;
}

bool
get_add_video_files(vlc_object_t *p_this)
{
	return var_InheritBool(p_this, "bittorrent-add-video-files");
}

bool
get_add_audio_files(vlc_object_t *p_this)
{
	return var_InheritBool(p_this, "bittorrent-add-audio-files");
}

bool
get_add_image_files(vlc_object_t *p_this)
{
	return var_InheritBool(p_this, "bittorrent-add-image-files");
}
