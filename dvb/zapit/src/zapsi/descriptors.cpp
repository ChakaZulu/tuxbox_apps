#include <stdio.h>
#include <map>
#include <string>
#include <map.h>
#include "sdt.h"
#include "scan.h"



std::map<int,transpondermap> scantransponders;
std::map<int,scanchannel> scanchannels;
multimap<std::string, bouquet_mulmap> scanbouquets;
std::string curr_chan_name;
int found_transponders;
int found_channels;

int stuffing_desc(char *buffer, FILE *logfd)
{
	return buffer[1]+2;
}

int linkage_desc(char *buffer, FILE *logfd)
{
	//printf("Linkange-descriptor to be implemented\n");
	return buffer[1]+2;
}

int priv_data_desc(char *buffer, FILE *logfd)
{
	//printf("Private data descriptor\n");
	return buffer[1]+2;
}

int network_name_desc(char *buffer, FILE *logfd)
{
	int len = buffer[1];
	/*int i;
	std::string name;

	for (i=0;i<len;i++)
		name += buffer[i+2];

	printf("Network-name: %s\n",name.c_str());
	*/
	return len+2;
}

int service_list_desc(char *buffer, FILE *logfd)
{
	int len = buffer[1];
	/*int current = 1;

	while (current < len)
	{
		int sid, st;
		sid = (buffer[++current]<<8)|buffer[++current];
		st = buffer[++current];
		printf("service_id: %04x\n", sid);
		printf("service-type: %04d\n", st);
	}
	*/
	return len+2;
}

int cable_deliv_system_desc(char *buffer, int tsid, FILE *logfd)
{
  int len = buffer[1];
  int freq = (((buffer[2] & 0xf0) >> 4) * 10000) + ((buffer[2] & 0xf) * 1000) + (((buffer[3] & 0xf0) >> 4) * 100) + ((buffer[3] & 0xf)*10) + (((buffer[4]&0xf0)>>4)) ;
  int symbolrate = (((buffer[9] & 0xf0) >> 4) * 100000) + ((buffer[9] & 0xf) * 10000) + (((buffer[10] & 0xf0) >> 4) * 1000) + ((buffer[10] & 0xf)*100) + (((buffer[11]&0xf0)>>4)*10) + (buffer[11]&0xf);
  int fec_inner = (buffer[12]&0xF);

  //printf("frequency: %d\n",freq);
  //printf("Symbol_rate: %d\n",symbolrate);
  //printf("FEC_inner: %d\n",fec_inner);
  if (fec_inner == 15)
  	fec_inner = 0;

  if (scantransponders.count(tsid) == 0)
    {
    	found_transponders++;
    	eventServer->sendEvent(CZapitClient::EVT_SCAN_NUM_TRANSPONDERS, CEventServer::INITID_ZAPIT, &found_transponders, sizeof(found_transponders) );
    	printf("New transponder\n");
	printf("tsid: %04x\n",tsid);
	/*printf("frequency: %d\n", freq);
	printf("Symbolrate: %d\n", symbolrate);
	printf("FEC_inner: %d\n",fec_inner);*/
      	scantransponders.insert(std::pair<int,transpondermap>(tsid, transpondermap(tsid,freq,symbolrate,fec_inner)));
      	fprintf(logfd, "Inserted transponder %04x, Freq: %d, SR: %d, FEC: %d\n", tsid,freq,symbolrate,fec_inner);
      }

  return len+2;
}

int sat_deliv_system_desc(char *buffer, int tsid,int diseqc, FILE *logfd)
{
  int len = buffer[1];
  int freq = (((buffer[2] & 0xf0) >> 4) * 100000) + ((buffer[2] & 0xf) * 10000) + (((buffer[3] & 0xf0) >> 4) * 1000) + ((buffer[3] & 0xf)*100) + (((buffer[4]&0xf0)>>4)*10) + (buffer[4]&0xf);
  int symbolrate = (((buffer[9] & 0xf0) >> 4) * 100000) + ((buffer[9] & 0xf) * 10000) + (((buffer[10] & 0xf0) >> 4) * 1000) + ((buffer[10] & 0xf)*100) + (((buffer[11]&0xf0)>>4)*10) + (buffer[11]&0xf);
  int fec_inner = (buffer[12]&0xF);
  int polarization = ((buffer[8]&0x60)>>5);

	if (scantransponders.count(tsid) == 0)
	{
	  found_transponders++;
	  eventServer->sendEvent(CZapitClient::EVT_SCAN_NUM_TRANSPONDERS, CEventServer::INITID_ZAPIT, &found_transponders, sizeof(found_transponders) );

	  printf("New transponder\n");
	  printf("tsid: %04x\n",tsid);
	  /*printf("frequency: %d\n", freq);
	  printf("Polarization: %d\n", polarization);
	  printf("Symbolrate: %d\n", symbolrate);
	  printf("FEC_inner: %d\n",fec_inner);*/
	  scantransponders.insert(std::pair<int,transpondermap>(tsid, transpondermap(tsid,freq,symbolrate,fec_inner,polarization,diseqc)));
	  fprintf(logfd, "Inserted transponder %04x, Freq: %d, SR: %d, FEC: %d, Pol: %d, Diseqc: %d\n", tsid,freq,symbolrate,fec_inner,polarization,diseqc);
	}

	return len+2;
}

