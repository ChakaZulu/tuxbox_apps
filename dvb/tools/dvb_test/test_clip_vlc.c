/*
 * $Id: test_clip_vlc.c,v 1.1 2003/07/21 20:31:51 gagga Exp $
 *
 * (C) 2003 gagga
 *
 * Start VLC on the server like this:
 * vlc -vvv c:\jones.mpg --sout "#standard{access=http,mux=ts,url=:8080}"
 *
 * Then run test_clip_vls like this:
 * ./test_clip_vls http 192.168.15.177 8080 0x44 0x45
 * VLC always uses vpid 0x44 and apid 0x45 while transcoding
 * 
 * Use VLC 0.6.1-test1 at least!
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
#include <poll.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ADAP	"/dev/dvb/adapter0"
#define ADEC	ADAP "/audio0"
#define VDEC	ADAP "/video0"
#define DMX	ADAP "/demux0"
#define DVR	ADAP "/dvr0"

#define NORMAL_DATA 1
#define HIPRI_DATA 2

#include "ringbuffer.h"
#define RINGBUFFERSIZE 348*188*10
#define MAXREADSIZE 348*188

#define STREAMTYPE_HTTP 1
#define STREAMTYPE_UDP  2

ringbuffer_t *ringbuf;
int streamtype;
char *server;
unsigned short port;
int firstbufferfilled;

void *
Read_Thread ()
{
  int skt_serv;
  if (streamtype == STREAMTYPE_UDP)
    {
// Create Serversockets

      struct sockaddr_in sin;
      // Open Socket
      skt_serv = socket (PF_INET, SOCK_DGRAM, 0);
      if (skt_serv < 0)
	{
	  fprintf (stderr, "Cannot open SOCKET.\n");
	  exit (-1);
	}

      // Setup socket
      memset (&sin, 0, sizeof (sin));
      sin.sin_family = AF_INET;
      sin.sin_addr.s_addr = htonl (INADDR_ANY);
      sin.sin_port = htons (port);
      if (bind (skt_serv, (struct sockaddr *) &sin, sizeof (sin)) < 0)
	{
	  fprintf (stderr, "Cannot bind SOCKET for port %d.\n", port);
	  shutdown (skt_serv, 2);
	  exit (-1);
	}
      fcntl (skt_serv, O_NONBLOCK);
    }
  else
    {


      // Open HTTP connection to VLC
      struct sockaddr_in servAddr;
      int ts_socket;
      printf ("Trying to call socket\n");
      ts_socket = socket (AF_INET, SOCK_STREAM, 0);
      printf ("Server: %s\n", server);
      printf ("Port: %d\n", port);

      servAddr.sin_family = AF_INET;
      servAddr.sin_port = htons (port);
      servAddr.sin_addr.s_addr = inet_addr (server);
      int ts;

      printf ("Trying to setup socket\n");
      ts =
	connect (ts_socket, (struct sockaddr *) &servAddr, sizeof (servAddr));
      if (ts < 0)
	{
	  perror ("SOCKET");
	  exit (-1);
	}
      fcntl (ts_socket, O_NONBLOCK);
      printf ("Socket OK\n");

      char *msg = "GET / HTTP/1.0\r\n\r\n";
      int len = strlen (msg);
      if (send (ts_socket, msg, len, 0) == -1)
	{
	  perror ("send()");
	  exit (-1);
	}

      printf ("GET Sent\n");

      unsigned char buf[RINGBUFFERSIZE];

      // Skip HTTP Header
      int found = 0;
      while (found < 4)
	{
	  len = recv (ts_socket, buf, 1, 0);
	  if ((found == 0) & (buf[0] == '\r'))
	    {
	      found++;
	    }
	  else if ((found == 1) & (buf[0] == '\n'))
	    {
	      found++;
	    }
	  else if ((found == 2) & (buf[0] == '\r'))
	    {
	      found++;
	    }
	  else if ((found == 3) & (buf[0] == '\n'))
	    {
	      found++;
	    }
	  else
	    {
	      (found = 0);
	    }
	  //printf("HTTP header skip: %s\n",buf);
	}
      skt_serv = ts_socket;

    }
  printf ("Read sockets created\n");
  unsigned char buf[RINGBUFFERSIZE];
  int len;
  int done;
  int size;
  for (;;)
    {
      while ((size = ringbuffer_write_space (ringbuf)) == 0)
	{
		firstbufferfilled=1;
	}
      //printf("ringbuf write space:%d\n",size);
      len = recv (skt_serv, buf, size, 0);
      //printf("bytes received:%d\n",len);
      while (len > 0)
	{
	  done = ringbuffer_write (ringbuf, buf, len);
	  len -= done;
	}

    }

}


int
main (int argc, char **argv)
{
//  unsigned short port;
  unsigned short pida, pidv;
  firstbufferfilled = 0;

  if (argc < 6)
    {
      printf ("usage: %s http|udp <server> <port> <pidv> <pida>\n", argv[0]);
      return 1;
    }

  const char *type = argv[1];
  server = argv[2];
  port = strtoul (argv[3], NULL, 0) & 0x1fff;
  pidv = strtoul (argv[4], NULL, 0) & 0x1fff;
  pida = strtoul (argv[5], NULL, 0) & 0x1fff;
  if (strcmp (type, "http") == 0)
    {
      streamtype = STREAMTYPE_HTTP;
    }
  else if (strcmp (type, "udp") == 0)
    {
      streamtype = STREAMTYPE_UDP;
    }
  else
    {
      printf ("usage: %s http|udp <server> <port> <pidv> <pida>\n", argv[0]);
      return 1;
    }

  ringbuf = ringbuffer_create (RINGBUFFERSIZE);
  printf ("ringbuffer created\n");

  // Setup drivers
  int dmxa, dmxv, dvr, adec, vdec, ts;
  struct dmx_pes_filter_params p;
  ssize_t wr;

  pthread_t rct;
  pthread_create (&rct, 0, Read_Thread, (void *) ringbuf);

  if ((dmxa = open (DMX, O_RDWR | O_NONBLOCK)) < 0)
    {
      perror (DMX);
      return 1;
    }

  if ((dmxv = open (DMX, O_RDWR | O_NONBLOCK)) < 0)
    {
      perror (DMX);
      return 1;
    }

  if ((dvr = open (DVR, O_WRONLY | O_NONBLOCK)) < 0)
    {
      perror (DVR);
      return 1;
    }

  if ((adec = open (ADEC, O_RDWR | O_NONBLOCK)) < 0)
    {
      perror (ADEC);
      return 1;
    }

  if ((vdec = open (VDEC, O_RDWR | O_NONBLOCK)) < 0)
    {
      perror (VDEC);
      return 1;
    }
  p.pid = pida;
  p.input = DMX_IN_DVR;
  p.output = DMX_OUT_DECODER;
  p.pes_type = DMX_PES_AUDIO;
  p.flags = DMX_IMMEDIATE_START;

  if (ioctl (dmxa, DMX_SET_PES_FILTER, &p) < 0)
    {
      perror ("DMX_SET_PES_FILTER");
      return 1;
    }

  p.pid = pidv;
  p.input = DMX_IN_DVR;
  p.output = DMX_OUT_DECODER;
  p.pes_type = DMX_PES_VIDEO;
  p.flags = DMX_IMMEDIATE_START;

  if (ioctl (dmxv, DMX_SET_PES_FILTER, &p) < 0)
    {
      perror ("DMX_SET_PES_FILTER");
      return 1;
    }

  if (ioctl (adec, AUDIO_PLAY) < 0)
    {
      perror ("AUDIO_PLAY");
      return 1;
    }

  if (ioctl (vdec, VIDEO_PLAY) < 0)
    {
      perror ("VIDEO_PLAY");
      return 1;
    }

  unsigned char buf[348 * 188];
  int done;

  printf ("read starting\n");
  size_t readsize, len;
  len=0;
  for (;;)
    {
      readsize = ringbuffer_read_space (ringbuf);
      if (readsize > MAXREADSIZE)
	{
	  readsize = MAXREADSIZE;
	}
      //printf("readsize=%d\n",readsize);
      if (firstbufferfilled>0) {
        len = ringbuffer_read (ringbuf, buf, (readsize / 188) * 188);

        //printf ("[%d bytes read from ringbuf]\n", len);
        done = 0;
        while (len > 0)
	{
	  wr = write (dvr, &buf[done], len);
	  //printf ("[%d bytes written]\n", wr);
	  len -= wr;
	  done += wr;
	}
      }
    }

  ioctl (vdec, VIDEO_STOP);
  ioctl (adec, AUDIO_STOP);
  ioctl (dmxv, DMX_STOP);
  ioctl (dmxa, DMX_STOP);

  close (ts);
  close (vdec);
  close (adec);
  close (dvr);
  close (dmxv);
  close (dmxa);

  return 0;
}
