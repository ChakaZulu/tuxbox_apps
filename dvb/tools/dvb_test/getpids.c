/*
 * $Id: getpids.c,v 1.3 2003/12/20 04:35:25 obi Exp $
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
#include <linux/dvb/dmx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>

static const int dflt_flt_cnt = 28;
static const int dflt_pid = 0;
static const int dflt_timeout = 200;
static const int dflt_verbose = 0;

static __u8 seen_pids[0x2000];

static void analyze_ts(const __u8 *buf, size_t len)
{
	size_t i;
	__u16 pid;

	for (i = 0; i < len; i += 188) {
		if (buf[i] == 0x47) {
			pid = (*(__u16 *)&buf[i + 1]) & 0x1fff;
			if (seen_pids[pid] == 0) {
				printf("pid %04x\n", pid);
				seen_pids[pid] = 1;
			}
		}
		else {
			i++;
		}
	}
}

static void usage(char *name)
{
	printf("usage: %s [-h] [-t <timeout>] [-p <pid>] [-f <flt_cnt>] [-v]\n", name);
	printf("   defaults:\n");
	printf("       timeout=%d       timeout in ms for each block of pid filters\n", dflt_timeout);
	printf("       pid=0x%04x        pid to start with\n", dflt_pid);
	printf("       flt_cnt=%d        number of pid filters to use simultaneously\n", dflt_flt_cnt);
	printf("   add -v for verbose output\n");
	printf("\n");
	printf("   for quick results try -t 50 -f 28\n");
	printf("   more exact results may be obtained with e.g. -t 100 -f 1\n");
}

int main(int argc, char **argv)
{
	extern char *optarg;
	int opt;

	struct dmx_pes_filter_params flt;
	unsigned char buf[188 * 1024];
	struct pollfd pfd;
	int *dmxfd;
	ssize_t r;
	int i;

	int flt_cnt = dflt_flt_cnt;
	int pid = dflt_pid;
	int timeout = dflt_timeout;
	int verbose = dflt_verbose;

	while ((opt = getopt(argc, argv, "f:hp:t:v")) != -1) {
		switch (opt) {
		case 'f':	/* filters */
			flt_cnt = strtol(optarg, NULL, 0);
			break;
		case 'h':	/* help */
			usage(argv[0]);
			return EXIT_SUCCESS;
		case 'p':	/* pid */
			pid = strtol(optarg, NULL, 0);
			break;
		case 't':	/* timeout */
			timeout = strtol(optarg, NULL, 0);
			break;
		case 'v':	/* verbose */
			verbose = 1;
			break;
		default:
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	dmxfd = malloc(sizeof(int) * flt_cnt);
	if (!dmxfd) {
		perror("malloc");
		return EXIT_FAILURE;
	}

	for (i = 0; i < flt_cnt; i++)
		dmxfd[i] = -1;

	memset(seen_pids, 0, sizeof(seen_pids));

	while (pid < 0x2000) {
		pfd.events = POLLIN | POLLPRI;
		pfd.fd = open("/dev/dvb/adapter0/dvr0", O_RDONLY | O_NONBLOCK);
		if (pfd.fd == -1) {
			perror("/dev/dvb/adapter0/dvr0");
			break;
		}
		for (i = 0; (i < flt_cnt) && (pid < 0x2000); i++) {
			if (dmxfd[i] == -1) {
				dmxfd[i] = open("/dev/dvb/adapter0/demux0", O_RDWR);
				if (dmxfd[i] == -1) {
					if (errno == EMFILE) {
						printf("note: reducing number of filters from %d to %d\n", flt_cnt, i);
						flt_cnt = i;
					}
					else {
						perror("/dev/dvb/adapter0/demux0");
					}
					break;
				}
			}
			flt.pid = pid++;
			flt.input = DMX_IN_FRONTEND;
			flt.output = DMX_OUT_TS_TAP;
			flt.pes_type = DMX_PES_OTHER;
			flt.flags = DMX_IMMEDIATE_START;
			if (ioctl(dmxfd[i], DMX_SET_PES_FILTER, &flt) < 0) {
				perror("DMX_SET_PES_FILTER");
				break;
			}
		}

		if (verbose)
			printf("range %04x to %04x\n", pid - i, pid - 1);

		if (poll(&pfd, 1, timeout) > 0) {
			if (pfd.revents & POLLIN) {
				r = read(pfd.fd, buf, sizeof(buf));
				if (r >= 188)
					analyze_ts(buf, r);
			}
		}

		for (i = 0; i < flt_cnt; i++)
			if (dmxfd[i] != -1)
				if (ioctl(dmxfd[i], DMX_STOP) < 0)
					perror("DMX_STOP");

		close(pfd.fd);
	}

	for (i = 0; i < flt_cnt; i++)
		close(dmxfd[i]);

	free(dmxfd);

	return EXIT_SUCCESS;
}

