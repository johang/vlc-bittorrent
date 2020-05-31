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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <pthread.h>

#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_list.h>
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_media_player.h>
#include <vlc/libvlc_media_list_player.h>

static bool play_item = false;
static bool play_subitems = false;
static bool print_item = false;
static bool print_subitems = false;
static bool random_seek = false;

static pthread_cond_t state_change_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t state_change_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *args[] = {
	"--ignore-config",
	"-I",
	"dummy",
	"--no-media-library",
	"--vout=dummy",
	"--aout=dummy",
	"--reset-plugins-cache",
	"--bittorrent-download-path",
	"/tmp",
	"--bittorrent-add-video-files",
	"--bittorrent-add-audio-files",
	"--bittorrent-add-image-files",
	"--bittorrent-keep-files",
	"--image-duration",
	"1",
};

static void
media_event_handler(const struct libvlc_event_t *, void *);

static void
media_player_event_handler(const struct libvlc_event_t *, void *);

static void
media_list_player_event_handler(const struct libvlc_event_t *, void *);

static void
media_subitemadded(libvlc_media_t *parent, libvlc_media_t *media)
{
	libvlc_event_manager_t *mem = libvlc_media_event_manager(media);
	assert(mem != NULL);

	libvlc_event_attach(mem, libvlc_MediaStateChanged, media_event_handler, NULL);

	if (print_subitems) {
		printf(
			"VLCDUMMY SUBITEM %s\n",
			libvlc_media_get_meta(media, libvlc_meta_Title));
	}
}

static void
media_statechanged(libvlc_media_t *media, libvlc_state_t new_state)
{
	libvlc_event_manager_t *mem = libvlc_media_event_manager(media);
	assert(mem != NULL);

	char *state = "";

	switch (new_state) {
	case libvlc_Opening:
		state = "OPENING";
		break;
	case libvlc_Playing:
		state = "PLAYING";
		break;
	case libvlc_Error:
		state = "ERROR";
		break;
	case libvlc_Ended:
		/* Unregister this event to avoid duplicate ENDED events */
		libvlc_event_detach(mem, libvlc_MediaStateChanged, media_event_handler, NULL);

		state = "ENDED";
		break;
	default:
		printf("state %d\n", new_state);
		return;
	}

	libvlc_media_stats_t stat;

	if (new_state == libvlc_Ended && libvlc_media_get_stats(media, &stat))
		printf(
			"VLCDUMMY STATE %s %s (%s audio frames, %s video frames)\n",
			libvlc_media_get_meta(media, libvlc_meta_Title),
			state,
			stat.i_decoded_audio > 0 ? ">0" : "0",
			stat.i_decoded_video > 0 ? ">0" : "0");
	else
		printf(
			"VLCDUMMY STATE %s %s\n",
			libvlc_media_get_meta(media, libvlc_meta_Title),
			state);
}

static void
media_event_handler(const struct libvlc_event_t *p_event, void *p_data)
{
	switch (p_event->type) {
	case libvlc_MediaSubItemAdded:
		media_subitemadded(
			(libvlc_media_t *) p_event->p_obj,
			p_event->u.media_subitem_added.new_child);
		break;
	case libvlc_MediaStateChanged:
		media_statechanged(
			(libvlc_media_t *) p_event->p_obj,
			(libvlc_state_t) p_event->u.media_state_changed.new_state);
		break;
	default:
		break;
	}
}

static void
media_player_event_handler(const struct libvlc_event_t *p_event, void *p_data)
{
	switch (p_event->type) {
	case libvlc_MediaPlayerOpening:
	case libvlc_MediaPlayerBuffering:
	case libvlc_MediaPlayerPlaying:
	case libvlc_MediaPlayerPaused:
	case libvlc_MediaPlayerStopped:
		pthread_cond_broadcast(&state_change_cond);
		break;
	default:
		break;
	}
}

static void
media_list_player_event_handler(const struct libvlc_event_t *p_event, void *p_data)
{
	switch (p_event->type) {
	case libvlc_MediaListPlayerPlayed:
	case libvlc_MediaListPlayerStopped:
		pthread_cond_broadcast(&state_change_cond);
		break;
	default:
		break;
	}
}

