#include <stdio.h>
#include <map>
#include <map.h>
#include "sdt.h"

int stuffing_desc(char *buffer)
{
	return buffer[1]+2;
}

int linkage_desc(char *buffer)
{
	printf("Linkange-descriptor to be implemented\n");
	return buffer[1]+2;
}

int priv_data_desc(char *buffer)
{
	printf("Private data descriptor\n");
	return buffer[1]+2;
}

int network_name_desc(char *buffer)
{
	int len = buffer[1];
	int i;
	std::string name;
		
	for (i=0;i<len;i++)
		name += buffer[i+2];
	
	printf("Network-name: %s\n",name.c_str());
	return len+2;
}

int service_list_desc(char *buffer)
{
	int len = buffer[1];
	int current = 1;
	
	while (current < len)
	{
		int sid, st;
		sid = (buffer[++current]<<8)|buffer[++current];
		st = buffer[++current];
		//printf("service_id: %04x\n", sid);
		//printf("service-type: %04d\n", st);
	}
	
	return len+2;
}

int cable_deliv_system_desc(char *buffer, int tsid)
{
	int len = buffer[1];
	int freq = (((buffer[2] & 0xf0) >> 4) * 100000) + ((buffer[2] & 0xf) * 10000) + (((buffer[3] & 0xf0) >> 4) * 1000) + ((buffer[3] & 0xf)*100)   + (((buffer[4]&0xf0)>>4)*10) + (buffer[4]&0xf);
	int symbolrate = (((buffer[9] & 0xf0) >> 4) * 100000) + ((buffer[9] & 0xf) * 10000) + (((buffer[10] & 0xf0) >> 4) * 1000) + ((buffer[10] & 0xf)*100) + (((buffer[11]&0xf0)<<4)*10) + (buffer[11]&0xf);
	int fec_inner = (buffer[12]&0xF0);
	
	//printf("frequency: %d\n",freq);
	//printf("Symbol_rate: %d\n",symbolrate);
	//printf("FEC_inner: %d\n",fec_inner);
	
	transponders.insert(std::pair<int,transponder>(tsid, transponder(tsid,freq,symbolrate,0,0,fec_inner,0)));
	
	//tune(freq,symbolrate);
	
	return len+2;
}

int sat_deliv_system_desc(char *buffer, int tsid)
{
	int len = buffer[1];
	int freq = (((buffer[2] & 0xf0) >> 4) * 100000) + ((buffer[2] & 0xf) * 10000) + (((buffer[3] & 0xf0) >> 4) * 1000) + ((buffer[3] & 0xf)*100) + (((buffer[4]&0xf0)>>4)*10) + (buffer[4]&0xf);
	int symbolrate = (((buffer[9] & 0xf0) >> 4) * 100000) + ((buffer[9] & 0xf) * 10000) + (((buffer[10] & 0xf0) >> 4) * 1000) + ((buffer[10] & 0xf)*100) + (((buffer[11]&0xf0)<<4)*10) + (buffer[11]&0xf);
	int fec_inner = (buffer[12]&0xF0);
	int polarisation = ((buffer[8]&0x6)>>5);
	
	printf("tsid: %04x\n", tsid);
	printf("frequency: %d\n", freq);
	printf("Polarisation: %d\n", polarisation);
	printf("Symbolrate: %d\n", symbolrate);
	printf("FEC_inner: %d\n",fec_inner);
	
	transponders.insert(std::pair<int,transponder>(tsid, transponder(tsid,freq,symbolrate,polarisation,0,fec_inner,0)));
	return len+2;
}

int terr_deliv_system_desc(char *buffer)
{
	printf("Anyone has a DVB-T Dbox?\n");
	return buffer[1]+2;
}

int multilingual_network_name_desc(char *buffer)
{
	printf("Multilingual network name descriptor\n");
	
	return buffer[1]+2;
}

int freq_list_desc(char *buffer)
{
	int len = buffer[1];
	int current = 3;
	
	printf("Coding-type: %d\n", (buffer[2]&0x3));
	
	while (current < len)
		printf("Center Frequency: %d", (buffer[++current]<<24)|(buffer[++current]<<16)|(buffer[++current]<<8)|buffer[++current]);		
		
	return len+2;
}

int cell_list_desc(char *buffer)
{
	printf("Cell-list-descriptor\nWho has a DVB-T Dbox?\n");
	return buffer[1]+2;
}

int cell_freq_list_desc(char *buffer)
{
	printf("Cell-list-descriptor\nWho has a DVB-T Dbox?\n");
	return buffer[1]+2;
}

int announcement_support_desc(char *buffer)
{
	printf("No announcements\n");
	return buffer[1]+2;
}

int service_name_desc(char *buffer, int sid, int tsid, int onid)
{
	int len = buffer[1];
	int i=0; 
	int name_len = buffer[3];
	int service_name_len = buffer[name_len+4];
	std::string provname;
	std::string servicename;
	int service_type = buffer[2];
	
	
	printf("service-type %d\n",service_type);
	
	for (i = 0; i<name_len; i++)
	{
		//if (isprint(buffer[i+4]))
			provname += buffer[i+4];
	}
	
	for (i=0;i<service_name_len;i++)
	{
	  //if (isprint(buffer[i+name_len+5]))
	  servicename += buffer[i+name_len+5];		
	}
	
	printf("provider: %s\n",provname.c_str());
	printf("service: %s\n",servicename.c_str());

	return len+2;
}

int bouquet_name_desc(char *buffer)
{
	int len = buffer[1];
	int i = 0;
	std::string name;
			
	for (i = 0; i<len; i++)
	{
	  name += buffer[i+2];
	}
	printf("Bouquet name: %s\n",name.c_str());
	return len+2;
}



int country_availability_desc(char *buffer)
{
  printf("country_availability_desc to be implemented\n");
  return buffer[1]+2;
}
	

int nvod_ref_desc(char *buffer,int sid,int tsid)
{
  int len = buffer[1];
  std::string servicename = "NVOD";

  printf("NVOD on:\n");
  for (int i = 0; i<len;i++)
    {
      int tsid, onid, sid;
      
      tsid = (buffer[i+2]<<16)|buffer[(++i)+2];
      onid = (buffer[(++i)+2]<<16)|buffer[(++i)+2];
      sid = (buffer[(++i)+2]<<16)|buffer[(++i)+2];
      
      printf("tsid: %04x, onid: %04x, sid: %04x\n",tsid,onid,sid);
      nvodchannels.insert(std::pair<int,channel>((onid<<16)+sid,channel(servicename,0,0,0,0,0,sid,tsid,onid,2)));
    }
  return len+2;
}

int time_shift_service_desc(char *buffer)
{
  printf("Time-shifted service descriptor\n");
  return buffer[1]+2;
}

int mosaic_desc(char *buffer)
{
  printf("mosaic-descriptor\n");
  return buffer[1]+2;
}

int ca_ident_desc(char *buffer)
{
  printf("ca-identifier descriptor\n");
  return buffer[1]+2;
}

int telephone_desc(char *buffer)
{
  printf("Telephone descriptor\n");
  return buffer[1]+2;
}

int multilingual_service_name_desc(char *buffer)
{
  printf("Multilingual service name descriptor\nGerman should be enough for us.\n");
  return buffer[1]+2;
}

int data_broadcast_desc(char *buffer)
{
  printf("Data descriptor\n");
  return buffer[1]+2;
}
