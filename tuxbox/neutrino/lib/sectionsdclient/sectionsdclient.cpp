/*
  Client-Interface für zapit  -   DBoxII-Project

  $Id: sectionsdclient.cpp,v 1.1 2002/01/07 21:28:22 McClean Exp $

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

  $Log: sectionsdclient.cpp,v $
  Revision 1.1  2002/01/07 21:28:22  McClean
  initial

  Revision 1.1  2002/01/06 19:10:06  Simplex
  made clientlib for zapit
  implemented bouquet-editor functions in lib


*/

#include "sectionsdclient.h"

CSectionsdClient::CSectionsdClient()
{
}

int CSectionsdClient::sectionsd_connect()
{
	int fd;

	sockaddr_in servaddr;
	char rip[]="127.0.0.1";
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;

	inet_pton(AF_INET, rip, &servaddr.sin_addr);
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	servaddr.sin_port=htons(1505);
	if(connect(fd, (sockaddr *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("[sectionsdclient] couldn't connect to  sectionsd!");
		return 0;
	}
	return fd;
}

bool CSectionsdClient::sectionsd_close(int fd)
{
	if(fd)
	{
		close(fd);
	}
}

bool CSectionsdClient::send(int fd, char* data, int size)
{
	if(fd)
	{
		write(fd, data, size);
	}
}

bool CSectionsdClient::receive(int fd, char* data, int size)
{
	if(fd)
	{
		read(fd, data, size);
	}
}
