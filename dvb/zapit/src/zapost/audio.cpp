/*
 * $Id: audio.cpp,v 1.10 2002/11/02 17:21:15 obi Exp $
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

#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <zapit/audio.h>
#include <zapit/settings.h>

CAudio::CAudio()
{
	initialized = false;

	mixer.volume_left = 0;
	mixer.volume_right = 0;
	status.bypass_mode = false;
	status.play_state = AUDIO_STOPPED;
	status.stream_source = AUDIO_SOURCE_MEMORY;

	if ((fd = open(AUDIO_DEVICE, O_RDWR)) < 0)
	{
		perror(AUDIO_DEVICE);
	}
	else if (ioctl(fd, AUDIO_GET_STATUS, &status) < 0)
	{
		perror("AUDIO_GET_STATUS");
		close(fd);
	}
	else if (ioctl(fd, AUDIO_SET_MIXER, &mixer) < 0)
	{
		perror("AUDIO_SET_MIXER");
		close(fd);
	}
	else
	{
		initialized = true;
	}
}

CAudio::~CAudio()
{
	if (initialized)
	{
		close(fd);
	}
}

int CAudio::setBypassMode (bool enable)
{
	if (ioctl(fd, AUDIO_SET_BYPASS_MODE, enable ? 0 : 1) < 0)
	{
		perror("AUDIO_SET_BYPASS_MODE");
		return -1;
	}

	status.bypass_mode = enable;

	return 0;
}

int CAudio::setMute (bool enable)
{
	if (ioctl(fd, AUDIO_SET_MUTE, enable) < 0)
	{
		perror("AUDIO_SET_MUTE");
		return -1;
	}

	status.mute_state = enable;

	return 0;
}

int CAudio::mute ()
{
	if (status.mute_state == false)
	{
		return setMute(true);
	}
	else
	{
		return -1;
	}

}

int CAudio::unmute ()
{
	if (status.mute_state == true)
	{
		return setMute(false);
	}
	else
	{
		return -1;
	}
}

int CAudio::enableBypass ()
{
	if (status.bypass_mode == false)
	{
		return setBypassMode(true);
	}
	else
	{
		return -1;
	}
}

int CAudio::disableBypass ()
{
	if (status.bypass_mode == true)
	{
		return setBypassMode(false);
	}
	else
	{
		return -1;
	}
}

int CAudio::setVolume (unsigned char left, unsigned char right)
{
	if ((mixer.volume_left == left) && (mixer.volume_right == right))
	{
		return 0;
	}

	mixer.volume_left = left;
	mixer.volume_right = right;

	if (ioctl(fd, AUDIO_SET_MIXER, &mixer) < 0)
	{
		perror("AUDIO_SET_MIXER");
		return -1;
	}

	return -1;
}

int CAudio::setSource (audio_stream_source_t source)
{
	if (status.stream_source == source)
		return 0;

	if (status.play_state != AUDIO_STOPPED)
		return -1;

	if (ioctl(fd, AUDIO_SELECT_SOURCE, source) < 0)
	{
		perror("AUDIO_SELECT_SOURCE");
		return -1;
	}

	status.stream_source = source;
	return 0;
}

int CAudio::start ()
{
	if (status.play_state == AUDIO_PLAYING)
	{
		return 0;
	}

	if (ioctl(fd, AUDIO_PLAY) < 0)
	{
		perror("AUDIO_PLAY");
		return -1;
	}

	status.play_state = AUDIO_PLAYING;

	return 0;
}

int CAudio::stop ()
{
	if (status.play_state == AUDIO_STOPPED)
	{
		return 0;
	}

	if (ioctl(fd, AUDIO_STOP) < 0)
	{
		perror("AUDIO_STOP");
		return -1;
	}

	status.play_state = AUDIO_STOPPED;

	return 0;
}

int CAudio::selectChannel (audio_channel_select_t sel)
{
	if (ioctl(fd, AUDIO_CHANNEL_SELECT, &sel) < 0)
	{
		perror("AUDIO_CHANNEL_SELECT");
		return -1;
	}

	status.channel_select = sel;

	return 0;
}

audio_channel_select_t CAudio::getSelectedChannel ()
{
	return status.channel_select;
}

