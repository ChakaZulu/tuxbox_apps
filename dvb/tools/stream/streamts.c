/*
 * $Id: streamts.c,v 1.8 2002/09/27 05:23:27 obi Exp $
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
 * 	http://dbox:<port>/apid,vpid (for ps or avpes)
 * 	http://dbox:<port>/pid1,pid2,.... (for ts)
 *
 * 	each pid must be a hexadecimal number
 *
 * command line parameters:
 *      -pes   send a packetized elementary stream (2 pids)
 *      -ps    send a program stream (2 pids)
 *      -ts    send a transport stream (MAXPIDS pids, see below)
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <ost/dmx.h>
#include <transform.h>

/* conversion buffer sizes */
#define IN_SIZE TS_SIZE * 10
#define IPACKS 2048

/* raw ts output buffer size */
#define BSIZE	1024 * 16

/* maximum number of pes pids */
#define MAXPIDS	8

#define DMXDEV	"/dev/dvb/card0/demux0"
#define DVRDEV	"/dev/dvb/card0/dvr0"

int dvrfd;
int demuxfd[MAXPIDS];
uint8_t demuxfd_count = 0;
uint8_t exit_flag = 0;

#define PACKET_SIZE 1448

uint8_t writebuf[PACKET_SIZE];
uint16_t writebuf_size = 0;


void ps_stdout (uint8_t * buf, int count, void * p) {

	int size = 0;
	uint8_t * bp;

	while (writebuf_size + count >= PACKET_SIZE) {

		if (writebuf_size) {
			size = PACKET_SIZE - writebuf_size;
			memcpy(writebuf + writebuf_size, buf, size);
			writebuf_size = 0;
			bp = writebuf;
		}
		else {
			size = PACKET_SIZE;
			bp = buf;
		}

		if (write(STDOUT_FILENO, bp, PACKET_SIZE) != PACKET_SIZE) {
			exit_flag = 1;
			return;
		}

		buf += size;
		count -= size;
	}

	if (count) {
		memcpy(writebuf + writebuf_size, buf, count);
		writebuf_size += count;
	}
}


int dvr_to_ps (int dvr_fd, uint16_t audio_pid, uint16_t video_pid, uint8_t ps) {

	uint8_t buf[IN_SIZE];
	uint8_t mbuf[TS_SIZE];
	int i;
	int c;
	int len;
        int count;
	uint16_t pid;
	uint8_t off;

	ipack pa, pv;
	ipack * p;

	init_ipack(&pa, IPACKS, ps_stdout, ps);
	init_ipack(&pv, IPACKS, ps_stdout, ps);

	/* read 188 bytes */
	if (save_read(dvr_fd, mbuf, TS_SIZE) < 0)
		return -1;

	/* find beginning of transport stream */
	for (i = 0; i < TS_SIZE; i++) {
		if (mbuf[i] == 0x47)
			break;
	}

	/* store first part of ts packet */
	memcpy(buf, mbuf + i, TS_SIZE - i);

	/* read missing bytes of ts packet */
	if (read(dvr_fd, mbuf, i) < 0)
		return -1;

	/* store second part of ts packet */
	memcpy(buf + TS_SIZE - i, mbuf, i);

	len = TS_SIZE;

	while (!exit_flag) {

		count = 0;

		while (count < IN_SIZE - TS_SIZE)
			if ((c = save_read(dvr_fd, buf + (len + count), IN_SIZE - (len + count))) > 0)
				count += c;

		for (i = 0; i < count; i += TS_SIZE) {

			off = 0;

			if ((count - i) < TS_SIZE)
				break;

			pid = get_pid(buf + i + 1);

			if (!(buf[i + 3] & 0x10)) // no payload?
				continue;

			if (pid == video_pid)
				p = &pv;

			else if (pid == audio_pid)
				p = &pa;

			else
				continue;

			if ((buf[i + 1] & 0x40) && (p->plength == MMAX_PLENGTH - 6)) {
				p->plength = p->found - 6;
				p->found = 0;
				send_ipack(p);
				reset_ipack(p);
			}

			if (buf[i + 3] & 0x20) // adaptation field?
				off = buf[i + 4] + 1;

			instant_repack(buf + 4 + off + i, TS_SIZE - 4 - off, p);
		}

		len = 0;
	}

	return 0;
}


