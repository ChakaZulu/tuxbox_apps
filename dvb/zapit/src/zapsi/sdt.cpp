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
#include <sdt.h>
#include <ost/dmx.h>
#include "descriptors.h"

#define DEMUX_DEV "/dev/ost/demux0"


extern std::string curr_chan_name;

int sdt(uint osid, bool scan_mode)
{
  struct dmxSctFilterParams flt;
  int demux;
  
  demux=open(DEMUX_DEV, O_RDWR);
  if (demux<0) {
    perror("/dev/ost/demux0");
    exit(0);
  }
  
  memset (&flt.filter, 0, sizeof (struct dmxFilter));
  
  flt.pid              = 0x11;
  flt.filter.filter[0] = 0x42;
  flt.filter.mask[0]  =0xFF;
  flt.timeout=10000;
  flt.flags=DMX_IMMEDIATE_START | DMX_CHECK_CRC;
  
  if (ioctl(demux, DMX_SET_FILTER, &flt)<0)  {
    perror("DMX_SET_FILTER");
  }
  
  ioctl(demux, DMX_START, 0);
  
  {
    char buffer[1024];
    int r;
    int current_i, sec_len;
    int network_id, tsid;
    
    if ((r=read(demux, buffer, 3))<=0)  {
      perror("read");
      close(demux);
      return -1;
      //exit(0);
    }
    
    sec_len = (((buffer[1]&0xF)<<8) + buffer[2]);
    
    //printf("The section is %d byte long\n", sec_len + 3);
    
    if ((r=read(demux, buffer+3, sec_len))<=0)  {
      perror("read");
      close(demux);
      //exit(0);
      return -1;
    }
    close(demux);
    
    tsid = (buffer[3]<<8) | buffer[4];
    printf("TSid: %04x\n",tsid);
    if (scan_mode && tsid != osid)
    	return -2;
    //printf("section_number: %04x\n",buffer[6]);
    //printf("last_section_number: %04x\n",buffer[7]);
    network_id = (buffer[8]<<8)|buffer[9];
    printf("network_id: %04x\n",network_id);
    
    current_i = 11;
    while (current_i < sec_len-1)
      {	
	int desc_len, desc_tot = 0;
	int sid;
	
	sid = (buffer[current_i]<<8)|buffer[++current_i];
	
	++current_i;
	desc_len = ((buffer[++current_i]&0xF)<<8) | buffer[++current_i];
	//printf("The descriptors-loop is %d byte long\n", desc_len);
	++current_i;
	
	if (osid == sid || scan_mode)
	{
	  printf("service_id: %x\n", sid); 
	while (desc_tot < desc_len)
	  {
	    switch (buffer[current_i+desc_tot])
	      {
	      case 0x42:
		desc_tot += stuffing_desc(&buffer[current_i+desc_tot]);
		break;
	      case 0x47:
		desc_tot += bouquet_name_desc(&buffer[current_i+desc_tot]);
		break;
	      case 0x48: 
		desc_tot += service_name_desc(&buffer[current_i+desc_tot],sid,tsid,network_id,scan_mode);
		break;
	      case 0x49:
		desc_tot += country_availability_desc(&buffer[current_i+desc_tot]);
		break;
	      case 0x4a: 
		desc_tot += linkage_desc(&buffer[current_i+desc_tot]);
		break;
	      case 0x4b:
		desc_tot += nvod_ref_desc(&buffer[current_i+desc_tot],tsid,scan_mode);
		break;
	      case 0x4c:
		desc_tot += time_shift_service_desc(&buffer[current_i+desc_tot]);
		break;
	      case 0x51:
		desc_tot += mosaic_desc(&buffer[current_i+desc_tot]);
		break;
	      case 0x53:
		desc_tot += ca_ident_desc(&buffer[current_i+desc_tot]);
		break;
	      case 0x57:
		desc_tot += telephone_desc(&buffer[current_i+desc_tot]);
		break;
	      case 0x5d:
		desc_tot += multilingual_service_name_desc(&buffer[current_i+desc_tot]);
		break;
	      case 0x5f:
		desc_tot += priv_data_desc(&buffer[current_i+desc_tot]);
		break;
	      case 0x64:
		desc_tot += data_broadcast_desc(&buffer[current_i+desc_tot]);
		break;
	      default:
		printf("The descriptor-tag was %02x\n", buffer[current_i+desc_tot]);
		desc_tot += buffer[current_i+desc_tot+1]+2;
	      }
	    //printf("Bytes read in description-loop %d\n", desc_tot);
	  }
	}
	current_i += desc_len;
      }
  }
  close(demux);
  return 23;
}
