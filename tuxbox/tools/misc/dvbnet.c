/*
 *   $Id: dvbnet.c,v 1.4 2002/08/27 19:00:46 obi Exp $
 *
 *   dvbnet.c - setup dvb net device (dbox-II-project)
 *
 *   Homepage: http://dbox2.elxsi.de
 *
 *   Copyright (C) 2001 Gillem
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 *   $Log: dvbnet.c,v $
 *   Revision 1.4  2002/08/27 19:00:46  obi
 *   use devfs device names
 *
 *   Revision 1.3  2002/08/21 08:25:47  obi
 *   no more compile warnings
 *
 *   Revision 1.2  2001/06/25 19:04:01  gillem
 *   - some changes
 *
 *
*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <ost/net.h>

int main(int argc, char **argv)
{
	int fd;
	struct dvb_net_if net_if;

	memset( &net_if, 0, sizeof(struct dvb_net_if));

	net_if.pid    = 0;
	net_if.if_num = 0;

	if((fd = open("/dev/dvb/card0/net0",O_RDWR|O_NONBLOCK)) < 0)
	{
		perror("NET DEVICE: ");
		return -1;
	}

	if ( (ioctl(fd,NET_ADD_IF,&net_if) < 0))
	{
		perror("NET ADD: ");
		close(fd);
		return -1;
	}

	if ( (ioctl(fd,NET_REMOVE_IF,net_if.if_num) < 0))
	{
		perror("NET REMOVE: ");
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}
