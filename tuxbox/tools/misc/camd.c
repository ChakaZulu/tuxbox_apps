/*
 * $Id: camd.c,v 1.21 2003/01/18 03:13:50 obi Exp $
 *
 * (C) 2002, 2003 by Andreas Oberritter <obi@tuxbox.org>
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
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int send_to_camd(unsigned char *buf, size_t len) {

	struct sockaddr_un servaddr;
	int clilen;
	int sock;

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, "/tmp/camd.socket");
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	sock = socket(PF_UNIX, SOCK_STREAM, 0);
	connect(sock, (struct sockaddr *) &servaddr, clilen);

	if (write(sock, buf, len) == len)
		return EXIT_SUCCESS;

	return EXIT_FAILURE;
}

int main(int argc, char **argv) {

	size_t length;
	unsigned char in_buf[2048];
	unsigned char out_buf[2048];
	unsigned int i, j = 0;
	unsigned int pid;

	if (argc != 5) {
		fprintf(stderr, "usage: %s <vpid> <apid> <pmtpid> <cadescriptors>\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* convert ascii to hex */
	for (length = 0; length < (strlen(argv[4]) >> 1); length++)
		sscanf(argv[4] + (length << 1), "%2hhx", &in_buf[length]);

	out_buf[j++] = 0x9f;
	out_buf[j++] = 0x80;
	out_buf[j++] = 0x32;
	out_buf[j++] = 0x00; /* length_field */
	out_buf[j++] = 0x03; /* ca_pmt_list_management */
	out_buf[j++] = 0x00; /* service_id [15:8] */
	out_buf[j++] = 0x00; /* service_id [7:0] */
	out_buf[j++] = 0x00;
	out_buf[j++] = 0x00; /* program_info_length [11:8] */
	out_buf[j++] = 0x00; /* program_info_length [7:0] */
	out_buf[j++] = 0x01; /* ca_pmt_cmd_id */

	for (i = 0; i < length; i += in_buf[i + 1] + 2) {
		if (in_buf[i] == 0x09) {
			out_buf[j++] = 0x09;
			out_buf[j++] = 0x04;
			out_buf[j++] = in_buf[i + 2];
			out_buf[j++] = in_buf[i + 3];
			out_buf[j++] = in_buf[i + 4];
			out_buf[j++] = in_buf[i + 5];
		}
	}

	/* program_info_length */
	out_buf[9] = j - 10;

	/* video pid */
	out_buf[j++] = 0x02;
	pid = strtoul(argv[1], 0, 16);
	out_buf[j++] = pid >> 8;
	out_buf[j++] = pid;
	out_buf[j++] = 0x00;
	out_buf[j++] = 0x00;

	/* audio pid */
	out_buf[j++] = 0x04;
	pid = strtoul(argv[2], 0, 16);
	out_buf[j++] = pid >> 8;
	out_buf[j++] = pid;
	out_buf[j++] = 0x00;
	out_buf[j++] = 0x00;

	/* length_field */
	out_buf[3] = j - 4;

	return send_to_camd(out_buf, j);
}

