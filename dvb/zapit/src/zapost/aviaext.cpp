/*
 * $Id: aviaext.cpp,v 1.1 2005/01/18 10:32:28 diemade Exp $
 *
 * (C) 2005 by Axel Buehning 'DieMade'
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

#include <zapit/aviaext.h>
#include <zapit/debug.h>
#include <zapit/settings.h>

#include <dbox/aviaEXT.h>

#define AVIAEXT_DEV "/dev/dbox/aviaEXT"

CAViAext::CAViAext(void)
{
	if ((fd = open(AVIAEXT_DEV, O_RDWR)) < 0)
		ERROR(AVIAEXT_DEV);
}

CAViAext::~CAViAext(void)
{
	if (fd >= 0)
		close(fd);
}

void CAViAext::iecOn(void)
{
	int res=0;

	if (fd < 0)
		return;

	res = ioctl(fd, AVIA_EXT_IEC_SET, 1);
	if (res<0)
		perror("aviaext: ioctl");
}

void CAViAext::iecOff(void)
{
	int res;
	
	if (fd < 0)
		return;


	res = ioctl(fd, AVIA_EXT_IEC_SET, 0);
	if (res<0)
		perror("aviaext: ioctl");
}

int CAViAext::iecState(void)
{
	int res;
	unsigned int param;
	
	if (fd < 0)
		return(-1);

	res = ioctl(fd, AVIA_EXT_IEC_GET, &param);

	if (res<0)
	{
		perror("aviaext: ioctl");
		return -1;
	}
	return param;
}	

void CAViAext::playbackSPTS(void)
{
	int res;
	
	if (fd < 0)
		return;

	res = ioctl(fd, AVIA_EXT_AVIA_PLAYBACK_MODE_SET, 1);
	if (res<0)
		perror("aviaext: ioctl");
}

void CAViAext::playbackPES(void)
{
	int res;
	
	if (fd < 0)
		return;

	res = ioctl(fd, AVIA_EXT_AVIA_PLAYBACK_MODE_SET, 0);
	if (res<0)
		perror("aviaext: ioctl");
}

int CAViAext::playbackState(void)
{
	int res;
	unsigned int param;
	
	if (fd < 0)
		return(-1);

	res = ioctl(fd, AVIA_EXT_AVIA_PLAYBACK_MODE_GET, &param);
	if (res<0)
	{
		perror("aviaext: ioctl");
		return -1;
	}
	return param;
}

