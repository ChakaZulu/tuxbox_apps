/*
 * $Id: video.cpp,v 1.10 2003/12/19 23:35:47 derget Exp $
 *
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
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
#include <sys/ioctl.h>
#include <unistd.h>

#include <zapit/debug.h>
#include <zapit/settings.h>
#include <zapit/video.h>

CVideo::CVideo(void)
{
	if ((fd = open(VIDEO_DEVICE, O_RDWR)) < 0)
		ERROR(VIDEO_DEVICE);

	setBlank(true);
}

CVideo::~CVideo(void)
{
	if (fd >= 0)
		close(fd);
}

int CVideo::setAspectRatio(video_format_t format)
{
	return fop(ioctl, VIDEO_SET_FORMAT, format);
}

video_format_t CVideo::getAspectRatio(void)
{
	struct video_status status;
	fop(ioctl, VIDEO_GET_STATUS, &status);
	return status.video_format;
}

int CVideo::setCroppingMode(video_displayformat_t format)
{
	return fop(ioctl, VIDEO_SET_DISPLAY_FORMAT, format);
}

video_displayformat_t CVideo::getCroppingMode(void)
{
	struct video_status status;
	fop(ioctl, VIDEO_GET_STATUS, &status);
	return status.display_format;
}

int CVideo::setSource(video_stream_source_t source)
{
	return fop(ioctl, VIDEO_SELECT_SOURCE, source);
}

video_stream_source_t CVideo::getSource(void)
{
	struct video_status status;
	fop(ioctl, VIDEO_GET_STATUS, &status);
	return status.stream_source;
}

int CVideo::start(void)
{
	return fop(ioctl, VIDEO_PLAY);
}

int CVideo::stop(void)
{
	return fop(ioctl, VIDEO_STOP);
}

int CVideo::setBlank(int enable)
{
	return fop(ioctl, VIDEO_SET_BLANK, enable);
}

int CVideo::getBlank(void)
{
	struct video_status status;
	fop(ioctl, VIDEO_GET_STATUS, &status);
	return status.video_blank;
}

video_play_state_t CVideo::getPlayState(void)
{
	struct video_status status;
	fop(ioctl, VIDEO_GET_STATUS, &status);
	return status.play_state;
}

int CVideo::setVideoSystem(int video_system)
{
        return fop(ioctl, VIDEO_SET_SYSTEM, video_system);
}

