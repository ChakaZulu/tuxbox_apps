/*
 * $Id: test_clip_vls.c,v 1.3 2003/09/11 23:08:40 obi Exp $
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
#include <poll.h>

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


#define TDIRECT        0
#define TPOLL          1

#define TDIRECT_PACKET 0
#define TVLS_PACKET    1
#define TDRIVER_PACKET 2

int
main (int argc, char **argv)
{
  unsigned short port;
  unsigned short pida, pidv;
  unsigned short packettype, type;

  if (argc < 6)
    {
      printf ("usage: %s <udpport> <pidv> <pida> <readtype> <packettype>\n",
	      argv[0]);
      return 1;
    }
  port = strtoul (argv[1], NULL, 0) & 0x1fff;
  pidv = strtoul (argv[2], NULL, 0) & 0x1fff;
  pida = strtoul (argv[3], NULL, 0) & 0x1fff;
  type = strtoul (argv[4], NULL, 0) & 0x1fff;
  packettype = strtoul (argv[5], NULL, 0) & 0x1fff;

  // Create Serversockets
  int skt_serv;
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
  fcntl(skt_serv, O_NONBLOCK);
  
  // Setup drivers
  int dmxa, dmxv, dvr, adec, vdec, ts;
  struct dmx_pes_filter_params p;
  ssize_t wr;

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

  int packetsize = 0;
  // 188 == DIRECT
  // 7 * 188 = VLS Packet Size
  // 348 * 188 = Max. Packetsize for driver
  if (packettype == TDIRECT_PACKET)
    {
      packetsize = 188;
    }
  else if (packettype == TVLS_PACKET)
    {
      packetsize = 7 * 188;
    }
  else if (packettype == TDRIVER_PACKET)
    {
      packetsize = 348 * 188;
    }
  printf ("Using packetsize: %d", packetsize);

  unsigned char buf[384 * 188];
  int done;

  struct pollfd poller[1];
  int retval;

  // Setup poll datastructures
  if (type == TPOLL)
    {
      poller[0].fd = skt_serv;
      poller[0].events = POLLIN | POLLPRI;
      poller[1].fd = dvr;
      poller[1].events = POLLOUT;
    }

  for (;;)
    {
      int len = 0;
//-----------------------------------------------------
      if (type == TPOLL)
	{
	  retval = poll (poller, (unsigned long) 2, -1);

	  if (retval < 0)
	    {
	      fprintf (stderr, "Error while polling\n");
	      return -1;
	    }

	  if (((poller[0].revents & POLLHUP) == POLLHUP) ||
	      ((poller[0].revents & POLLERR) == POLLERR) ||
	      ((poller[0].revents & POLLNVAL) == POLLNVAL))
	    return 0;

	  if ((poller[0].revents & POLLIN) == POLLIN)
	    {
	      len = read (poller[0].fd, buf, packetsize);
	      //printf ("[%d POLLIN bytes read]\n", wr);
	    }
	  if ((poller[0].revents & POLLPRI) == POLLPRI)
	    {
	      len = read (poller[0].fd, buf, packetsize);
	      printf ("[%d POLLIN bytes read]\n", wr);
	    }
	  if ((poller[1].revents & POLLOUT) == POLLOUT)
	    {
	      done = 0;
	      while (len > 0)
		{
		  //if ((wr = write(dvr, &buf[done], len)) <= 0)
		  //      continue;
		  wr = write (dvr, &buf[done], len);
		  //printf ("[%d POLLOUT bytes written]\n", wr);
		  len -= wr;
		  done += wr;
		}

	    }
	}

//-----------------------------------------------------
      if (type == TDIRECT)
	{
	  if (packetsize == 188 || packetsize == 7 * 188)
	    {
	      len = recv (skt_serv, buf, packetsize, 0);
	    }
	  else if (packetsize == 348 * 188)
	    {
	      int j = 0;
	      for (j = 0; j < 49; j++)
		{
		  len += read (skt_serv, &buf[j * 7 * 188], 7 * 188);
		}
	    }
	  if (len < 0)
	    {
	      perror ("recv()");
	      break;
	    }
	  //printf ("[%d bytes received]\n", len);
	  done = 0;
	  while (len > 0)
	    {
	      wr = write (dvr, &buf[done], len);
	      //printf ("[%d bytes written]\n", wr);
	      len -= wr;
	      done += wr;
	    }
	}
//--------------------------------------------------------

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
