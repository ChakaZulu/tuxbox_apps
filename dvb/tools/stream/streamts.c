/*
 * $Id: streamts.c,v 1.3 2002/03/06 05:36:31 obi Exp $
 *
 * inetd style daemon for streaming transport streams
 *
 * usage:
 * wget http://dbox:streamts/pid0,pid1,pid2,pid3 ...
 * (where every pid must consist of 4 characters and is hex)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/poll.h>
#include <errno.h>

#include <ost/dmx.h>
#include <ost/frontend.h>
#include <ost/sec.h>
#include <ost/video.h>

#define BSIZE	1024 * 16
#define MAXPIDS	16
#define DMXDEV	"/dev/ost/demux0"
#define DVRDEV	"/dev/ost/dvr0"

int setPesFilter (int pid, dmxPesType_t type)
{
	int fd;
	struct dmxPesFilterParams flt; 

	//printf("pid %x\n", pid);
	//printf("type %x\n", type);

	switch(type)
	{
	case DMX_PES_AUDIO:
	case DMX_PES_VIDEO:
	case DMX_PES_TELETEXT:
	case DMX_PES_SUBTITLE:
	case DMX_PES_PCR:
	case DMX_PES_OTHER:
		break;
	default:
		return -1;
	}

	fd=open(DMXDEV, O_RDWR);
	if (fd < 0)
	{
		perror(DMXDEV);
		return -1;
	}
	ioctl(fd, DMX_SET_BUFFER_SIZE, 1024 * 1024);

	flt.pid = pid;
	flt.input = DMX_IN_FRONTEND;
	flt.output = DMX_OUT_TS_TAP;
	flt.pesType = type;
	flt.flags = 0;

	if (ioctl(fd, DMX_SET_PES_FILTER, &flt) < 0)
	{
		perror("DMX_SET_PES_FILTER");
		return -1;
	}
	if (ioctl(fd, DMX_START, 0) < 0)
	{
		perror("DMX_START");
		return -1;
	}

	return fd;
}

int main(int argc, char **argv)
{
	int dvrfd;
	int pid;
	dmxPesType_t type;	/* 0..6 */
	char buffer[BSIZE];
	char *bp;

	int demuxfd[MAXPIDS];
	int demuxfd_count = 0;

	bp = buffer;
	while (bp - buffer < BSIZE)
	{
		unsigned char c;
		read(1, &c, 1);
		if ((*bp++ = c) == '\n')
			break;
	}
	*bp++ = 0;

	bp = buffer;
	if (!strncmp(buffer, "GET /", 5))
	{
		printf("HTTP/1.1 200 OK\r\nServer: d-Box network\r\n\r\n");
		bp += 5;
	}
	fflush(stdout);

	dvrfd = open(DVRDEV, O_RDONLY);
	if (dvrfd < 0)
	{
		perror(DVRDEV);
		return -dvrfd;
	}

	/* pids shall be each 4 bytes, comma separated */
	/* a better solution might be to specify the pes type to allow watching tv while streaming(?) */
	do
	{
		type = DMX_PES_OTHER;
		sscanf(bp, "%x", &pid);
		bp += 4;

		//sscanf(bp, "%x-%x", &type, &pid);
		//bp += 6;

		if ((demuxfd[demuxfd_count] = setPesFilter(pid, type)) < 0)
			return demuxfd[demuxfd_count];
		else
			demuxfd_count++;
	}
	while ((*bp++ == ',') && (demuxfd_count < MAXPIDS));

	while (1)
	{
		int pr = 0;
		int r;
		int tr = BSIZE;

		while (tr)
		{
			if ((r = read(dvrfd, buffer + pr, tr)) <= 0)
				continue;
			pr += r;
			tr -= r;
		}

		if (write(1, buffer, r) != r)
		{
			perror("write");
			break;
		}
	}

	while (demuxfd_count > 0)
		close(demuxfd[--demuxfd_count]);

	close(dvrfd);
	return 0;
}

