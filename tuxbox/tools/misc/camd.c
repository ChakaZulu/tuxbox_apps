/*
 * $Id: camd.c,v 1.23 2003/08/06 00:56:30 obi Exp $
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

int send_to_camd(unsigned char *buf, size_t len)
{
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

int main(int argc, char **argv)
{
	size_t length = 0;
	unsigned char in_buf[8192];
	unsigned char out_buf[4096];
	unsigned int i, j = 0;
	unsigned int id;
	unsigned short desc_len;

	if ((argc < 4) || (argc > 5)) {
		fprintf(stderr, "usage: %s <vpid> <apid> <sid> [cadescriptors]\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* convert ascii to hex */
	if (argc == 5)
		while (length < (strlen(argv[4]) >> 1)) {
			sscanf(argv[4] + (length << 1), "%2hhx", &in_buf[length]);
			length++;
		}

	out_buf[j++] = 0x9f;
	out_buf[j++] = 0x80;
	out_buf[j++] = 0x32;
	out_buf[j++] = 0x82; /* length_field */
	out_buf[j++] = 0x00; /* length_field */
	out_buf[j++] = 0x00; /* length_field */
	out_buf[j++] = 0x03; /* ca_pmt_list_management */

	/* service id */
	id = strtoul(argv[3], 0, 16);
	out_buf[j++] = id >> 8;
	out_buf[j++] = id;

	out_buf[j++] = 0x00;
	out_buf[j++] = 0x00; /* program_info_length [11:8] */
	out_buf[j++] = 0x00; /* program_info_length [7:0] */
	out_buf[j++] = 0x01; /* ca_pmt_cmd_id */

	for (i = 0; i < length; i += desc_len) {
		desc_len = in_buf[i + 1] + 2;
		if (in_buf[i] == 0x09) {
			memcpy(&out_buf[j], &in_buf[i], desc_len);
			j += desc_len;
		}
	}

	/* program_info_length */
	out_buf[10] = ((j - 12) >> 8) & 0xff;
	out_buf[11] = (j - 12) & 0xff;

	/* video pid */
	out_buf[j++] = 0x02;
	id = strtoul(argv[1], 0, 16);
	out_buf[j++] = id >> 8;
	out_buf[j++] = id;
	out_buf[j++] = 0x00;
	out_buf[j++] = 0x00;

	/* audio pid */
	out_buf[j++] = 0x04;
	id = strtoul(argv[2], 0, 16);
	out_buf[j++] = id >> 8;
	out_buf[j++] = id;
	out_buf[j++] = 0x00;
	out_buf[j++] = 0x00;

	/* length_field */
	out_buf[4] = ((j - 6) >> 8) & 0xff;
	out_buf[5] = (j - 6) & 0xff;

	return send_to_camd(out_buf, j);
}

