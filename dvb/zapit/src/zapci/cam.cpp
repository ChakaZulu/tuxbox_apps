/*
 * $Id: cam.cpp,v 1.22 2002/09/21 17:58:42 thegoodguy Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string>
#include <unistd.h>

#include "cam.h"

/* zapit */
#include <settings.h>   // CAMD_UDS_NAME

CCam::CCam ()
{
	camdSocket = -1;
}

bool CCam::camdConnect ()
{
	struct sockaddr_un servaddr;
	int clilen;

	std::string filename = CAMD_UDS_NAME;

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, filename.c_str());
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);

	if ((camdSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("[CCam::camdConnect] socket");
		return false;
	}

	if (connect(camdSocket, (struct sockaddr*) &servaddr, clilen) < 0)
	{
		perror("[CCam::camdConnect] connect");
		return false;
	}

	return true;
}

void CCam::camdDisconnect ()
{
	if (camdSocket != -1)
	{
		close(camdSocket);
		camdSocket = -1;
	}
}

int CCam::sendMessage (unsigned char * data, unsigned short length)
{
	camdDisconnect();

	if (camdConnect() == false)
	{
		return -1;
	}
	else if (write(camdSocket, data, length) < 0)
	{
		perror("[CCam::sendMessage] write");
		camdDisconnect();
		return -1;
	}
	else
		return 0;
}

int CCam::setCaPmt (CCaPmt * caPmt)
{
	unsigned char buffer[caPmt->getLength()];

	unsigned int pos = caPmt->writeToBuffer(buffer);

	return sendMessage(buffer, pos);
}
