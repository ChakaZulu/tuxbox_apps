/*
 * $Id: streampes.c,v 1.6 2003/01/07 00:43:59 obi Exp $
 *
 * Copyright (C) 2001 by tmbinc
 * Copyright (C) 2001 by kwon
 * Copyright (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <linux/dvb/dmx.h>

#define BSIZE					 1024*16

int main(int argc, char **argv)
{
	int fd;
	unsigned short pid;
	struct dmx_pes_filter_params flt; 
	char buffer[BSIZE], *bp;
	unsigned char c;
	
	bp = buffer;

	while (bp-buffer < BSIZE) {

		read(STDIN_FILENO, &c, 1);

		if ((*bp++=c)=='\n')
			break;
	}

	*bp++ = 0;

	bp = buffer;

	if (!strncmp(buffer, "GET /", 5))
	{
		printf("HTTP/1.1 200 OK\r\nServer: d-Box network\r\n\r\n");
		bp += 5;
	}

	fflush(stdout);

	fd = open("/dev/dvb/adapter0/demux0", O_RDWR);

	if (fd < 0) {
		perror("/dev/dvb/adapter0/demux0");
		return -fd;
	}

	ioctl(fd, DMX_SET_BUFFER_SIZE, 256 * 1024);
	sscanf(bp, "%hx", &pid);

	flt.pid = pid;
	flt.input = DMX_IN_FRONTEND;
	flt.output = DMX_OUT_TAP;
	flt.pes_type = DMX_PES_OTHER;
	flt.flags = DMX_IMMEDIATE_START;

	if (ioctl(fd, DMX_SET_PES_FILTER, &flt) < 0) {
		perror("DMX_SET_PES_FILTER");
		return errno;
	}

	while (1) {

		int pr = 0, r;
		int tr = BSIZE;

		while (tr) {

			if ((r=read(fd, buffer+pr, tr)) <= 0)
				continue;
			pr+=r;
			tr-=r;
		}

		if (write(STDOUT_FILENO, buffer, r) != r)
			break;
	}

	close(fd);
	return 0;
}