static void
play_media_list(libvlc_instance_t *vlc, libvlc_media_list_t *ml)
{
	if (libvlc_media_list_count(ml) <= 0)
		return;

	libvlc_media_list_player_t *mlp = libvlc_media_list_player_new(vlc);
	assert(mlp != NULL);

	libvlc_event_manager_t *mlpem = libvlc_media_list_player_event_manager(mlp);
	assert(mlpem != NULL);

	libvlc_event_attach(mlpem, libvlc_MediaListPlayerPlayed, media_list_player_event_handler, NULL);
	libvlc_event_attach(mlpem, libvlc_MediaListPlayerStopped, media_list_player_event_handler, NULL);

	libvlc_media_list_player_set_media_list(mlp, ml);

	/* Start playing media list */
	libvlc_media_list_player_play(mlp);

	sleep(10);

	libvlc_media_player_t *mp = libvlc_media_list_player_get_media_player(mlp);

	if (random_seek) {
		for (int i = 0; i < 100; i++) {
			/* Random seek to between 0% and 90% of the file */
			libvlc_media_player_set_position(mp,
				(0.9f * (float) rand()) / (float) RAND_MAX);

			printf("VLCDUMMY SEEK\n");

			sleep(10);
		}

		/* Seek to end of file */
		libvlc_media_player_set_position(mp, 1.0f);
	}

	while (libvlc_media_list_player_get_state(mlp) != libvlc_Ended) {
		pthread_cond_wait(&state_change_cond, &state_change_mutex);
	}

	libvlc_media_list_release(ml);
}

static void
play_media(libvlc_instance_t *vlc, libvlc_media_t *md)
{
	libvlc_event_manager_t *mdem = libvlc_media_event_manager(md);
	assert(mdem != NULL);

	libvlc_event_attach(mdem, libvlc_MediaSubItemAdded, media_event_handler, NULL);
	libvlc_event_attach(mdem, libvlc_MediaStateChanged, media_event_handler, NULL);

	libvlc_media_player_t *mi = libvlc_media_player_new_from_media(md);
	assert(mi != NULL);

	libvlc_event_manager_t *mpem = libvlc_media_player_event_manager(mi);
	assert(mpem != NULL);

	libvlc_event_attach(mpem, libvlc_MediaPlayerOpening, media_player_event_handler, NULL);
	libvlc_event_attach(mpem, libvlc_MediaPlayerBuffering, media_player_event_handler, NULL);
	libvlc_event_attach(mpem, libvlc_MediaPlayerPlaying, media_player_event_handler, NULL);
	libvlc_event_attach(mpem, libvlc_MediaPlayerPaused, media_player_event_handler, NULL);	
	libvlc_event_attach(mpem, libvlc_MediaPlayerStopped, media_player_event_handler, NULL);

	/* Start playing media */
	libvlc_media_player_play(mi);

	while (libvlc_media_player_get_state(mi) != libvlc_Ended) {
		pthread_cond_wait(&state_change_cond, &state_change_mutex);
	}

	libvlc_media_player_release(mi);

	if (play_subitems) {
		play_media_list(vlc, libvlc_media_subitems(md));
	}
}

int
main(int argc, char **argv)
{
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--play-item") == 0) {
			play_item = true;
		} else if (strcmp(argv[i], "--print-item") == 0) {
			print_item = true;
		} else if (strcmp(argv[i], "--play-subitems") == 0) {
			play_subitems = true;
		} else if (strcmp(argv[i], "--print-subitems") == 0) {
			print_subitems = true;
		} else if (strcmp(argv[i], "--random-seek") == 0) {
			random_seek = true;
		}
	}

	libvlc_instance_t *vlc = libvlc_new(sizeof(args) / sizeof(args[0]), args);
	assert(vlc != NULL);

	libvlc_media_t *md = strstr(argv[1], "://") ?
		libvlc_media_new_location(vlc, argv[1]) :
		libvlc_media_new_path(vlc, argv[1]);
	assert(md != NULL);

	if (print_item) {
		printf(
			"VLCDUMMY ITEM %s\n",
			libvlc_media_get_meta(md, libvlc_meta_Title));
	}

	if (play_item) {
		play_media(vlc, md);
	}

	libvlc_media_release(md);

	/* Can't do libvlc_release here because vlc will unload libtorrent before
	   the session thread finishes, which leads to crashes in the
	   destroy_session function. I guess the way to properly fix this bug is to
	   drop the session thread altogether. */
	//libvlc_release(vlc);

	printf("VLCDUMMY END\n");

	/* Flush output in case we trigger the libtorrent call-after-free bug */
	fflush(stdout);

	return 0;
}
