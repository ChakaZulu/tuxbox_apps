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

#define BSIZE					 1024*16

int main(int argc, char **argv)
{
	int fd,fd2, pid;
	struct dmxPesFilterParams flt; 
	char buffer[BSIZE], *bp;
	
	bp=buffer;
	while (bp-buffer < BSIZE)
	{
		unsigned char c;
		read(1, &c, 1);
		if ((*bp++=c)=='\n')
			break;
	}
	*bp++=0;
	
	bp=buffer;
	if (!strncmp(buffer, "GET /", 5))
	{
		printf("HTTP/1.1 200 OK\r\nServer: d-Box network\r\n\r\n"); // Content-Type: video/mpeg\r\n\r\n");
		bp+=5;
	}
	fflush(stdout);
	
	fd2=open("/dev/ost/dvr0", O_RDONLY);
	if (fd2<0)
	{
		perror("/dev/ost/dvr0");
		return -fd2;
	}
	
	fd=open("/dev/ost/demux0", O_RDWR);
	if (fd<0)
	{
		perror("/dev/ost/demux0");
		return -fd;
	}
	ioctl(fd, DMX_SET_BUFFER_SIZE, 1024*1024);
	sscanf(bp, "%x", &pid);
	printf("pid: %x\n", pid);
	flt.pid=pid;
	flt.input=DMX_IN_FRONTEND;
	flt.output=DMX_OUT_TS_TAP;
	flt.pesType=DMX_PES_OTHER;
	flt.flags=0;
	printf("pid %x\n", pid);
	if (ioctl(fd, DMX_SET_PES_FILTER, &flt)<0)
	{
		perror("DMX_SET_PES_FILTER");
		return errno;
	}
	if (ioctl(fd, DMX_START, 0)<0)
	{
		perror("DMX_SET_PES_FILTER");
		return errno;
	}

	while (1)
	{
		int pr=0, r;
		int tr=BSIZE;
		while (tr)
		{
			if ((r=read(fd2, buffer+pr, tr))<=0)
				continue;
			pr+=r;
			tr-=r;
		}

		if (write(1, buffer, r)!=r)
		{
			perror("output");
			break;
		}
	}
	
	close(fd);
	return 0;
}
