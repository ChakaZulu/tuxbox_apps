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
#include <string.h>

#include <ost/dmx.h>
#include <ost/frontend.h>
#include <ost/sec.h>
#include <ost/video.h>

#define BSIZE					 10000

int main(int argc, char **argv)
{
	int fd, pid,filter,mask;
	struct dmxSctFilterParams flt; 
	char buffer[BSIZE], *bp;

	memset(&flt.filter.filter,0,DMX_FILTER_SIZE);
	memset(&flt.filter.mask,0,DMX_FILTER_SIZE);
	
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
	
	fd=open("/dev/dvb/card0/demux0", O_RDONLY);
	if (fd<0)
	{
		perror("/dev/dvb/card0/demux0");
		return -fd;
	}
	
	sscanf(bp, "%4x %2x %2x", &pid,&filter,&mask);
	//sscanf(bp+5, "%x", &filter);
	
	printf("pid: %x\n", pid);
	printf("filter: %x\n",filter);
	printf("mask: %x\n",mask);
	
	flt.pid=pid;
	flt.filter.filter[0]=filter;
	flt.filter.mask[0]=mask;
	flt.timeout=10000;
	flt.flags=DMX_IMMEDIATE_START;
		
	
	if (ioctl(fd, DMX_SET_FILTER, &flt)<0)
	{
		perror("DMX_SET_PES_FILTER");
		return errno;
	}

	while (1)
	{
		//int pr=0;
		int r,a;
		//int tr=BSIZE;

		if ((r=read(fd,buffer,3)) <=0) perror("read");
		a=(((buffer[1] & 0xF)<<8) + buffer[2]);
		if ((r=read(fd,buffer+3,a)) <=0) perror("read");
		//for(i=0;i<10;i++)printf("%02x ",buffer[i]);
		//printf("\n");

		if (write(1, buffer, a)!=a)
		{
			perror("output");
			break;
		}
	}
	
	close(fd);
	return 0;
}
