/*
 * $Id: test_clip.c,v 1.4 2003/05/25 01:41:27 obi Exp $
 *
 * (C) 2003 Andreas Oberritter <obi@tuxbox.org>
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
#include <linux/dvb/audio.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ADAP	"/dev/dvb/adapter0"
#define ADEC	ADAP "/audio0"
#define VDEC	ADAP "/video0"
#define DMX	ADAP "/demux0"
#define DVR	ADAP "/dvr0"

int main(int argc, char **argv)
{
	unsigned char buf[384 * 188];
	unsigned char *tsfilename;
	unsigned short pida, pidv;
	int dmxa, dmxv, dvr, adec, vdec, ts;
	struct dmx_pes_filter_params p;
	ssize_t wr;
	size_t r;
	int done;

	if (argc < 4) {
		printf("usage: %s <filename> <video pid> <audio pid> [<audio bypass>]\n", argv[0]);
		return 1;
	}

	tsfilename = argv[1];
	pidv = strtoul(argv[2], NULL, 0) & 0x1fff;
	pida = strtoul(argv[3], NULL, 0) & 0x1fff;

	if ((dmxa = open(DMX, O_RDWR)) < 0) {
		perror(DMX);
		return 1;
	}

	if ((dmxv = open(DMX, O_RDWR)) < 0) {
		perror(DMX);
		return 1;
	}

	if ((dvr = open(DVR, O_WRONLY)) < 0) {
		perror(DVR);
		return 1;
	}

	if ((adec = open(ADEC, O_RDWR)) < 0) {
		perror(ADEC);
		return 1;
	}

	if ((vdec = open(VDEC, O_RDWR)) < 0) {
		perror(VDEC);
		return 1;
	}

	if ((ts = open(tsfilename, O_RDONLY)) < 0) {
		perror(tsfilename);
		return 1;
	}

	p.pid = pida;
	p.input = DMX_IN_DVR;
	p.output = DMX_OUT_DECODER;
	p.pes_type = DMX_PES_AUDIO;
	p.flags = DMX_IMMEDIATE_START;

	if (ioctl(dmxa, DMX_SET_PES_FILTER, &p) < 0) {
		perror("DMX_SET_PES_FILTER");
		return 1;
	}

	p.pid = pidv;
	p.input = DMX_IN_DVR;
	p.output = DMX_OUT_DECODER;
	p.pes_type = DMX_PES_VIDEO;
	p.flags = DMX_IMMEDIATE_START;

	if (ioctl(dmxv, DMX_SET_PES_FILTER, &p) < 0) {
		perror("DMX_SET_PES_FILTER");
		return 1;
	}

	if (argc >= 5) {
		unsigned long bypass = strtoul(argv[4], NULL, 0);

		if (ioctl(adec, AUDIO_SET_BYPASS_MODE, bypass) < 0) {
			perror("AUDIO_SET_BYPASS_MODE");
			return 1;
		}
	}

	if (ioctl(adec, AUDIO_PLAY) < 0) {
		perror("AUDIO_PLAY");
		return 1;
	}

	if (ioctl(vdec, VIDEO_PLAY) < 0) {
		perror("VIDEO_PLAY");
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

	ioctl(vdec, VIDEO_STOP);
	ioctl(adec, AUDIO_STOP);
	ioctl(dmxv, DMX_STOP);
	ioctl(dmxa, DMX_STOP);

	close(ts);
	close(vdec);
	close(adec);
	close(dvr);
	close(dmxv);
	close(dmxa);

	return 0;
}

