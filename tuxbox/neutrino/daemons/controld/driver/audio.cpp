/*
	Control-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/



	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "audio.h"
#include "dbox/avs_core.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



void audioControl::setAudioMode(int mode)
{
	int fd;

	if ((fd = open("/dev/ost/audio0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd,AUDIO_CHANNEL_SELECT,&mode)< 0)
	{
		perror("AVSIOGVOL:");
		return;
	}
	close(fd);
}

void audioControl::setMuteSPDIF(bool mute)
{
	int fd;
	int i = mute?SPDIF_OFF:SPDIF_ON;

	if ((fd = open("/dev/ost/audio0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd,AUDIO_SPDIF_SET,&i)< 0)
	{
		perror("AVSIOGVOL:");
		return;
	}
	close(fd);

}

void audioControl::setVolume(char volume)
{
	int fd;

	int i = 64-int(volume*64.0/100.0);

	if (i < 0)
	{
		i=0;
	}
	else if (i > 63)
	{
		i=63;
	}

	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd,AVSIOSVOL,&i)< 0)
	{
		perror("AVSIOGVOL:");
		return;
	}
	close(fd);
}

void audioControl::setMute(bool mute)
{
	int i;
	int fd;

	i=mute?AVS_MUTE:AVS_UNMUTE;

	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd,AVSIOSMUTE,&i)< 0)
	{
		perror("AVSIOSMUTE:");
		return;
	}
	close(fd);
}

