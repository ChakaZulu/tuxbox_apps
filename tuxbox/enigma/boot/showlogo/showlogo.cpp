/*
 * $Id: showlogo.cpp,v 1.4 2005/09/25 07:29:21 digi_casi Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <config.h>
#if HAVE_DVB_API_VERSION < 3
#include <ost/dmx.h>
#include <ost/video.h>
#include <ost/audio.h>
#define VIDEO_DEV "/dev/dvb/card0/video0"
#define AUDIO_DEV "/dev/dvb/card0/audio0"
#define DEMUX_DEV "/dev/dvb/card0/demux0"
#else
#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>
#include <linux/dvb/audio.h>
#define VIDEO_DEV "/dev/dvb/adapter0/video0"
#define AUDIO_DEV "/dev/dvb/adapter0/audio0"
#define DEMUX_DEV "/dev/dvb/adapter0/demux0"
#define audioStatus audio_status
#define videoStatus video_status
#define pesType pes_type
#define playState play_state
#define audioStreamSource_t audio_stream_source_t
#define videoStreamSource_t video_stream_source_t
#define streamSource stream_source
#define dmxPesFilterParams dmx_pes_filter_params
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

// #define OLD_VBI

#undef strcpy
#undef strcmp
#undef strlen
#undef strncmp

#ifdef OLD_VBI  // oldvbi header
#include <dbox/avia_gt_vbi.h>
#endif

// Non DVB API Functions and ioctls

#define VIDEO_FLUSH_CLIP_BUFFER 0
#define VIDEO_GET_PTS           _IOR('o', 1, unsigned int*)
#define VIDEO_SET_AUTOFLUSH     _IOW('o', 2, int)
#define VIDEO_CLEAR_SCREEN      3
#define VIDEO_SET_FASTZAP	_IOW('o', 4, int)

#define LOGO "/root/platform/kernel/bild"

void displayIFrame(const char *frame, int len)
{
	int fdv = open("/dev/video", O_WRONLY);
	if (fdv > 0)
	{
		int fdvideo = open(VIDEO_DEV, O_RDWR);
		ioctl(fdvideo, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_MEMORY );
		ioctl(fdvideo, VIDEO_CLEAR_BUFFER);
		ioctl(fdvideo, VIDEO_PLAY);
	
		for (int i = 0; i < 2; i++)
			write(fdv, frame, len);

		unsigned char buf[128];
		memset(&buf, 0, 128);
		write(fdv, &buf, 128);

		ioctl(fdv, VIDEO_SET_AUTOFLUSH, 0);
		ioctl(fdvideo, VIDEO_SET_BLANK, 0);
		close(fdvideo);
		ioctl(fdv, VIDEO_SET_AUTOFLUSH, 1);

		close(fdv);
	}
}

void displayIFrameFromFile(const char *filename)
{
	int file = open(filename, O_RDONLY);
	if (file > 0)
	{
		int size = lseek(file, 0, SEEK_END);
		lseek(file, 0, SEEK_SET);
		if (size > 0)
		{
			char *buffer = new char[size];
			read(file, buffer, size);
			displayIFrame(buffer, size);
			delete[] buffer;
		}
		close(file);
	}
}

int main(int argc, char **argv) 
{
	char *logo = (argc < 2) ? strdup(LOGO) : argv[1];
	displayIFrameFromFile(logo);
	return EXIT_SUCCESS;
}

