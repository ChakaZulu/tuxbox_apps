/*
 * $Id: streamts.c,v 1.6 2002/08/11 01:44:09 obi Exp $
 * 
 * inetd style daemon for streaming avpes, ps and ts
 * 
 * Copyright (C) 2002 Andreas Oberritter <obi@tuxbox.org>
 *
 * based on code which is
 * Copyright (C) 2001 TripleDES
 * Copyright (C) 2000, 2001 Marcus Metzler <marcus@convergence.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 *
 * usage:
 * 	wget http://dbox:port/apid,vpid (for ps or avpes)
 * 	wget http://dbox:port/pid1,pid2,.... (for ts)
 *
 * 	each pid must consist of 4 characters and must be hex
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include <ost/dmx.h>
#include <ost/frontend.h>
#include <ost/sec.h>
#include <ost/video.h>

#include <transform.h>

#define BSIZE	1024 * 16
#define MAXPIDS	8
#define DMXDEV	"/dev/ost/demux0"
#define DVRDEV	"/dev/ost/dvr0"

#define IN_SIZE	TS_SIZE * 10

int dvrfd;
int demuxfd[MAXPIDS];
int demuxfd_count;

void bye (int return_code)
{
	/* close all demux devices */
	while (demuxfd_count > 0)
	{
		close(demuxfd[--demuxfd_count]);
	}

	close(dvrfd);
	exit(return_code);
}

void write_stdout(uint8_t *buf, int count, void *p)
{
	if (write(STDOUT_FILENO, buf, count) != count)
	{
		perror("write");
		bye(0);
	}
}

int dvr_to_ps (int dvr_fd, uint16_t audio_pid, uint16_t video_pid, uint8_t ps)
{
	uint8_t buf[IN_SIZE];
	uint8_t mbuf[TS_SIZE];
	int i;
	int c;
	int len;
        int count;
	uint16_t pid;
	uint8_t off;

	ipack pa, pv;
	ipack *p;

	init_ipack(&pa, 2048, write_stdout, ps);
	init_ipack(&pv, 2048, write_stdout, ps);

	/* read 188 bytes */
	if (save_read(dvr_fd, mbuf, TS_SIZE) < 0)
	{
		perror("read");
		return -1;
	}

	/* find beginning of transport stream */
	for (i = 0; i < TS_SIZE ; i++)
	{
		if (mbuf[i] == 0x47)
			break;
	}

	/* store first part of ts packet */
	memcpy(buf, mbuf + i, TS_SIZE - i);

	/* read missing bytes of ts packet */
	if (read(dvr_fd, mbuf, i) < 0)
	{
		perror("read");
		return -1;
	}

	/* store second part of ts packet */
	memcpy(buf + TS_SIZE - i, mbuf, i);

	len = TS_SIZE;

	while(1)
	{
		count = 0;
		while (count < IN_SIZE - TS_SIZE)
		{
			if ((c = save_read(dvr_fd, buf + (len + count), IN_SIZE - (len + count))) > 0)
			{
				count += c;
			}
		}

		for(i = 0; i < count; i += TS_SIZE)
		{
			off = 0;
			pid = get_pid(buf + i + 1);
			
			if (!(buf[i + 3] & 0x10)) // no payload?
			{
				continue;
			}
			if (pid == video_pid)
			{
				p = &pv;
			}
			else if (pid == audio_pid)
			{
				p = &pa;
			}
			else
			{
				continue;
			}

			if (buf[i + 1] & 0x40)
			{
				if (p->plength == MMAX_PLENGTH - 6)
				{
					p->plength = p->found - 6;
					p->found = 0;
					send_ipack(p);
					reset_ipack(p);
				}
			}

			if (buf[i + 3] & 0x20) // adaptation field?
			{
				off = buf[i + 4] + 1;
			}

			instant_repack(buf + 4 + off + i, TS_SIZE - 4 - off, p);
		}

		len = 0;
	}
	return 0;
}

int setPesFilter (int pid, dmxPesType_t type)
{
	int fd;
	struct dmxPesFilterParams flt; 

	switch(type)
	{
	case DMX_PES_AUDIO:
	case DMX_PES_VIDEO:
	case DMX_PES_TELETEXT:
	case DMX_PES_SUBTITLE:
	case DMX_PES_PCR:
	case DMX_PES_OTHER:
		break;
	default:
		return -1;
	}

	if ((fd = open(DMXDEV, O_RDWR)) < 0)
	{
		perror(DMXDEV);
		return -1;
	}
	ioctl(fd, DMX_SET_BUFFER_SIZE, 1024 * 1024);

	flt.pid = pid;
	flt.input = DMX_IN_FRONTEND;
	flt.output = DMX_OUT_TS_TAP;
	flt.pesType = type;
	flt.flags = 0;

	if (ioctl(fd, DMX_SET_PES_FILTER, &flt) < 0)
	{
		perror("DMX_SET_PES_FILTER");
		return -1;
	}

	if (ioctl(fd, DMX_START, 0) < 0)
	{
		perror("DMX_START");
		return -1;
	}

	return fd;
}

void usage ()
{
	printf("valid command line parameters:\n");
	printf(" -pes   send a packetized elementary stream (2 pids)\n");
	printf(" -ps    send a program stream (2 pids)\n");
	printf(" -ts    send a transport stream (max. %d pids)\n", MAXPIDS);
}

int main(int argc, char **argv)
{
	int pid;
	int pids[MAXPIDS];
	dmxPesType_t type;	/* 0..6 */
	char buffer[BSIZE];
	char *bp;
	uint8_t mode;

	demuxfd_count = 0;

	if (argc != 2)
	{
		usage();
		return 0;
	}
	if (!strncmp(argv[1], "-pes", 4))
	{
		mode = 0;
	}
	else if (!strncmp(argv[1], "-ps", 3))
	{
		mode = 1;
	}
	else if (!strncmp(argv[1], "-ts", 3))
	{
		mode = 2;
	}
	else
	{
		usage();
		return 0;
	}

	bp = buffer;
	while (bp - buffer < BSIZE)
	{
		unsigned char c;
		read(1, &c, 1);
		if ((*bp++ = c) == '\n')
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

	if ((dvrfd = open(DVRDEV, O_RDONLY)) < 0)
	{
		perror(DVRDEV);
		return -dvrfd;
	}

	/* pids shall be each 4 bytes, comma separated */
	/* a better solution might be to specify the pes type to allow watching tv while streaming(?) */
	do
	{
		type = DMX_PES_OTHER;
		sscanf(bp, "%x", &pid);
		bp += 4;

		//sscanf(bp, "%x-%x", &type, &pid);
		//bp += 6;
		
		pids[demuxfd_count] = pid;

		if ((demuxfd[demuxfd_count] = setPesFilter(pid, type)) < 0)
		{
			bye(-1);
		}
		else
		{
			demuxfd_count++;
		}
	}
	while ((*bp++ == ',') && (demuxfd_count < MAXPIDS));

	if (mode < 2)
	{
		/* convert transport stream and write to stdout */
		if (demuxfd_count != 2)
		{
			usage();
			bye(0);
		}

		dvr_to_ps(dvrfd, pids[0], pids[1], mode);
	}
	else
	{
		while (1)
		{
			int pr = 0;
			int r;
			int tr = BSIZE;

			while (tr)
			{
				if ((r = read(dvrfd, buffer + pr, tr)) <= 0)
					continue;
				pr += r;
				tr -= r;
			}

			write_stdout(buffer, r, NULL);
		}
	}

	bye(0);

	return 0;
}

