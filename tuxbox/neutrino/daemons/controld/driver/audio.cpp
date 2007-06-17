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

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dbox/avs_core.h>
#include "audio.h"

#define AVS_DEVICE "/dev/dbox/avs0"

void audioControl::setVolume(const unsigned char volume)
{
#ifndef HAVE_DREAMBOX_HARDWARE
	int fd;

	int i = volume;

	if ((fd = open(AVS_DEVICE, O_RDWR)) < 0)
		perror("[controld] " AVS_DEVICE);
	else {
		if (ioctl(fd, AVSIOSVOL, &i) < 0)
			perror("[controld] AVSIOSVOL");

		close(fd);
	}
#endif
}

void audioControl::setMute(const bool mute)
{
#ifndef HAVE_DREAMBOX_HARDWARE
	int fd, a;
	
	a = mute ? AVS_MUTE : AVS_UNMUTE;

	if ((fd = open(AVS_DEVICE, O_RDWR)) < 0)
		perror("[controld] " AVS_DEVICE);

	else {
		if (ioctl(fd, AVSIOSMUTE, &a) < 0)
			perror("[controld] AVSIOSMUTE");

		close(fd);
	}
#endif
}

