/*
 * $Id: test_clip_pes.c,v 1.1 2003/05/24 07:17:14 obi Exp $
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

#include <errno.h>
#include <fcntl.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/video.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ADAP	"/dev/dvb/adapter0"
#define ADEC	ADAP "/audio0"
#define VDEC	ADAP "/video0"

int main(int argc, char **argv)
{
	struct pollfd pfd[2];
	unsigned char abuf[188 * 188], vbuf[188 * 188];
	unsigned char *afilename, *vfilename;
	int adec, vdec, afile, vfile;
	ssize_t awr, vwr;
	size_t ar = 0, vr = 0;
	int adone = 0, vdone = 0;
	unsigned int acaps, vcaps;

	if (argc != 3) {
		printf("usage: %s <audio file> <video file>\n", argv[0]);
		return 1;
	}

	afilename = argv[1];
	vfilename = argv[2];

	if ((adec = open(ADEC, O_WRONLY | O_NONBLOCK)) < 0) {
		perror(ADEC);
		return 1;
	}

	if ((vdec = open(VDEC, O_WRONLY)) < 0) {
		perror(VDEC);
		return 1;
	}

	if ((afile = open(afilename, O_RDONLY)) < 0) {
		perror(afilename);
		return 1;
	}

	if ((vfile = open(vfilename, O_RDONLY)) < 0) {
		perror(vfilename);
		return 1;
	}

	if (ioctl(adec, AUDIO_GET_CAPABILITIES, &acaps) < 0) {
		perror("AUDIO_GET_CAPABILITIES");
		return 1;
	}

	if (ioctl(vdec, VIDEO_GET_CAPABILITIES, &vcaps) < 0) {
		perror("VIDEO_GET_CAPABILITIES");
		return 1;
	}

	if (!(acaps & AUDIO_CAP_MP2)) {
		fprintf(stderr, "audio decoder does not support mpeg2 pes\n");
		return 1;
	}

	if (!(vcaps & VIDEO_CAP_MPEG2)) {
		fprintf(stderr, "video decoder does not support mpeg2 pes\n");
		return 1;
	}

	if (ioctl(adec, AUDIO_SELECT_SOURCE, AUDIO_SOURCE_MEMORY) < 0) {
		perror("AUDIO_SELECT_SOURCE");
		return 1;
	}

	if (ioctl(vdec, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_MEMORY) < 0) {
		perror("VIDEO_SELECT_SOURCE");
		return 1;
	}

	if (ioctl(adec, AUDIO_SET_STREAMTYPE, AUDIO_CAP_MP2) < 0) {
		perror("AUDIO_SET_STREAMTYPE");
		return 1;
	}

	if (ioctl(vdec, VIDEO_SET_STREAMTYPE, VIDEO_CAP_MPEG2) < 0) {
		perror("VIDEO_SET_STREAMTYPE");
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

	pfd[0].fd = afile;
	pfd[0].events = POLLOUT;
	pfd[1].fd = vfile;
	pfd[1].events = POLLOUT;

	while (1) {
		if (ar <= 0) {
			if ((ar = read(afile, abuf, sizeof(abuf))) < 0) {
				perror("audio read");
				break;
			}
			adone = 0;
		}
		if (vr <= 0) {
			if ((vr = read(vfile, vbuf, sizeof(vbuf))) < 0) {
				perror("video read");
				break;
			}
			vdone = 0;
		}

		if ((ar == 0) && (vr == 0))
			break;

		if (poll(pfd, 2, 0)) {
			if (pfd[0].revents & POLLOUT) {
				if ((awr = write(adec, &abuf[adone], ar)) < 0) {
					if (errno != EAGAIN)
						perror("audio write");
				}
				else {
					ar -= awr;
					adone += awr;
				}
			}
			if (pfd[1].revents & POLLOUT) {
				if ((vwr = write(vdec, &vbuf[vdone], vr)) < 0) {
					perror("video write");
				}
				else {
					vr -= vwr;
					vdone += vwr;
				}
			}
		}
	}

	ioctl(vdec, VIDEO_STOP);
	ioctl(adec, AUDIO_STOP);
	close(vfile);
	close(afile);
	close(vdec);
	close(adec);

	return 0;
}

