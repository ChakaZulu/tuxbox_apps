/* test_sections.c - Test for section filters.
 * usage: DEMUX=/dev/dvb/adapterX/demuxX test_sections [PID [TID]]
 *
 * Copyright (C) 2002 convergence GmbH
 * Johannes Stezenbach <js@convergence.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <linux/dvb/dmx.h>

#include "hex_dump.h"

#define MAX_SECTION_SIZE 8192


void usage(void)
{
	fprintf(stderr, "usage: test_sections PID [TID]\n");
	fprintf(stderr, "       The default demux device used can be changed\n");
	fprintf(stderr, "       using the DEMUX environment variable;\n");
	fprintf(stderr, "       set env BUFFER to play with DMX_SET_BUFFER_SIZE\n");
	exit(1);
}


void process_section(int fd)
{
	uint8_t buf[MAX_SECTION_SIZE];
	int bytes;

	bytes = read(fd, buf, sizeof(buf));
	if (bytes < 0) {
		perror("read");
		if (errno != EOVERFLOW)
			exit(1);
	}
	hex_dump(buf, bytes);
	printf("\n");
}

int set_filter(int fd, unsigned int pid, unsigned int tid)
{
	struct dmx_sct_filter_params f;
	unsigned long bufsz;

	if (getenv("BUFFER")) {
		bufsz=strtoul(getenv("BUFFER"), NULL, 0);
		if (bufsz > 0 && bufsz <= MAX_SECTION_SIZE) {
			fprintf(stderr, "DMX_SET_BUFFER_SIZE %lu\n", bufsz);
			if (ioctl(fd, DMX_SET_BUFFER_SIZE, bufsz) == -1) {
				perror("DMX_SET_BUFFER_SIZE");
				return 1;
			}
		}
	}
	memset(&f.filter, 0, sizeof(struct dmx_filter));
	f.pid = (uint16_t) pid;
	if (tid < 0x100) {
		f.filter.filter[0] = (uint8_t) tid;
		f.filter.mask[0]   = 0xff;
	}
	f.timeout = 0;
	f.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

	if (ioctl(fd, DMX_SET_FILTER, &f) == -1) {
		perror("DMX_SET_FILTER");
		return 1;
	}
	return 0;
}


int main(int argc, char *argv[])
{
	int dmxfd;
	unsigned long pid, tid;
	char * dmxdev = "/dev/dvb/adapter0/demux0";

	if (argc < 2 || argc > 3)
		usage();

	pid = strtoul(argv[1], NULL, 0);
	if (pid > 0x1fff)
		usage();
	if (argc > 2) {
		tid = strtoul(argv[2], NULL, 0);
		if (tid > 0xff)
			usage();
	} else
		tid = 0x100;

	if (getenv("DEMUX"))
		dmxdev = getenv("DEMUX");
	fprintf(stderr, "test_sections: using '%s'\n", dmxdev);
	fprintf(stderr, "               PID 0x%04lx\n", pid);
	if (tid < 0x100)
		fprintf(stderr, "               TID 0x%02lx\n", tid);

	if ((dmxfd = open(dmxdev, O_RDWR)) < 0){
		perror("open");
		return 1;
	}

	if (set_filter(dmxfd, pid, tid) != 0)
		return 1;

	for (;;) {
		process_section(dmxfd);
	}

	close(dmxfd);
	return 0;
}