int terr_deliv_system_desc(char *buffer, FILE *logfd)
{
	printf("Anyone has a DVB-T Dbox?\n");
	return buffer[1]+2;
}

int multilingual_network_name_desc(char *buffer, FILE *logfd)
{
	//printf("Multilingual network name descriptor\n");

	return buffer[1]+2;
}

int freq_list_desc(char *buffer, FILE *logfd)
{
	int len = buffer[1];
	/*int current = 3;

	printf("Coding-type: %d\n", (buffer[2]&0x3));

	while (current < len)
		printf("Center Frequency: %d", (buffer[++current]<<24)|(buffer[++current]<<16)|(buffer[++current]<<8)|buffer[++current]);
	*/
	return len+2;
}

int cell_list_desc(char *buffer, FILE *logfd)
{
	//printf("Cell-list-descriptor\nWho has a DVB-T Dbox?\n");
	return buffer[1]+2;
}

int cell_freq_list_desc(char *buffer, FILE *logfd)
{
	//printf("Cell-list-descriptor\nWho has a DVB-T Dbox?\n");
	return buffer[1]+2;
}

int announcement_support_desc(char *buffer, FILE *logfd)
{
	//printf("No announcements\n");
	return buffer[1]+2;
}

char last_provider[100];

int service_name_desc(char *buffer, int sid, int tsid, int onid,bool scan_mode, FILE *logfd)
{
	int len = buffer[1];
	int i=0;
	int name_len = buffer[3];
	int service_name_len = buffer[name_len+4];
	std::string provname;
	std::string servicename;
	std::map<int,scanchannel>::iterator I = scanchannels.find((tsid<<16)+sid);
	int service_type = buffer[2];

	//printf("service-type %d\n",service_type);

	for (i = 0; i<name_len; i++)
	{
		//if (isprint(buffer[i+4]))
			//provname += buffer[i+4];
			switch (buffer[i+4]) {
     				case '&':
        				provname += "&amp;";
        			break;
      				case '<':
        				provname += "&lt;";
        			break;
      				case '\"':
       					provname += "&quot;";
        			break;
      				case 0x81:
      				case 0x82:
        			break;
      				case 0x86:
				//        provname += "<b>";
        			break;
      				case 0x87:
				//        provname += "</b>";
        			break;
      				case 0x8a:
				//      provname += "<br/>";
        			break;
      				default:
        			if (buffer[i+4]<32)
          			break;
        			if ((buffer[i+4]>=32) && (buffer[i+4]<128))
          				provname += buffer[i+4];
          			else if (buffer[i+4] == 128)
           				;
        			else
        			{
        				char val[5];
        				sprintf(val, "%d", buffer[i+4]);
          				provname += "&#";
          				provname += val;
          				provname += ";";
          			}
		}
	}

	for (i=0;i<service_name_len;i++)
	{
		//if (isprint(buffer[i+name_len+5]))
			//servicename += buffer[i+name_len+5];

			switch (buffer[i+name_len+5]) {
     				case '&':
        				servicename += "&amp;";
        			break;
      				case '<':
        				servicename += "&lt;";
        			break;
      				case '\"':
       					servicename += "&quot;";
        			break;
      				case 0x81:
      				case 0x82:
        			break;
      				case 0x86:
				//        servicename += "<b>";
        			break;
      				case 0x87:
				//        servicename += "</b>";
        			break;
      				case 0x8a:
				//      servicename += "<br/>";
        			break;
      				default:
        			if (buffer[i+name_len+5]<32)
          			break;
        			if ((buffer[i+name_len+5]>=32) && (buffer[i+name_len+5]<128))
          				servicename += buffer[i+name_len+5];
          			else if (buffer[i+name_len+5] == 128)
           				;
        			else
        			{
        				char val[5];
        				sprintf(val, "%d", buffer[i+name_len+5]);
          				servicename += "&#";
          				servicename += val;
          				servicename += ";";
          			}
		}
	}

	printf("provider: %s\n",provname.c_str());
	printf("service: %s\n",servicename.c_str());

	fprintf(logfd, "provider: %s\n",provname.c_str());
	fprintf(logfd, "service: %s\n",servicename.c_str());
	if (scan_mode)
	{
#ifdef NVOD_HACK
	curr_chan_name = servicename;
#endif
	if (scanchannels.count((tsid<<16)+sid) != 0)
	{
		printf("Found a channel in map\n");
		I->second.name = servicename;
		I->second.onid = onid;
		I->second.service_type = service_type;
	}
	else
	{
		found_channels++;
		eventServer->sendEvent(CZapitClient::EVT_SCAN_NUM_TRANSPONDERS, CEventServer::INITID_ZAPIT, &found_channels, sizeof(found_channels) );

		scanchannels.insert(std::pair<int,scanchannel>((tsid<<16)+sid,scanchannel(servicename,sid,tsid,onid,service_type)));
	}

	if (provname == "")
		provname = "Unknown Provider";

	if ( strcmp(last_provider, provname.c_str()) != 0 )
	{
		strcpy(last_provider, provname.c_str());
	    eventServer->sendEvent(CZapitClient::EVT_SCAN_PROVIDER, CEventServer::INITID_ZAPIT, last_provider, strlen(last_provider)+ 1 );
	}

	if (service_type == 1 || service_type == 2 || service_type == 4 || service_type == 5)
		scanbouquets.insert(std::pair<std::string,bouquet_mulmap>(provname.c_str(),bouquet_mulmap(provname, servicename, sid,onid)));

	}


	return len+2;
}

