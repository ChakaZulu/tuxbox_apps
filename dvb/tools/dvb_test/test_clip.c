/*
 * $Id: test_clip.c,v 1.1 2003/04/12 06:09:05 obi Exp $
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
#include <sys/mman.h>
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
	unsigned char *tsfilename;
	unsigned short pida, pidv;
	int dmxa, dmxv, dvr, adec, vdec, ts;
	struct dmx_pes_filter_params p;
	struct stat stat;
	void *mm, *ptr;
	ssize_t len, wr;

	if (argc != 4) {
		printf("usage: %s <filename> <video pid> <audio pid>\n", argv[0]);
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

	if (ioctl(adec, AUDIO_PLAY) < 0) {
		perror("AUDIO_PLAY");
		return 1;
	}

	if (ioctl(vdec, VIDEO_PLAY) < 0) {
		perror("VIDEO_PLAY");
		return 1;
	}

	if (fstat(ts, &stat) < 0) {
		perror("fstat");
		return 1;
	}

	if ((mm = mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, ts, 0)) == NULL) {
		perror("mmap");
		return 1;
	}

	ptr = mm;
	len = stat.st_size;

	while (len > 0) {

		if ((wr = write(dvr, ptr, len)) < 0) {
			perror("write");
			break;
		}

		len -= wr;
		ptr += wr;
	}

	munmap(mm, stat.st_size);

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

