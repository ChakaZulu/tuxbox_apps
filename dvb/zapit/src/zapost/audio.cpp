/*
 * $Id:
 *
 * (C) 2002 by Steffen Hehn 'McClean'
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

#include "audio.h"

#include <ost/audio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>



CAudio::CAudio()
{
	audio_fd = -1;
}

CAudio::~CAudio()
{
	if (audio_fd != -1)
	{
		close(audio_fd);
	}
}

 /*
 * return 0 on success or if nothing to do
 * return -1 otherwise
 */
int CAudio::setBypassMode (bool isAc3)
{
/*
	if (isAc3 == wasAc3)
	{
		return 0;
	}
*/
	if (audio_fd != -1)
	{
		close(audio_fd);
	}

	if ((audio_fd = open(AUDIO_DEV, O_RDWR)) < 0)
	{
		perror("[zapit] " AUDIO_DEV);
		return -1;
	}

	if (ioctl(audio_fd, AUDIO_SET_BYPASS_MODE, isAc3 ? 0 : 1) < 0)
	{
		perror("[zapit] AUDIO_SET_BYPASS_MODE");
		close(audio_fd);
		return -1;
	}

//	wasAc3 = isAc3;

	return 0;
}

/*
 * return 0 on success
 * return -1 otherwise
 */
int CAudio::setMute (bool mute)
{
	if ((audio_fd == -1) && ((audio_fd = open(AUDIO_DEV, O_RDWR)) < 0))
	{
		perror("[zapit] " AUDIO_DEV);
		return -1;
	}

	if (ioctl(audio_fd, AUDIO_SET_MUTE, mute) < 0)
	{
		perror("[zapit] AUDIO_SET_MUTE");
		return -1;
	}

	return 0;
}

/*
 * return false on success
 * return true otherwise
 */
bool CAudio::mute()
{
	return setMute(true);
}

/*
 * return false on success
 * return true otherwise
 */
bool CAudio::unMute()
{
	return setMute(false);
}

/*
 * return false on success
 * return true otherwise
 */
bool CAudio::enableBypass()
{
	return setBypassMode(true);
}

/*
 * return false on success
 * return true otherwise
 */
bool CAudio::disableBypass()
{
	return setBypassMode(false);
}

/*
 * return 0 on success
 * return -1 otherwise
 */
int CAudio::setVolume (unsigned int left, unsigned int right)
{
	audioMixer_t mixer;

	if ((audio_fd == -1) && ((audio_fd = open(AUDIO_DEV, O_RDWR)) < 0))
	{
		perror("[zapit] " AUDIO_DEV);
		return -1;
	}

	mixer.volume_left = left;
	mixer.volume_right = right;

	if (ioctl(audio_fd, AUDIO_SET_MIXER, &mixer) < 0)
	{
		perror("[zapit] AUDIO_SET_MIXER");
		return -1;
	}

	return 0;
}
