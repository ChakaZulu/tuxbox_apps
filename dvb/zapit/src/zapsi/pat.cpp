#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <string>
#include "sdt.h"
#include <ost/dmx.h>
#include <sys/poll.h>

#define DEMUX_DEV "/dev/ost/demux0"

using namespace std;

map<uint,channel> nvodchannels;
map<uint,channel>::iterator cI;

int pat(uint oonid)
{
	struct dmxSctFilterParams flt;
	int demux, pt;
	struct pollfd dmx_fd;
	
	demux=open(DEMUX_DEV, O_RDWR);
  	if (demux<0) {
    		perror("/dev/ost/demux0");
    		exit(0);
  	}

  	memset (&flt.filter, 0, sizeof (struct dmxFilter));
  
  	flt.pid=0;
    	flt.filter.mask[0]  =0xFF;
  	flt.timeout=5000;
  	flt.flags=DMX_IMMEDIATE_START | DMX_CHECK_CRC;
  	
  	if (ioctl(demux, DMX_SET_FILTER, &flt)<0)  {
    		perror("DMX_SET_FILTER");
  		}
  
  	ioctl(demux, DMX_START, 0);
  
  	dmx_fd.fd = demux;
  	dmx_fd.events = POLLIN;
  	dmx_fd.revents = 0;
  
  	pt = poll(&dmx_fd, 1, 500);

  	if (!pt)
  	{
  		printf("Poll Timeout\n");
  		return 0;
  	}
  	else
  	{
  		char buffer[1024];
  		int r;
  		int current, sec_len;
  		int tsid;
  		
		if ((r=read(demux, buffer, 3))<=0)  {
   			perror("read");
    			close(demux);
    			exit(0);
    		}
    		
  		sec_len = (((buffer[1]&0xF)<<8) + buffer[2]);

    		if ((r=read(demux, buffer+3, sec_len))<=0)  {
    			perror("read");
    			close(demux);
    			exit(0);
		}
		
		close(demux);
		
		tsid = (buffer[3]<<8)|buffer[4];
		printf("TSID: %04x\n", tsid);
		current = 8;
		
		while (current < sec_len-1)
		{
			int p_nr = (buffer[current]<<8) | buffer[current+1];
			int pid = ((buffer[current+2]&0x1f)<<8) | buffer[current+3];
			printf("P-Nr: %04x: %04x\n",p_nr,pid);
			if (nvodchannels.count((oonid<<16)|p_nr) >0)
			{
			  cI = nvodchannels.find((oonid<<16)|p_nr);
			  cI->second.pmt = pid;
			  printf("found p_nr: %04x\npmt: %04x\n", p_nr, pid);
			}
			else
			{
				
				
			  /*	if (channels.count((tsid<<16)+p_nr) == 0)
				{
					channels.insert(std::pair<int,channel>((tsid<<16)+p_nr,channel(p_nr,tsid,pid)));
				}
				else
				{
					citerator I = channels.find((tsid<<16)+p_nr);
					
					I->second.pmt = pid;
				}
			  */
			}
			
			current = current+4;
		}
	}
	close(demux);
	return 1;
}
