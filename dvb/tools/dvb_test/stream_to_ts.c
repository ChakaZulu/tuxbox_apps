/*
 * $Id: stream_to_ts.c,v 1.2 2003/09/11 23:08:39 obi Exp $
 *
 * (C) 2003 gagga
 * Parts by obi
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
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ADAP	"/dev/dvb/adapter0"
#define ADEC	ADAP "/audio0"
#define VDEC	ADAP "/video0"
#define DMX	ADAP "/demux0"
#define DVR	ADAP "/dvr0"

int main(int argc, char **argv)
{
	unsigned short port;
	char *tsfilename;
	int ts;

	if (argc < 3) {
		printf("usage: %s <port> <filename>\n", argv[0]);
		return 1;
	}
	port = strtoul(argv[1], NULL, 0) & 0x1fff;
	tsfilename = argv[2];

        int skt_serv;
        struct sockaddr_in sin;

        // Open Socket
        skt_serv = socket( PF_INET, SOCK_DGRAM, 0);
        if (  skt_serv < 0 ) {
                fprintf( stderr, "Cannot open SOCKET.\n");
                exit(-1);
        }

        // Setup socket
        memset( &sin, 0, sizeof( sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
        sin.sin_port = htons( port);
        if ( bind( skt_serv, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
                fprintf( stderr, "Cannot bind SOCKET for port %d.\n", port);
                shutdown( skt_serv, 2);
                exit(-1);
        }

        if ((ts = open(tsfilename, O_WRONLY || O_CREAT)) < 0) {
		perror(tsfilename);
		return 1;
	}
        // Read Datagrams
	unsigned char buf[384 * 188];
        int done;
        int wr;
        
        for ( ;;) {
            int len;
            len = recv( skt_serv, buf, 384 * 188, 0);
            if (len < 0) {
               perror( "recv()");
               break;
            }
            //printf("[%d bytes received]\n", len);
		done = 0;
		while (len > 0) {
			if ((wr = write(ts, &buf[done], len)) <= 0)
				continue;
                        printf("[%d bytes written]\n", wr);
			len -= wr;
			done += wr;
		}

        }


	close(ts);

	return 0;
}

