/*
 * $Id: test_clip_teletext.c,v 1.1 2003/11/30 00:05:28 carjay Exp $
 *
 * (C) 2003 Andreas Oberritter <obi@tuxbox.org>
 *	- slightly modified for teletext by Carsten Juttner <carjay@gmx.net>
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
#include <linux/dvb/dmx.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ADAP	"/dev/dvb/adapter0"
#define DMX	ADAP "/demux0"
#define DVR	ADAP "/dvr0"

int main(int argc, char **argv)
{
	unsigned char buf[384 * 188];
	unsigned char *tsfilename;
	unsigned short pidt;
	int dmxt, dvr, ts;
	struct dmx_pes_filter_params p;
	ssize_t wr;
	size_t r;
	int done;

	if (argc < 3) {
		printf("usage: %s <filename> <teletext pid>\n", argv[0]);
		return 1;
	}

	tsfilename = argv[1];
	pidt = strtoul(argv[2], NULL, 0) & 0x1fff;

	if ((dmxt = open(DMX, O_RDWR)) < 0) {
		perror(DMX);
		return 1;
	}

	if ((dvr = open(DVR, O_WRONLY)) < 0) {
		perror(DVR);
		return 1;
	}

	if ((ts = open(tsfilename, O_RDONLY)) < 0) {
		perror(tsfilename);
		return 1;
	}

	p.pid = pidt;
	p.input = DMX_IN_DVR;
	p.output = DMX_OUT_DECODER;
	p.pes_type = DMX_PES_TELETEXT;
	p.flags = DMX_IMMEDIATE_START;

	if (ioctl(dmxt, DMX_SET_PES_FILTER, &p) < 0) {
		perror("DMX_SET_PES_FILTER");
		return 1;
	}

	while ((r = read(ts, buf, sizeof(buf))) > 0) {
		done = 0;
		while (r > 0) {
			if ((wr = write(dvr, &buf[done], r)) <= 0)
				continue;
			r -= wr;
			done += wr;
		}
	}

	ioctl(dmxt, DMX_STOP);

	close(ts);
	close(dvr);
	close(dmxt);

	return 0;
}

