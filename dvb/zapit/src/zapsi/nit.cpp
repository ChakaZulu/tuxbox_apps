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
#include "getservices.h"
#include "descriptors.h"

#define DEMUX_DEV "/dev/ost/demux0"

using namespace std;

void nit(int diseqc, FILE *logfd)
{
	struct dmxSctFilterParams flt;
	struct pollfd dmx_fd;
	int demux,pt;
  	char buffer[1024];
  	int r;
  	int current, sec_len;
  	int step = 0;
  	
  	fprintf(logfd, "Starting NIT\n");
  	while (step < 10)
  	{
  	step++;
  	printf("Reading NIT step %d\n", step);
  	fprintf(logfd, "Reading NIT step %d\n", step);
  	
	demux=open(DEMUX_DEV, O_RDWR);
  	if (demux<0) {
    		perror("/dev/ost/demux0");
    		fprintf(logfd, "Error opening demux\n");
    		return;
  	}

  	memset (&flt.filter, 0, sizeof (struct dmxFilter));
  
  	flt.pid=0x10;
  	flt.filter.filter[0] = 0x40;
    	flt.filter.mask[0]  =0xFF;
  	flt.timeout=10000;
  	flt.flags=DMX_CHECK_CRC;
  	
  	if (ioctl(demux, DMX_SET_FILTER, &flt)<0)  {
    		perror("DMX_SET_FILTER");
  		}
  
  	ioctl(demux, DMX_START, 0);
/*  
  	dmx_fd.fd = demux;
  	dmx_fd.events = POLLIN;
  	dmx_fd.revents = 0;
	
  
  	pt = poll(&dmx_fd, 1, 15000);
  
  	if (!pt)
  	{
		printf("Poll timeout\n");
		close(demux);
		return;
  	}
*/  	
  	
  	if ((r=read(demux, buffer, 3))<=0)  {
   		perror("[zapit] read NIT.");
   		fprintf(logfd, "Error reading first 3 bytes.\n");
    		close(demux);
    		continue;
    	}
    	
  	sec_len = (((buffer[1]&0xF)<<8) + buffer[2]);
  	//printf("Section-length: %d bytes\n", sec_len);
  	
    	if ((r=read(demux, buffer+3, sec_len))<=0)  {
    		perror("[zapit] read NIT.");
    		fprintf(logfd, "Error reading %d bytes.\n",sec_len);
    		close(demux);
    		continue;
	}
	ioctl(demux,DMX_STOP,0);
	close(demux);
	break;
	};

	if (step == 10)
	{
		fprintf(logfd, "Two many steps reading NIT. Cancelling\n");
		return;
	}
	
	/*
	printf("<NIT>\n");
	printf("Network-ID %04x\n", (buffer[3]<<8)|buffer[4]);
	printf("section-number: %04x\n", buffer[6]);
	printf("last section-number: %04x\n", buffer[7]);
	*/
	fprintf(logfd, "Network-ID %04x\n", (buffer[3]<<8)|buffer[4]);
	
	{
		int desc_len, loop_len;
		int tsid = 0;
		
		current = 10;
		desc_len = ((buffer[8]&0xF)<<8)|buffer[9];
		//printf("Desc-len: %d bytes\n", desc_len);
		
		while (current < desc_len+10)
		{
			switch (buffer[current])
			{
				case 0x40:
					current += network_name_desc(&buffer[current],logfd);
					break;
				case 0x41:
					current += service_list_desc(&buffer[current],logfd);
					break;
				case 0x42:
					current += stuffing_desc(&buffer[current],logfd);
					break;
				case 0x43:
					current += sat_deliv_system_desc(&buffer[current],tsid,diseqc,logfd);
					break;
				case 0x44:
					current += cable_deliv_system_desc(&buffer[current],tsid,logfd);
					break;
				case 0x4a:
					current += linkage_desc(&buffer[current],logfd);
					break;
				case 0x5a:
					current += terr_deliv_system_desc(&buffer[current],logfd);
					break;
				case 0x5b:
					current += multilingual_network_name_desc(&buffer[current],logfd);
					break;
				case 0x5f:
					current += priv_data_desc(&buffer[current],logfd);
					break;
				case 0x62:
					current += freq_list_desc(&buffer[current],logfd);
					break;
				case 0x6c:
					current += cell_list_desc(&buffer[current],logfd);
					break;
				case 0x6d:
					current += cell_freq_list_desc(&buffer[current],logfd);
					break;
				case 0x6e:
					current += announcement_support_desc(&buffer[current],logfd);
					break;
				default:
					printf("The descriptor tag was: %02x\n",buffer[current]);
					fprintf(logfd, "Unknown descriptor found: %x\n", buffer[current]);
					current += buffer[current+1]+2;
				}
			}
			//printf("Current: %d\n", current);
			
			loop_len = ((buffer[current]&0xF)<<8)|buffer[++current];
			//printf("loop_len = %d bytes\n", loop_len);
			++current;
			
			while (current < sec_len-3)
			{
				int desc_tot = 0;
				int onid;
				
				tsid = (buffer[current]<<8)|buffer[++current];
				onid = (buffer[++current]<<8)|buffer[++current];
				printf("TS-ID: %x\n",tsid);
				printf("Original network-id: %x\n", onid);
				
				fprintf(logfd, "TS-ID: %x\n",tsid);
				fprintf(logfd, "Original network-id: %x\n", onid);
				
				desc_len = ((buffer[++current]&0xF)<<8)|buffer[++current];
				//printf("transport_descriptors_length = %d bytes\n",desc_len);
				++current;
				
				if (transponders.count(tsid) == 0)
				  {
				    while (desc_tot < desc_len)
				      {
					switch (buffer[current+desc_tot])
					  {
					  case 0x40:
					    desc_tot += network_name_desc(&buffer[current+desc_tot],logfd);
					    break;
					  case 0x41:
					    desc_tot += service_list_desc(&buffer[current+desc_tot],logfd);
					    break;
					  case 0x42:
					    desc_tot += stuffing_desc(&buffer[current+desc_tot],logfd);
					    break;
					  case 0x43:
					    desc_tot += sat_deliv_system_desc(&buffer[current+desc_tot],tsid,diseqc,logfd);
					    break;
					  case 0x44:
					    desc_tot += cable_deliv_system_desc(&buffer[current+desc_tot],tsid,logfd);
					    break;
					  case 0x4a:
					    desc_tot += linkage_desc(&buffer[current+desc_tot],logfd);
					    break;
					  case 0x5a:
					    desc_tot += terr_deliv_system_desc(&buffer[current+desc_tot],logfd);
					    break;
					  case 0x5b:
					    desc_tot += multilingual_network_name_desc(&buffer[current+desc_tot],logfd);
					    break;
					  case 0x5f:
					    desc_tot += priv_data_desc(&buffer[current+desc_tot],logfd);
					    break;
					  case 0x62:
					    desc_tot += freq_list_desc(&buffer[current+desc_tot],logfd);
					    break;
					  case 0x6c:
					    desc_tot += cell_list_desc(&buffer[current+desc_tot],logfd);
					    break;
					  case 0x6d:
					    desc_tot += cell_freq_list_desc(&buffer[current+desc_tot],logfd);
					    break;
					  case 0x6e:
					    desc_tot += announcement_support_desc(&buffer[current+desc_tot],logfd);
					    break;
					  default:
					    printf("The descriptor tag was: %02x\n",buffer[current+desc_tot]);
					    fprintf(logfd, "Unknown descriptor found: %x\n", buffer[current+desc_tot]);
					    current += buffer[current+1+desc_tot]+2;
					  }
					//printf("read %d bytes\n",desc_tot);
				      }
				  }
				current += desc_len;
				
			}
			
	}
	
	//printf("Last buffer is %d\n",current);		
	
	
	
	//printf("</NIT>\n");
	fprintf(logfd, "NIT ended\n");
}