int setPesFilter (uint16_t pid)
{
	int fd;
	struct dmxPesFilterParams flt; 

	if ((fd = open(DMXDEV, O_RDWR)) < 0)
		return -1;

	if (ioctl(fd, DMX_SET_BUFFER_SIZE, 1024 * 1024) < 0)
		return -1;

	flt.pid = pid;
	flt.input = DMX_IN_FRONTEND;
	flt.output = DMX_OUT_TS_TAP;
	flt.pesType = DMX_PES_OTHER;
	flt.flags = 0;

	if (ioctl(fd, DMX_SET_PES_FILTER, &flt) < 0)
		return -1;

	if (ioctl(fd, DMX_START, 0) < 0)
		return -1;

	return fd;
}


int main (int argc, char ** argv) {

	int pid;
	int pids[MAXPIDS];
	char buffer[BSIZE];
	char *bp;
	uint8_t mode;

	if (argc != 2)
		return EXIT_FAILURE;

	if (!strncmp(argv[1], "-pes", 4))
		mode = 0;

	else if (!strncmp(argv[1], "-ps", 3))
		mode = 1;

	else if (!strncmp(argv[1], "-ts", 3))
		mode = 2;

	else
		return EXIT_FAILURE;

	bp = buffer;

	while (bp - buffer < BSIZE) {

		unsigned char c;

		read(STDIN_FILENO, &c, 1);

		if ((*bp++ = c) == '\n')
			break;
	}

	*bp++ = 0;

	bp = buffer;

	if (!strncmp(buffer, "GET /", 5)) {
		printf("HTTP/1.1 200 OK\r\nServer: d-Box network\r\n\r\n");
		bp += 5;
	}

	fflush(stdout);

	if ((dvrfd = open(DVRDEV, O_RDONLY)) < 0)
		return EXIT_FAILURE;

	do {
		sscanf(bp, "%x", &pid);

		pids[demuxfd_count] = pid;

		if ((demuxfd[demuxfd_count] = setPesFilter(pid)) < 0)
			break;

		demuxfd_count++;
	}
	while ((bp = strchr(bp, ',')) && (bp++) && (demuxfd_count < MAXPIDS));



	/* convert transport stream and write to stdout */
	if (((mode == 0) || (mode == 1)) && (demuxfd_count == 2))
		dvr_to_ps(dvrfd, pids[0], pids[1], mode);

	/* write raw transport stream to stdout */
	else if (mode == 2) {

		uint8_t first = 1;
		uint8_t offset;

		ssize_t pos;
		ssize_t r = 0;
		ssize_t todo;

		while (!exit_flag) {

			offset = 0;
			pos = 0;
			todo = BSIZE;

			while ((!exit_flag) && (todo)) {

				r = read(dvrfd, buffer + pos, todo);

				switch (r) {
				case -1:
					exit_flag = 1;
					first = 0;
					r = 0;
					break;

				case 0:
					continue;

				default:
					pos += r;
					todo -= r;
					break;
				}
			}

			/* make sure to start with a ts header */
			if (first) {

				for (; offset < TS_SIZE; offset++) {
					if (buffer[offset] == 0x47)
						break;
				}

				first = 0;
			}

			if (write(STDOUT_FILENO, buffer + offset, r - offset) != r - offset)
				break;
		}
	}

	while (demuxfd_count > 0)
		close(demuxfd[--demuxfd_count]);

	close(dvrfd);

	return EXIT_SUCCESS;
}

