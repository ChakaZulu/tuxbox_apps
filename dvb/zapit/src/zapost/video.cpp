/*
 * $Id: video.cpp,v 1.7 2002/12/17 23:07:50 obi Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <zapit/debug.h>
#include <zapit/settings.h>
#include <zapit/video.h>


CVideo::CVideo ()
{
	initialized = false;
	status.play_state = VIDEO_STOPPED;
	status.stream_source = VIDEO_SOURCE_DEMUX;

	if ((fd = open(VIDEO_DEVICE, O_RDWR)) < 0)
	{
		ERROR(VIDEO_DEVICE);
	}
	else if (ioctl(fd, VIDEO_GET_STATUS, &status) < 0)
	{
		ERROR("VIDEO_GET_STATUS");
		close(fd);
	}
	else
	{
		setBlank(true);
		initialized = true;
	}
}

CVideo::~CVideo ()
{
	if (initialized)
		close(fd);
}

int CVideo::setAspectRatio (video_format_t format)
{
	if (status.video_format == format)
		return 0;

	if (ioctl(fd, VIDEO_SET_FORMAT, format) < 0)
	{
		ERROR("VIDEO_SET_FORMAT");
		return -1;
	}

	status.video_format = format;
	return 0;
}

int CVideo::setCroppingMode (video_displayformat_t format)
{
	if (status.display_format == format)
		return 0;

	if (ioctl(fd, VIDEO_SET_DISPLAY_FORMAT, format) < 0)
	{
		ERROR("VIDEO_SET_DISPLAY_FORMAT");
		return -1;
	}

	status.display_format = format;
	return 0;
}

int CVideo::setSource (video_stream_source_t source)
{
	if (status.stream_source == source)
		return 0;

	if (status.play_state != VIDEO_STOPPED)
		return -1;

	if (ioctl(fd, VIDEO_SELECT_SOURCE, source) < 0)
	{
		ERROR("VIDEO_SELECT_SOURCE");
		return -1;
	}

	status.stream_source = source;

	return 0;
}

int CVideo::start ()
{
	if (status.play_state == VIDEO_PLAYING)
		return 0;

	if (ioctl(fd, VIDEO_PLAY) < 0)
	{
		ERROR("VIDEO_PLAY");
		return -1;
	}

	status.play_state = VIDEO_PLAYING;

	return 0;
}

int CVideo::stop ()
{
	if (status.play_state == VIDEO_STOPPED)
		return 0;

	if (ioctl(fd, VIDEO_STOP, status.video_blank) < 0)
	{
		ERROR("VIDEO_STOP");
		return -1;
	}

	status.play_state = VIDEO_STOPPED;

	return 0;
}

int CVideo::setBlank (bool blank)
{
	if (status.video_blank == blank)
		return 0;

	if (ioctl(fd, VIDEO_SET_BLANK, blank) < 0)
	{
		ERROR("VIDEO_SET_BLANK");
		return -1;
	}

	status.video_blank = blank;

	return 0;
}

