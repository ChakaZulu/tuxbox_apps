/*
 * $Id: audio.cpp,v 1.2 2002/05/12 12:54:47 obi Exp $
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

#include "audio.h"

#define AUDIO_DEV "/dev/ost/audio0"


CAudio::CAudio()
{
	initialized = false;

	if ((fd = open(AUDIO_DEV, O_RDWR)) < 0)
	{
		perror(AUDIO_DEV);
	}
	else if (ioctl(fd, AUDIO_GET_STATUS, &status) < 0)
	{
		perror("AUDIO_GET_STATUS");
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

bool CAudio::setBypassMode (bool enable)
{
	if (ioctl(fd, AUDIO_SET_BYPASS_MODE, enable ? 0 : 1) < 0)
	{
		perror("AUDIO_SET_BYPASS_MODE");
		return false;
	}

	status.bypassMode = enable;
	return true;
}

bool CAudio::setMute (bool enable)
{
	if (ioctl(fd, AUDIO_SET_MUTE, enable) < 0)
	{
		perror("[zapit] AUDIO_SET_MUTE");
		return false;
	}

	status.muteState = enable;
	return true;
}

bool CAudio::mute()
{
	if (status.muteState == false)
		return setMute(true);
	else
		return false;

}

bool CAudio::unMute()
{
	if (status.muteState == true)
		return setMute(false);
	else
		return false;
}

bool CAudio::enableBypass()
{
	if (status.bypassMode == false)
		return setBypassMode(true);
	else
		return false;
}

bool CAudio::disableBypass()
{
	if (status.bypassMode == true)
		return setBypassMode(false);
	else
		return false;
}

bool CAudio::setVolume (unsigned char left, unsigned char right)
{
	mixer.volume_left = left;
	mixer.volume_right = right;

	if (ioctl(fd, AUDIO_SET_MIXER, &mixer) < 0)
	{
		perror("AUDIO_SET_MIXER");
		return false;
	}

	return true;
}