int bouquet_name_desc(char *buffer, FILE *logfd)
{
	int len = buffer[1];
	/*int i = 0;
	std::string name;

	for (i = 0; i<len; i++)
	{
	  name += buffer[i+2];
	}
	printf("Bouquet name: %s\n",name.c_str());*/
	return len+2;
}



int country_availability_desc(char *buffer, FILE *logfd)
{
  //printf("country_availability_desc to be implemented\n");
  return buffer[1]+2;
}


int nvod_ref_desc(char *buffer,int tsid,bool scan_mode, FILE *logfd)
{
	int len = buffer[1];
	std::string servicename;

#ifdef NVOD_HACK
	int number = 1;
	//printf("NVOD on:\n");
	for (int i = 0; i<len;i++)
	{
		int tsid, onid, sid;
		char number_c[3];

		sprintf(number_c,"%d", number++);
		servicename = curr_chan_name + "/" + number_c;
		//printf("\n\nFound nvod-reference: %s\n\n",servicename.c_str());
		tsid = (buffer[i+2]<<16)|buffer[(++i)+2];
		onid = (buffer[(++i)+2]<<16)|buffer[(++i)+2];
		sid = (buffer[(++i)+2]<<16)|buffer[(++i)+2];

		printf("tsid: %04x, onid: %04x, sid: %04x\n",tsid,onid,sid);
		if (scan_mode)
		{
			if (scanchannels.count((tsid<<16)+sid) != 0)
		  	{
		    	//printf("Found a channel in map\n");
		    	std::map<int,scanchannel>::iterator I = scanchannels.find((tsid<<16)+sid);
		    	I->second.name = servicename;
		    	I->second.onid = onid;
		    	I->second.service_type = 1;
		  	}
			else
		  	{
		  	found_channels++;
		    	scanchannels.insert(std::pair<int,scanchannel>((tsid<<16)+sid,scanchannel(servicename,sid,tsid,onid,1)));
			}
		}
		else
		{
			nvodchannels.insert(std::pair<int,channel>((onid<<16)+sid,channel(servicename,0,0,0,0,0,sid,tsid,onid,1)));
		}
	}

#endif

	return len+2;
}

int time_shift_service_desc(char *buffer, FILE *logfd)
{
  //printf("Time-shifted service descriptor\n");
  return buffer[1]+2;
}

int mosaic_desc(char *buffer, FILE *logfd)
{
  //printf("mosaic-descriptor\n");
  return buffer[1]+2;
}

int ca_ident_desc(char *buffer, FILE *logfd)
{
  //printf("ca-identifier descriptor\n");
  return buffer[1]+2;
}

int telephone_desc(char *buffer, FILE *logfd)
{
  //printf("Telephone descriptor\n");
  return buffer[1]+2;
}

int multilingual_service_name_desc(char *buffer, FILE *logfd)
{
  //printf("Multilingual service name descriptor\nGerman should be enough for us.\n");
  return buffer[1]+2;
}

int data_broadcast_desc(char *buffer, FILE *logfd)
{
  //printf("Data descriptor\n");
  return buffer[1]+2;
}
