/*
 * $Header: /cvs/tuxbox/apps/misc/libs/libconnection/basicsocket.cpp,v 1.1 2003/02/24 14:05:02 thegoodguy Exp $
 *
 * Basic Socket Class - The Tuxbox Project
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
 *
 * License: GPL
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

#include "basicsocket.h"

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

bool send_data(int fd, const void * data, const size_t size, const timeval timeout)
{
	fd_set       readfds, writefds, exceptfds;
	timeval      tv;
	const void * buffer;
	size_t       n;
	int          rc;

	n = size;

	while (n > 0)
	{
		buffer = (void *)((char *)data + (size - n));
		rc = ::send(fd, buffer, n, MSG_DONTWAIT | MSG_NOSIGNAL);
		
		if (rc == -1)
		{
			perror("[basicsocket] send_data");
			if (errno == EPIPE)
				return false;
			
			FD_ZERO(&readfds);
			FD_ZERO(&writefds);
			FD_ZERO(&exceptfds);
			FD_SET(fd, &writefds);
			
			tv = timeout;
			
			rc = select(fd + 1, &readfds, &writefds, &exceptfds, &tv);
			
			if (rc == 0)
			{
				printf("[basicsocket] send timed out.\n");
				return false;
			}
			if (rc == -1)
			{
				perror("[basicsocket] send_data select");
				return false;
			}
		}
		else
			n -= rc;
	}
	return true;
}
