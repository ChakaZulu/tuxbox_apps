/*
 * $Header: /cvs/tuxbox/apps/misc/libs/libconnection/basicsocket.cpp,v 1.5 2009/03/28 13:49:22 seife Exp $
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
	fd_set       writefds;
	timeval      tv;
	const void * buffer;
	size_t       n;
	int          rc;
	int olderr;

	n = size;

	while (n > 0)
	{
		buffer = (void *)((char *)data + (size - n));
		rc = ::send(fd, buffer, n, MSG_DONTWAIT | MSG_NOSIGNAL);

		if (rc == -1)
		{
			olderr = errno;
			if (errno != EAGAIN) // this is "write would block...", which is not an error
				fprintf(stderr,"[basicsocket] send_data: %m (n = %d/%d, pid = %d)\n", n, size, getpid());
			if (olderr == EPIPE)
				return false;

			FD_ZERO(&writefds);
			FD_SET(fd, &writefds);

			tv = timeout;

			rc = select(fd + 1, NULL, &writefds, NULL, &tv);

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


bool receive_data(int fd, void * data, const size_t size, const timeval timeout)
{
	fd_set    readfds;
	timeval   tv;
	void    * buffer;
	size_t    n;
	int       rc;

	n = size;

	while (n > 0)
	{
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);

		tv = timeout;

		rc = select(fd + 1, &readfds, NULL, NULL, &tv);

		if (rc == 0)
		{
			printf("[basicsocket] receive timed out. waiting process %d\n", getpid());
			return false;
		}
		if (rc == -1)
		{
			perror("[basicsocket] receive_data select");
			return false;
		}
		buffer = (void *)((char *)data + (size - n));
		rc = ::recv(fd, buffer, n, MSG_DONTWAIT | MSG_NOSIGNAL);

		if ((rc == 0) || (rc == -1))
		{
			if (rc == -1)
			{
				perror("[basicsocket] receive_data");

				if (errno == EPIPE)
					return false;
			}
			else
			{
				/*
				 * silently return false
				 *
				 * printf("[basicsocket] no more data\n");
				 */
				return false;
			}

		}
		else
			n -= rc;
	}
	return true;
}
