/*
 * $Id: audio.cpp,v 1.14 2007/06/04 17:06:43 dbluelle Exp $
 *
 * (C) 2002 by Steffen Hehn 'McClean' &
 *	Andreas Oberritter <obi@tuxbox.org>
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <zapit/audio.h>
#include <zapit/debug.h>
#include <zapit/settings.h>

CAudio::CAudio(void)
{
	if ((fd = open(AUDIO_DEVICE, O_RDWR)) < 0)
		ERROR(AUDIO_DEVICE);
}

CAudio::~CAudio(void)
{
	if (fd >= 0)
		close(fd);
}

int CAudio::setBypassMode(int disable)
{
	return quiet_fop(ioctl, AUDIO_SET_BYPASS_MODE, disable);
}

int CAudio::setMute(int enable)
{
	return fop(ioctl, AUDIO_SET_MUTE, enable);
}

int CAudio::enableBypass(void)
{
	return setBypassMode(0);
}

int CAudio::disableBypass(void)
{
	return setBypassMode(1);
}

int CAudio::mute(void)
{
	return setMute(1);
}

int CAudio::unmute(void)
{
	return setMute(0);
}

int CAudio::setVolume(unsigned int left, unsigned int right)
{
	struct audio_mixer mixer;
	mixer.volume_left = left;
	mixer.volume_right = right;
	return fop(ioctl, AUDIO_SET_MIXER, &mixer);
}

int CAudio::setSource(audio_stream_source_t source)
{
	return fop(ioctl, AUDIO_SELECT_SOURCE, source);
}

audio_stream_source_t CAudio::getSource(void)
{
	struct audio_status status;
	fop(ioctl, AUDIO_GET_STATUS, &status);
	return status.stream_source;
}

int CAudio::start(void)
{
	return fop(ioctl, AUDIO_PLAY);
}

int CAudio::stop(void)
{
	return fop(ioctl, AUDIO_STOP);
}

int CAudio::setChannel(audio_channel_select_t channel)
{
	return fop(ioctl, AUDIO_CHANNEL_SELECT, channel);
}

audio_channel_select_t CAudio::getChannel(void)
{
	struct audio_status status;
	fop(ioctl, AUDIO_GET_STATUS, &status);
	return status.channel_select;
}

