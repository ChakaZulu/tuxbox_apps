/*

  Zapit  -   DBoxII-Project

  $Id: zapit.cpp,v 1.80 2002/02/20 18:48:33 obi Exp $

  Done 2001 by Philipp Leusmann using many parts of code from older
  applications by the DBoxII-Project.

  Kommentar:

  Dies ist ein zapper der für die kommunikation über tcp/ip ausgelegt ist.
  Er benutzt Port 1505
  Die Kanalliste muß als CONFIGDIR/zapit/settings.xml erstellt vorhanden sein.
  Die Bouqueteinstellungen liegen in CONFIGDIR/zapit/bouquets.xml

  cmd = 1 zap to channel (numeric)
  param = channelnumber

  cmd = 2 kill current zap for streaming

  cmd = 3 zap to channel (channelname)
  param3 = channelname

  cmd = 4 shutdown the box

  cmd = 5 Get the Channellist

  cmd = 6 Switch to RadioMode

  cmd = 7 Switch to TVMode

  cmd = 8 Get back a struct of current Apid-descriptions.

  cmd = 9 Change apid
  param = apid-number (0 .. count_apids-1)

  cmd = 'a' Get last channel

  cmd = 'b' Get current vpid and apid

  cmd = 'c'  - wie cmd 5, nur mit onid_sid

  cmd = 'd'  wie cmd 1, nut mit onid_sid
  param = (onid<<16)|sid
  response[1] liefert status-infos...

  cmd = 'e' change nvod (Es muss vorher auf den Basiskanal geschaltet worden sein)
  param = (onid<<16)|sid

  cmd = 'f' is nvod-base-channel?
  if true returns chans_msg2 for each nvod_channel;

  cmd = 'p' prepare channels
  calls prepare_channels to reload services and bouquets

  cmd = 'q' get list of all bouquets

  cmd = 'r' get list of channels of a specified bouquet
  param = id of bouquet

  cmd = 't' Get or-ed values for caid and ca-version.
  		caid 0x1722 == 1
  		caid 0x1702 == 2
  		caid 0x1762 == 4
  		other caid == 8
  		cam-type E == 16
  		cam-type D == 32
  		cam-type F == 64
  		other cam-type == 128
  		so valid are : 33, 18 and 68

  cmd = 'u' get current vtxt-pid


  Bei Fehlschlagen eines Kommandos wird der negative Wert des kommandos zurückgegeben.

  License: GPL

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/



#include "zapit.h"
#include "lcddclient.h"

extern int errno;

#ifndef DVBS
CLcddClient lcdd;
#endif /* DVBS */

static int debug = 0;

#define dprintf(fmt, args...) {if(debug) printf(fmt, ## args);}
#define dputs(str) {if(debug) puts(str);}

extern uint16_t old_tsid;
uint curr_onid_sid = 0;
bool OldAC3 = false;

uint8_t fec_inner = 0;
uint16_t vpid, apid, pmt = 0;
bool current_is_nvod;
std::string nvodname;

int video_fd = -1;
int dmx_video_fd = -1;
int dmx_audio_fd = -1;

std::map<uint, uint> allnumchannels_tv;
std::map<uint, uint> allnumchannels_radio;
std::map<std::string, uint> allnamechannels_tv;
std::map<std::string, uint> allnamechannels_radio;

extern std::map<uint, transponder>transponders;
std::map<uint, channel> allchans_tv;
std::map<uint, uint> numchans_tv;
std::map<std::string, uint> namechans_tv;
std::map<uint, channel> allchans_radio;
std::map<uint, uint> numchans_radio;
std::map<std::string, uint> namechans_radio;

typedef std::map<uint, transponder>::iterator titerator;

bool Radiomode_on = false;
pids pids_desc;
bool caid_set = false;

pthread_t scan_thread;
extern int found_transponders;
extern int found_channels;
extern short curr_sat;
extern short scan_runs;
boolean use_vtxtd = false;
int vtxt_pid;

void start_scan();
volatile sig_atomic_t keep_going = 1; /* controls program termination */

void sendBouquetList();
void sendChannelListOfBouquet( uint nBouquet);

CBouquetManager* g_BouquetMan;

#ifdef USE_EXTERNAL_CAMD
static int camdpid = -1;
static int lastpmt = -1;
#endif /* USE_EXTERNAL_CAMD */

void termination_handler (int signum)
{
#ifdef USE_EXTERNAL_CAMD
	if (camdpid != -1)
	{
		kill(camdpid, SIGKILL);
		waitpid(camdpid, 0, 0);
	}
#endif
	close(connfd);
	system("cp /tmp/zapit_last_chan " CONFIGDIR "/zapit/last_chan");
	exit(0);
}

#ifndef DVBS
// nachdem #include "gen_vbi.h" noch nicht geht (noch nicht offiziell im cdk...)
#define VBI_START_VTXT 1
#define VBI_STOP_VTXT 2

int set_vtxt (uint vpid)
{
	int fd;
	int vtxtsock;
	FILE *vtxtfd;
	struct sockaddr_un vtxtsrv;
	char vtxtbuf[255];
	char hexpid[20];

	vtxt_pid = vpid;

	if (use_vtxtd)
	{
		memset(&hexpid, 0, sizeof(hexpid));
		sprintf(hexpid, "%x", vpid);
		vtxtsock=socket(AF_LOCAL, SOCK_STREAM, 0);
		memset(&vtxtsrv, 0, sizeof(vtxtsrv));
		vtxtsrv.sun_family = AF_LOCAL;
		strcpy(vtxtsrv.sun_path, "/var/dvb/vtxtd");
		connect(vtxtsock, (const struct sockaddr *) &vtxtsrv, sizeof(vtxtsrv));
		vtxtfd=fdopen(vtxtsock, "a+");
		setlinebuf(vtxtfd);

		if (vtxtfd == NULL)
		{
			perror("[zapit] vtxtd fdopen");
		}
		else
		{
			if (vpid == 0)
			{
				fprintf(vtxtfd,"stop\n");
				dprintf("[zapit] vtxtd return %s\n", fgets(vtxtbuf, 255, vtxtfd));
			}
			else
			{
				fprintf(vtxtfd,"stop\n");
				dprintf("[zapit] vtxtd return %s\n", fgets(vtxtbuf,255,vtxtfd));
				fprintf(vtxtfd,"pid %s\n", hexpid);
				dprintf("[zapit] vtxtd return %s\n", fgets(vtxtbuf,255,vtxtfd));
			}
			fclose(vtxtfd);
		}
	}
	else
	{
		fd = open("/dev/dbox/vbi0", O_RDWR);

		if (fd < 0)
		{
			perror ("[zapit] /dev/dbox/vbi0");
			return -fd;
		}

		if (vpid == 0)
		{
			if (ioctl(fd, VBI_STOP_VTXT, vpid) < 0)
			{
				close(fd);
				perror("[zapit] VBI_STOP_VTXT");
				return 1;
			}
		}
		else
		{
			if (ioctl(fd, VBI_START_VTXT, vpid) < 0)
			{
				close(fd);
				perror("[zapit] VBI_START_VTXT");
				return 1;
			}
		}
		
		close(fd);
	}
	
	return 0;
}
#endif /* DVBS */

int parsePMTInfo (unsigned char *buffer, int len, int ca_system_id)
{
	int count = 0;
	int desc;
	int len2;
	int ca_id;
	int ca_pid = no_ecmpid_found;

	while (count < len)
	{
		desc = buffer[count++];
		len2 = buffer[count++];

		if (desc == 0x09)
		{
			ca_id = (buffer[count] << 8) | buffer[count + 1];
			count += 2;

			if ((ca_id == ca_system_id) && ((ca_id >> 8) == ((0x18|0x27)&0xD7)))
			{
				ca_pid= ((buffer[count] & 0x1F) << 8) | (buffer[count + 1] & 0xFF);
			}

			else if (ca_pid == no_ecmpid_found)
			{
				ca_pid = invalid_ecmpid_found;
			}
			
	  		count+=2;
			count+=(len2-4);
		}
		else
		{
			count+=len2;
		}
	}
	
	return ca_pid;
}

pids parse_pmt (int pid, int ca_system_id)
{
	unsigned char buffer[PMT_SIZE];
	int fd;
	int pt;
	int ap_count = 0;
	int vp_count = 0;
	int ecm_pid = no_ecmpid_found;
	struct dmxSctFilterParams flt;
	pids ret_pids;
	struct pollfd dmx_fd;
	int PMTInfoLen;
	int dp;
	int section_length;
	uint8_t stype;
	uint16_t epid, esinfo;
	int i_pt;
	int tag_type;
	int tag_len;

	memset(&ret_pids, 0, sizeof(ret_pids));

	if ((fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
		perror("[zapit] unable to open demux device");
		return ret_pids;
	}

	memset(&flt.filter.filter, 0, DMX_FILTER_SIZE);
	memset(&flt.filter.mask, 0, DMX_FILTER_SIZE);

	dprintf("[zapit] pmtpid: %04x\n", pid);

	flt.pid = pid;
	flt.filter.filter[0] = 0x02;
	flt.filter.mask[0]  = 0xFF;
	flt.timeout = 5000;
	flt.flags= DMX_CHECK_CRC | DMX_ONESHOT | DMX_IMMEDIATE_START;

	dprintf("[zapit] DMX_SET_FILTER\n");

	if (ioctl(fd, DMX_SET_FILTER, &flt) < 0)
	{
		perror("[zapit] DMX_SET_FILTER");
		close(fd);
		return ret_pids;
	}
	
	dmx_fd.fd = fd;
	dmx_fd.events = POLLIN;
	dmx_fd.revents = 0;

	// wenn er ordentlich tuned, dann hat er nach 250ms die pmt, wenn nicht, dann hilft längeres warten auch nicht....
	// Bei mir kommt trotzdem manchmal nen timeout. Setze auf 400 (faralla)
	pt = poll(&dmx_fd, 1, 400);  // war 500;

	if (!pt)
	{
		dprintf("[zapit] poll timeout - zapping is probably going to fail\n");
		close(fd);
		return ret_pids;
	}
	else if (pt < 0)
	{
		perror ("[zapit] pmt-poll");
		close(fd);
		return ret_pids;
	}
	else
	{
		dprintf("[zapit] parsepmt is reading DMX\n");

		if (read(fd, buffer, PMT_SIZE) <= 0)
		{
			perror("[zapit] read pmt");
			close(fd);
			return ret_pids;
		}

		close(fd);

		dprintf("[zapit] parsepmt is parsing pmts\n");

		section_length = ((buffer[1] & 0xF) << 8) + buffer[2];

		/*
		FILE *file=fopen("zapit.pmt", "wb");
		if (file)
		{
			fwrite(buffer, sec_len+ 3, 1, file);
			fclose(file);
		}
		*/
		
		PMTInfoLen = ((buffer[10] & 0xF) << 8) | buffer[11];
		dp= 12;
		
		if (PMTInfoLen > 0)
		{
			ecm_pid = parsePMTInfo(&buffer[12], PMTInfoLen, ca_system_id);
			dp += PMTInfoLen;
		}
		else
		{
			ecm_pid = no_ecmpid_found; // not scrambled...
		}

		dprintf("[zapit] ecm_pid: %04x\n", ecm_pid);

		while (dp < (section_length - 1))
		{
			stype = buffer[dp++];
			dprintf("[zapit] stream type: %x\n", stype);

			epid = ((buffer[dp] & 0x1F) << 8) | buffer[dp+1];
			dp += 2;
			
			esinfo = ((buffer[dp] & 0x0F) << 8) | buffer[dp+1];
			dp += 2;

			if (((stype == 1) || (stype == 2)) && (epid != 0))
			{
				if (ecm_pid == no_ecmpid_found)
					ecm_pid = parsePMTInfo(&buffer[dp], esinfo, ca_system_id);

				ret_pids.vpid = epid;
				vp_count++;
				dp += esinfo;
			}
			else if (((stype == 3) || (stype == 4) || (stype == 6)) && (epid != 0))
			{
				i_pt = dp;
				tag_type, tag_len;
				ret_pids.apids[ap_count].component_tag = -1;
				dp += esinfo;

				ret_pids.apids[ap_count].is_ac3 = false;
				ret_pids.apids[ap_count].desc[0] = 0;

				while (i_pt < dp)
				{
					tag_type = buffer[i_pt++];
					tag_len = buffer[i_pt++];
					
					switch (tag_type)
					{
					case 0x6A: // AC3
						ret_pids.apids[ap_count].is_ac3 = true;
						break;
						
					case 0x0A: // LangDescriptor
						if (ret_pids.apids[ap_count].desc[0] == 0)
						{
							buffer[i_pt+ 3]= 0; // quick'n'dirty
							memcpy(ret_pids.apids[ap_count].desc, &(buffer[i_pt]), tag_len);
						}
						break;
						
					case 0x52: // STREAM_IDENTIFIER_DESCR
						ret_pids.apids[ap_count].component_tag = buffer[i_pt];
						break;

					case 0x56: // DESCR_TELETEXT
						ret_pids.vtxtpid = epid;
						// printf("[zapit] vtxtpid %x\n", ret_pids.vtxtpid);
						break;
					default:
						break;
					}

					i_pt += tag_len;
				}

				if ((stype == 3) || (stype == 4) || ret_pids.apids[ap_count].is_ac3)
				{
					if (ret_pids.apids[ap_count].desc[0] == 0)
						sprintf(ret_pids.apids[ap_count].desc, "%02d", ap_count+ 1);

					ret_pids.apids[ap_count].pid = epid;
					
					if (ap_count <max_num_apids )
						ap_count++;
				}
			}
			else
			{
				dp+= esinfo;
			}
		}
		
		ret_pids.count_apids = ap_count;
		ret_pids.count_vpids = vp_count;
		ret_pids.ecmpid = ecm_pid;
	}

	dprintf("[zapit] parsepmt is nearly over\n");
	return ret_pids;
}



int find_emmpid (int ca_system_id)
{
	char buffer[CAT_SIZE];
	int fd;
	int section_length;
	int count;
	struct dmxSctFilterParams flt;
	
	if ((fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
		perror("[zapit] unable to open demux device");
		return 0;
	}

	memset(&flt.filter.filter, 0, DMX_FILTER_SIZE);
	memset(&flt.filter.mask, 0, DMX_FILTER_SIZE);
	
	flt.pid = 0x0001;
	flt.filter.filter[0] = 0x01;
	flt.filter.mask[0] = 0xFF;
	flt.timeout = 1000;
	flt.flags = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;

	if (ioctl(fd, DMX_SET_FILTER, &flt) < 0)
	{
		perror("[zapit] DMX_SET_FILTER");
		close(fd);
		return 0;
	}

	if (read(fd, buffer, CAT_SIZE) <= 0)
	{
		perror("[zapit] unable to read from demux device");
		close(fd);
		return 0;
	}

	close(fd);

	section_length = ((buffer[1] & 0x0F) << 8) | buffer[2];
	
	count = 8;

	while (count < (section_length - 1))
	{
		if ((((buffer[count + 2] << 8) | buffer[count + 3]) == ca_system_id) &&	(buffer[count + 2] == ((0x18|0x27)&0xD7)))
		{
			return ((buffer[count + 4] << 8) | buffer[count + 5]) & 0x1FFF;
		}
		else
		{
			count += buffer[count + 1] + 2;
		}
	}

	return 0;
}

#ifndef USE_EXTERNAL_CAMD
void _writecamnu (int cmd, unsigned char *data, int len)
{
	int camfd;
	char buffer[256];
	int csum = 0;
	int i;
	int pt;
	struct pollfd cam_pfd;
	bool output = false;

	if((camfd = open("/dev/dbox/cam0", O_RDWR)) < 0)
	{
		perror("[zapit] _writecam: open cam0");
		close(camfd);
		return;
	}

	buffer[0] = 0x6E;
	buffer[1] = 0x50;
	buffer[2] = (len + 1) | ((cmd != 0x23) ? 0x80 : 0);
	buffer[3] = cmd;
	memcpy(buffer + 4, data, len);
	len += 4;
	for (i = 0; i < len; i++)
	{
		csum^=buffer[i];
	}
	buffer[len++]=csum;

	if (write(camfd, buffer + 1, len - 1) <= 0)
	{
		perror("[zapit] cam: write");
		close(camfd);
		return;
	}

	if (buffer[4] == 0x03)
	{
		close(camfd);
		return; // Let get_caid read the caid;
	}

#if 0
	if (buffer[4] == 0x84)
	{
  		close(camfd);
		return; //Setting emmpid. No answer expected.
	}
#endif

	if (buffer[4] == 0x0d)
	{
		output = true;
	}

	if ((output) && (debug))
	{
		printf("[zapit] sending to cam: ");
		for (i = 0; i < len; i++) printf("%02X ", buffer[i]);
		printf("\n");
	}
	
	cam_pfd.fd = camfd;
	cam_pfd.events = POLLIN;
	cam_pfd.revents = 0;

	pt = poll(&cam_pfd, 1, 1000);

	if (!pt)
	{
		dprintf("[zapit] Read cam. Poll timeout\n");
		close(camfd);
		return;
	}

	if (read(camfd, &buffer, sizeof(buffer)) <= 0)
	{
		perror("[zapit] read cam");
		close(camfd);
		return;
	}

	if ((output) && (debug))
	{
		printf("[zapit] ca returned: ");
		for (i = 0; i < buffer[2] + 4; i++) printf("%02X ", buffer[i]);
		printf("\n");
	}

	close(camfd);
	return;
}

void writecam (unsigned char *data, int len)
{
	_writecamnu(0x23, data, len);
}

void descramble (int onID, int serviceID, int unknown, int caID, int ecmpid, pids *decode_pids)
{
	unsigned char buffer[100];
	int i;
	int p;

	buffer[0] = 0x0D;
	buffer[1] = onID >> 8;
	buffer[2] = onID & 0xFF;
	buffer[3] = serviceID >> 8;
	buffer[4] = serviceID & 0xFF;
	buffer[5] = unknown >> 8;
	buffer[6] = unknown & 0xFF;
	buffer[7] = caID >> 8;
	buffer[8] = caID & 0xFF;
	buffer[9] = ecmpid >> 8;
	buffer[10] = ecmpid & 0xFF;
	buffer[11] = decode_pids->count_vpids + decode_pids->count_apids;

	p = 12;

	for(i = 0; i < decode_pids->count_vpids; i++)
  	{
		buffer[p++] = decode_pids->vpid >> 8;
		buffer[p++] = decode_pids->vpid & 0xFF;
		buffer[p++] = 0x80;
		buffer[p++] = 0;
	}

	for(i = 0; i < decode_pids->count_apids; i++)
	{
		buffer[p++] = decode_pids->apids[i].pid >> 8;
		buffer[p++] = decode_pids->apids[i].pid & 0xFF;
		buffer[p++] = 0x80;
		buffer[p++] = 0;
	}
	
	writecam(buffer, p);

#if 0
	buffer[12] = vpid >> 8;
	buffer[13] = vpid & 0xFF;
	buffer[14] = 0x80;
	buffer[15] = 0;
	buffer[16] = apid >> 8;
	buffer[17] = apid & 0xFF;
	buffer[18] = 0x80;
	buffer[19] = 0;
	writecam(buffer, 20);
#endif
	return;
}

void cam_reset ()
{
	unsigned char buffer[1];
	buffer[0] = 0x9;
	writecam(buffer, 1);
	return;
}

void setemm (int unknown, int caID, int emmpid)
{
	unsigned char buffer[7];
	buffer[0] = 0x84;
	buffer[1] = unknown >> 8;
	buffer[2] = unknown & 0xFF;
	buffer[3] = caID >> 8;
	buffer[4] = caID & 0xFF;
	buffer[5] = emmpid >> 8;
	buffer[6] = emmpid & 0xFF;
	writecam(buffer, 7);
	return;
}
#endif /* USE_EXTERNAL_CAMD */

void save_settings()
{
	FILE *channel_settings;
	channel_settings = fopen("/tmp/zapit_last_chan", "w");

	if (channel_settings == NULL)
	{
		perror("[zapit] fopen: /tmp/zapit_last_chan");
	}

	if (Radiomode_on)
	{
		CBouquetManager::radioChannelIterator cit = g_BouquetMan->radioChannelsFind(curr_onid_sid);
		if (cit != g_BouquetMan->radioChannelsEnd())
		{
			fprintf(channel_settings, "radio\n");
			fprintf(channel_settings, "%06d\n", (*cit)->chan_nr);
			fprintf(channel_settings, "%s\n", (*cit)->name.c_str());
		}
	}
	else
	{
		CBouquetManager::tvChannelIterator cit = g_BouquetMan->tvChannelsFind(curr_onid_sid);
		if (cit != g_BouquetMan->tvChannelsEnd())
		{
			fprintf(channel_settings, "tv\n");
			fprintf(channel_settings, "%06d\n", (*cit)->chan_nr);
			fprintf(channel_settings, "%s\n", (*cit)->name.c_str());
		}
	}

	fclose(channel_settings);
	//printf("Saved settings\n");
	return;
}

channel_msg load_settings()
{
	FILE *channel_settings;
	channel_msg output_msg;
	char *buffer;

	buffer = (char*) malloc(31);

	memset(&output_msg, 0, sizeof(output_msg));

	channel_settings = fopen("/tmp/zapit_last_chan", "r");

	if (channel_settings == NULL)
	{
		perror("[zapit] fopen: /tmp/zapit_last_chan");
		output_msg.mode = 't';
		output_msg.chan_nr = 1;
		return output_msg;
	}

	fscanf(channel_settings, "%s", buffer);

	if (!strcmp(buffer, "tv"))
	{
		output_msg.mode = 't';
	}
	else if (!strcmp(buffer, "radio"))
	{
		output_msg.mode = 'r';
	}
	else
	{
		printf("[zapit] no valid settings found\n");
		output_msg.mode = 't';
		output_msg.chan_nr = 1;
		return output_msg;
	}

	fscanf(channel_settings, "%s", buffer);
	output_msg.chan_nr = atoi(buffer);

	fscanf(channel_settings, "%s", buffer);
	strncpy(output_msg.name, buffer, 30);

	fclose(channel_settings);

	return output_msg;
}

#ifndef USE_EXTERNAL_CAMD
void *decode_thread(void *ptr)
{
	decode_vals *vals;
	vals = (decode_vals *) ptr;
	int emmpid = 0;

	dprintf("[zapit] starting decode_thread\n");

	if (vals->do_cam_reset)
	{
		cam_reset();
	}

	descramble(vals->onid, vals->tsid, 0x104, caid, vals->ecmpid, vals->parse_pmt_pids);

	if (vals->do_search_emmpids)
	{
		if((emmpid = find_emmpid(caid)) != 0)
		{
			dprintf("[zapit] emmpid >0x%04x< found for caid 0x%04x\n", emmpid, caid);
			setemm(0x104, caid, emmpid);
		}
		else
		{
			dprintf("[zapit] no emmpid found...\n");
		}
	}
	
	free(vals);
	dprintf("[zapit] ending decode_thread\n");
	pthread_exit(0);
}
#endif /* USE_EXTERNAL_CAMD */

int zapit (uint onid_sid, bool in_nvod)
{
	struct dmxPesFilterParams pes_filter;
	uint16_t Pmt, Vpid, Apid;
	pids parse_pmt_pids;
	std::map<uint, channel>::iterator cit;
	//time_t current_time = 0;
	//uint16_t emmpid;
	bool do_search_emmpid;
	//std::map<uint,channel>::iterator cI;
	int ac3d;
	bool new_tsid = false;
#ifndef USE_EXTERNAL_CAMD
	pthread_t dec_thread;
	bool do_cam_reset = false;
#else
	char vpidbuf[5];
	char apidbuf[5];
	char pmtpidbuf[5];
#endif /* USE_EXTERNAL_CAMD */

	if (in_nvod)
	{
		current_is_nvod = true;
		
		if (nvodchannels.count(onid_sid) > 0)
		{
			cit = nvodchannels.find(onid_sid);
		}
		else
		{
			dprintf("[zapit] onid_sid %08x not found\n", onid_sid);
			return -3;
		}
	}
	else
	{
		current_is_nvod = false;
		
		if (Radiomode_on)
		{
			if (allchans_radio.count(onid_sid) > 0)
			{
				cit = allchans_radio.find(onid_sid);
			}
			else
			{
				dprintf("[zapit] onid_sid %08x not found\n", onid_sid);
				return -3;
			}
		}
		else
		{
			if (allchans_tv.count(onid_sid) > 0)
			{
				cit = allchans_tv.find(onid_sid);
				nvodname = cit->second.name;
			}
			else
			{
				dprintf("[zapit] onid_sid %08x not found\n", onid_sid);
				return -3;
			}
		}
	}

	dprintf("[zapit] stop decoding\n");

	if (video_fd >= 0)
	{
		ioctl(video_fd, VIDEO_STOP, (boolean)1);
		close(video_fd);
		video_fd = -1;
	}

	dprintf("[zapit] stop video queue\n");
	
	if (dmx_video_fd >= 0)
	{
		ioctl(dmx_video_fd, DMX_STOP, 0);
		close(dmx_video_fd);
		dmx_video_fd = -1;
	}

	dprintf("[zapit] stop audio queue\n");
	
	if (dmx_audio_fd >= 0)
	{
		ioctl(dmx_audio_fd, DMX_STOP,0);
		close(dmx_audio_fd);
		dmx_audio_fd = -1;
	}

	if (cit->second.tsid != old_tsid)
	{
		new_tsid = true;
#ifndef USE_EXTERNAL_CAMD
		do_cam_reset = true;
#endif /* USE_EXTERNAL_CAMD */
		dprintf("[zapit] tunig to tsid %04x\n", cit->second.tsid);
		
		if (tune(cit->second.tsid) < 0)
		{
			dprintf("[zapit] no transponder with tsid %04x found\nHave to look it up in nit\n", cit->second.tsid);
			return -3;
		}
		
		do_search_emmpid = true;
	}
	else
	{
		do_search_emmpid = false;
	}

	if (cit->second.service_type == 4)
	{
		current_is_nvod = true;
		//nvodname = cit->second.name;
		curr_onid_sid = onid_sid;
		save_settings();
		return 3;
	}

	if ((cit->second.pmt == 0) && (cit->second.service_type != 4))
	{
		dprintf("[zapit] trying to find pmt for sid %04x, onid %04x\n", cit->second.sid, cit->second.onid);
		
		if (in_nvod)
		{
			pat(cit->second.onid, &nvodchannels);
		}
		else if (Radiomode_on)
		{
			pat(cit->second.onid, &allchans_radio);
		}
		else
		{
			pat(cit->second.onid, &allchans_tv);
		}
	}

#ifndef USE_EXTERNAL_CAMD
	if (caid == 0)
	{
		caid = get_caid();
	}
#else
	// sucks
	caid = 0x1702;
#endif /* USE_EXTERNAL_CAMD */

	memset(&parse_pmt_pids, 0, sizeof(parse_pmt_pids));
	parse_pmt_pids = parse_pmt(cit->second.pmt, caid);

	//printf("VPID parsed from pmt: %x\n", parse_pmt_pids.vpid);
	//for (i = 0;i<parse_pmt_pids.count_apids;i++)
	//{
	//	printf("Audio-PID %d from parse_pmt() is: %x\n", i, parse_pmt_pids.apid[i]);
	//}

	if (parse_pmt_pids.count_vpids > 0)
	{
		cit->second.vpid = parse_pmt_pids.vpid;
		
		if (cit->second.vpid == 0)
		{
			cit->second.vpid = 0x1fff;
		}
		
		//cit->second.last_update = current_time;
	}
	else
	{
		dprintf("[zapit] using standard-pids (this probably fails...)\n");
	}

	if (parse_pmt_pids.count_apids >0)
	{
		cit->second.apid = parse_pmt_pids.apids[0].pid;
		//cit->second.last_update = current_time;
	}

	//if (parse_pmt_pids.ecmpid != 0)
	{
		cit->second.ecmpid = parse_pmt_pids.ecmpid;
		//cit->second.last_update = current_time;
	}

#ifndef DVBS
	if (in_nvod)
	{
		lcdd.setServiceName(nvodname);
	}
	else
	{
		lcdd.setServiceName(cit->second.name);
	}
#endif /* DVBS */

	if ((cit->second.vpid != 0x1fff) || (cit->second.apid != 0x8191))
	{
		pids_desc = parse_pmt_pids;

		Vpid = cit->second.vpid;
		Apid = cit->second.apid;
		Pmt = cit->second.pmt;

		dprintf("[zapit] zapping to sid: %04x %s. VPID: 0x%04x. APID: 0x%04x, PMT: 0x%04x\n", cit->second.sid, cit->second.name.c_str(), cit->second.vpid, cit->second.apid, cit->second.pmt);

#ifdef USE_EXTERNAL_CAMD
		printf("pmt: %04x, lastpmt: %04x, new_tsid: %d\n", Pmt, lastpmt, new_tsid);
		if ((Pmt != lastpmt) || (new_tsid == true))
		{
			lastpmt = Pmt;

			if (camdpid != -1)
			{
				kill(camdpid, SIGKILL);
				waitpid(camdpid, 0, 0);
				camdpid = -1;
			}

			switch ((camdpid = fork()))
			{
			case -1:
				perror("[zapit] unable to fork() for camd");
				break;
			case 0:
				sprintf(vpidbuf, "%x", Vpid);
				sprintf(apidbuf, "%x", Apid);
				sprintf(pmtpidbuf, "%x", Pmt);
				if (execlp("camd", "camd", vpidbuf, apidbuf, pmtpidbuf, NULL) < 0)
				{
					perror("[zapit] camd");
					exit(0);
				}
				break;
			default:
				dprintf("[zapit] camd pid: %d\n", camdpid);
				break;
			}
		}
#else
		if ((cit->second.ecmpid > 0) && (cit->second.ecmpid < no_ecmpid_found))
		{
			decode_vals *vals = (decode_vals*) malloc(sizeof(decode_vals));
			vals->onid = cit->second.onid;
			vals->tsid = cit->second.tsid;
			vals->ecmpid = cit->second.ecmpid;
			vals->parse_pmt_pids = &parse_pmt_pids;
			vals->do_search_emmpids = do_search_emmpid;
			vals->do_cam_reset = do_cam_reset;

			pthread_create(&dec_thread, 0,decode_thread, (void*) vals);
		}
#endif /* USE_EXTERNAL_CAMD */

		dprintf("[zapit] open decoder\n");

		if ((video_fd = open(VIDEO_DEV, O_RDWR)) < 0)
		{
			perror("[zapit] unable to open video device");
			exit(1);
		}
		
		dprintf("[zapit] start decoding\n");

		if (ioctl(video_fd, VIDEO_SELECT_SOURCE, (videoStreamSource_t)VIDEO_SOURCE_DEMUX) < 0)
		{
			perror("[zapit] unable to select video source");
		}

		if (ioctl(video_fd, VIDEO_PLAY) < 0)
		{
			perror("[zapit] unable to play video");
		}

		dprintf("[zapit] set audio mode\n");
		
		if (parse_pmt_pids.apids[0].is_ac3 != OldAC3)
		{
			OldAC3 = parse_pmt_pids.apids[0].is_ac3;
			
			if ((ac3d = open(AUDIO_DEV, O_RDWR)) < 0)
			{
				perror("[zapit] unable to open audio device");
				exit(1);
			}
			else
			{
				if(ioctl(ac3d, AUDIO_SET_BYPASS_MODE, ( OldAC3 )?0:1) < 0)
				{
					perror("[zapit] unable to set audio mode");
				}

				close(ac3d);
			}
		}

		if ((dmx_video_fd = open(DEMUX_DEV, O_RDWR)) < 0)
		{
			perror("[zapit] unable to open demux device");
			exit(1);
		}

		dprintf("[zapit] setting vpid %04x\n", Vpid);

		/* vpid */
		pes_filter.pid = Vpid;
		pes_filter.input = DMX_IN_FRONTEND;
		pes_filter.output = DMX_OUT_DECODER;
		pes_filter.pesType = DMX_PES_VIDEO;
		pes_filter.flags = 0;
		
		if (ioctl(dmx_video_fd, DMX_SET_PES_FILTER, &pes_filter) < 0)
		{
			perror("[zapit] unable to set dmx pes filter");
			exit(1);
		}
		else
		{
			vpid = Vpid;
		}

		if((dmx_audio_fd = open(DEMUX_DEV, O_RDWR)) < 0)
		{
			perror("[zapit] unable open demux device");
			exit(1);
		}

		dprintf("[zapit] setting apid %04x\n", Apid);
		
		/* apid */
		pes_filter.pid     = Apid;
		pes_filter.input   = DMX_IN_FRONTEND;
		pes_filter.output  = DMX_OUT_DECODER;
		pes_filter.pesType = DMX_PES_AUDIO;
		pes_filter.flags   = 0;
		
		if (ioctl(dmx_audio_fd, DMX_SET_PES_FILTER, &pes_filter) < 0)
		{
			perror("[zapit] unable to set dmx pes filter");
			exit(1);
		}
		else
		{
			apid = Apid;
		}

		dprintf("[zapit] starting dmx\n");
		

		if (ioctl(dmx_audio_fd, DMX_START, 0) < 0)
		{
			perror("[zapit] unable to start audio demux");
		}

		if (ioctl(dmx_video_fd, DMX_START ,0) < 0)
		{
			perror("[zapit] unable to start video demux");
		}
		
		dprintf("[zapit] playing\n");


#ifndef DVBS
		//if (parse_pmt_pids.vtxtpid != 0)
		//{
			dprintf("[zapit] setting vtxt\n");
			set_vtxt(parse_pmt_pids.vtxtpid);
		//}
#endif /* DVBS */
		dprintf("[zapit] saving settings\n");
		curr_onid_sid = onid_sid;
		
		if (!in_nvod)
		{
			save_settings();
		}
#ifndef USE_EXTERNAL_CAMD
		//wait for decode_thread to exit
		dprintf("[zapit] waiting for decode_thread\n");
		pthread_join(dec_thread,0);
#endif /* DVBS */
	}
	else
	{
		dprintf("[zapit] not a channel. Won´t zap\n");
		return -3;
	}

	return 3;
}


int numzap(int channel_number) {
  std::map<uint, uint>::iterator cit;

  nvodchannels.clear();
  if (Radiomode_on)
    {
      if (allnumchannels_radio.count(channel_number) == 0)
	{
	  dprintf("[zapit] given channel not found");
	  return -2;
	}
      cit  = allnumchannels_radio.find(channel_number);
    }
  else
    {
      if (allnumchannels_tv.count(channel_number) == 0)
	{
	  dprintf("[zapit] given channel not found");
	  return -2;
	}
      cit = allnumchannels_tv.find(channel_number);
    }

  dprintf("[zapit] zapping to onid_sid %04x\n", cit->second);
  if (zapit(cit->second,false) > 0)
    return 2;
  else
    return -2;
}


int namezap(std::string channel_name) {
  std::map<std::string,uint>::iterator cit;

  nvodchannels.clear();
  // Search current channel
  if (Radiomode_on)
    {
      if (allnamechannels_radio.count(channel_name) == 0)
	{
	  dprintf("[zapit] given channel not found");
	  return -2;
	}
      cit  = allnamechannels_radio.find(channel_name);
    }
  else
    {
      if (allnamechannels_tv.count(channel_name) == 0)
	{
	  dprintf("[zapit] given channel not found");
	  return -2;
	}
      cit = allnamechannels_tv.find(channel_name);
    }

  dprintf("[zapit] zapping to onid_sid %04x\n", cit->second);
  if (zapit(cit->second,false) > 0)
    return 3;
  else
    return -3;
}

int changeapid(ushort pid_nr)
{
	int ac3d;

	struct dmxPesFilterParams pes_filter;
	std::map<uint,channel>::iterator cit;

	if (current_is_nvod)
	{
		cit = nvodchannels.find(curr_onid_sid);
	}
	else
	{
		if (Radiomode_on)
		{
			cit = allchans_radio.find(curr_onid_sid);
		}
		else
		{
			cit = allchans_tv.find(curr_onid_sid);
		}
	}

	if (pid_nr <= pids_desc.count_apids)
	{
		//if (dmx_audio_fd >= 0)
		//{
		//	close(dmx_audio_fd);
		//	dmx_audio_fd = -1;
		//}
#ifndef USE_EXTERNAL_CAMD
		//descramble(0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff);

		//if ((cit->second.ecmpid > 0) && (cit->second.ecmpid < no_ecmpid_found))
		//{
			//cam_reset();
			//descramble(cit->second.onid, cit->second.tsid, 0x104, caid, cit->second.ecmpid, pids_desc.apids[pid_nr].pid, vpid);
		//}
#endif /* DVBS */

		if ((dmx_audio_fd == -1) && ((dmx_audio_fd = open(DEMUX_DEV, O_RDWR)) < 0))
		{
			perror("[zapit] unable to open demux device");
			exit(1);
		}

		/* apid */
		pes_filter.pid = pids_desc.apids[pid_nr].pid;
		pes_filter.input = DMX_IN_FRONTEND;
		pes_filter.output = DMX_OUT_DECODER;
		pes_filter.pesType = DMX_PES_AUDIO;
		pes_filter.flags = 0;

		if (ioctl(dmx_audio_fd, DMX_STOP, 0) < 0)
		{
			perror("[zapit] unable to stop audio");
		}

		if (ioctl(dmx_audio_fd, DMX_SET_PES_FILTER, &pes_filter) < 0)
		{
			perror("[zapit] unable to set demux pes filter");
		}
		
		apid = pids_desc.apids[pid_nr].pid;

		if (pids_desc.apids[pid_nr].is_ac3 != OldAC3)
		{
			OldAC3 = pids_desc.apids[pid_nr].is_ac3;
			
			if ((ac3d = open(AUDIO_DEV, O_RDWR)) < 0)
			{
				perror("[zapit] unable to open audio device");
				exit(1);
			}
			else
			{
				dprintf("Setting audiomode to %d", ( OldAC3 )?0:1);
				
				if (ioctl(ac3d, AUDIO_SET_BYPASS_MODE, (OldAC3)?0:1) < 0)
				{
					perror("[zapit] unable to set audio mode");
				}
				
				close(ac3d);
			}
		}

		if (ioctl(dmx_audio_fd, DMX_START, 0) < 0)
		{
			perror("[zapit] unable to start audio");
			return -8;
		}
		else
		{
			return 8;
		}
	}
	else
	{
		return -8;
	}
}


#if 0
void display_pic()
{
	struct videoDisplayStillPicture sp;
	char *picture;
	int pic_fd;
	struct stat pic_desc;
	
	if ((pic_fd = open("/root/ilogo.mpeg", O_RDONLY)) < 0)
	{
		perror("open still picture");
		exit(0);
	}

	if((video_fd == -1) && ((video_fd = open(VIDEO_DEV, O_RDWR|O_NONBLOCK)) < 0))
	{
		perror("[zapit] unable to open video device");
		exit(1);
	}

	fstat(pic_fd, &pic_desc);

	sp.iFrame = (char *) malloc(pic_desc.st_size);
	sp.size = pic_desc.st_size;
	printf("I-frame size: %d\n", sp.size);

	if(!sp.iFrame)
	{
		printf("No memory for I-Frame\n");
		exit(0);
	}

	printf("read: %d bytes\n",read(pic_fd,sp.iFrame,sp.size));

	if (ioctl(video_fd, VIDEO_STOP, (boolean)0) < 0)
	{
		perror("VIDEO_STOP");
		//exit(0);
	}

	if (ioctl(video_fd, VIDEO_STILLPICTURE, sp) < 0))
	{
		perror("VIDEO STILLPICTURE");
		//exit(0);
	}

	sleep(3);

	if (ioctl(video_fd, VIDEO_PLAY) < 0)
	{
		perror("VIDEO PLAY");
		//exit(0);
	}

	return;
}
#endif

void endzap()
{
	if (video_fd >= 0)
	{
		ioctl(video_fd, VIDEO_STOP, (boolean)1);
		close(video_fd);
		video_fd = -1;
	}

	if (dmx_video_fd >= 0)
	{
		close(dmx_video_fd);
		dmx_video_fd = -1;
	}

	if (dmx_audio_fd >= 0)
	{
		close(dmx_audio_fd);
		dmx_audio_fd = -1;
	}

	return;
}


void shutdownBox()
{
	if (execlp("/sbin/halt", "/sbin/halt", 0) < 0)
	{
		perror("[zapit] unable to execute halt");
	}

	return;
}

int sleepBox()
{
	int device;
	if((device = open(FRONT_DEV, O_RDWR)) < 0)
	{
		perror("[zapit] unable to open frontend device");
		return -4;
	}

#ifdef OLD_TUNER_API
	if (ioctl(device, OST_SET_POWER_STATE, OST_POWER_SUSPEND) != 0)
#else
	if (ioctl(device, FE_SET_POWER_STATE, FE_POWER_SUSPEND) != 0)
#endif
	{
		perror("[zapit] unable to set suspend-mode");
		return -4;
	}

	// why 4 or -4? - obi
	return(4);
}

void setRadioMode()
{
	dprintf("[zapit] switching to radio-mode\n");
	Radiomode_on = true;
	return;
}

void setTVMode()
{
	Radiomode_on = false;
	return;
}

int prepare_channels()
{
	int ls;
	
	std::map<uint, uint>::iterator numit;
	std::map<std::string, uint>::iterator nameit;
	std::map<uint, channel>::iterator cit;

	// for the case this function is NOT called for the first time (by main())
	// we clear all cannel lists, they are refilled
	// by LoadServices() and LoadBouquets()
	allnumchannels_tv.clear();
	allnumchannels_radio.clear();
	allnamechannels_tv.clear();
	allnamechannels_radio.clear();
	transponders.clear();
	allchans_tv.clear();
	numchans_tv.clear();
	namechans_tv.clear();
	allchans_radio.clear();
	numchans_radio.clear();
	namechans_radio.clear();
	//found_transponders = 0;
	//found_channels = 0;

	g_BouquetMan->clearAll();

	ls = LoadServices();
	g_BouquetMan->loadBouquets();
	g_BouquetMan->renumServices();

	// wtf is 23? obi
	return 23;
}

void start_scan(unsigned short do_diseqc)
{
	int vid;

	transponders.clear();
	namechans_tv.clear();
	numchans_tv.clear();
	namechans_radio.clear();
	numchans_radio.clear();
	allchans_tv.clear();
	allchans_radio.clear();
	allnumchannels_tv.clear();
	allnumchannels_radio.clear();
	allnamechannels_tv.clear();
	allnamechannels_radio.clear();
	found_transponders = 0;
	found_channels = 0;

#ifndef DVBS
	// stop teletext
	set_vtxt(0);
#endif /* DVBS */

	if (video_fd >= 0)
	{
		ioctl(video_fd, VIDEO_STOP, (boolean)1);
		close(video_fd);
		video_fd = -1;
	}

	if (dmx_video_fd >= 0)
	{
		ioctl(dmx_video_fd, DMX_STOP, 0);
		close(dmx_video_fd);
		dmx_video_fd = -1;
	}

	if (dmx_audio_fd >= 0)
	{
		ioctl(dmx_audio_fd, DMX_STOP, 0);
		close(dmx_audio_fd);
		dmx_audio_fd = -1;
	}

	if (pthread_create(&scan_thread, 0, start_scanthread, &do_diseqc))
	{
		perror("[zapit] pthread_create: scan_thread");
		exit(1);
	}

	while (scan_runs == 0)
	{
		printf("[zapit] waiting for scan to start\n");
	}

	return;
}

void parse_command()
{
  char *status;
  short carsten;
  std::map<uint,uint>::iterator sit;
  std::map<uint,channel>::iterator cit;
  int number = 0;
  int caid_ver = 0;
  //printf ("parse_command\n");

  //byteorder!!!!!!
  //rmsg.param2 = ((rmsg.param2 & 0x00ff) << 8) | ((rmsg.param2 & 0xff00) >> 8);

  /*
    printf("Command received\n");
    printf("  Version: %d\n", rmsg.version);
    printf("  Command: %d\n", rmsg.cmd);
    printf("  Param: %c\n", rmsg.param);
    printf("  Param2: %d\n", rmsg.param2);
    printf("  Param3: %s\n", rmsg.param3);
  */

  if(rmsg.version==1)
  {
  switch (rmsg.cmd)
    {
    case 1:
      dprintf("[zapit] zapping by number\n");
      if (numzap( atoi((const char*) &rmsg.param) ) > 0)
      	status = "001";
      else
      	status = "-01";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) <0) {
	perror("[zapit] could not send any return\n");
	return;
      }
      break;
    case 2:
      dprintf("[zapit] killing zap\n");
      endzap();
      status = "002";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) <0) {
	perror("[zapit] could not send any return\n");
	return;
      }
      break;
    case 3:
      dprintf("[zapit] zapping by name\n");
      if (namezap(rmsg.param3) == 3)
      	status = "003";
      else
      	status = "-03";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) <0) {
	perror("[zapit] could not send any return\n");
	return;
      }
      break;
    case 4:
      status = "004";

      dprintf("[zapit] shutdown\n");
      shutdownBox();
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) <0) {
	perror("[zapit] could not send any return\n");
	return;
      }
      break;
    case 5:
      if (Radiomode_on)
    	{
	  if (!allchans_radio.empty())
	    {
	      status = "005";
	      if (send(connfd, status, strlen(status),0) == -1) {
		perror("[zapit] could not send any return\n");
		return;
	      }
	      for (sit = allnumchannels_radio.begin(); sit != allnumchannels_radio.end(); sit++)
		{
		  cit = allchans_radio.find(sit->second);
		  channel_msg chanmsg;
		  strncpy(chanmsg.name, cit->second.name.c_str(),29);
		  chanmsg.chan_nr = sit->first;
		  chanmsg.mode = 'r';
		  if (send(connfd, &chanmsg, sizeof(chanmsg),0) == -1)
		    {
		      perror("[zapit] could not send any return\n");
		      return;
		    }
		}
	    }
	  else
	    {
	      printf("[zapit] radio_channellist is empty\n");
	      status = "-05";
	      if (send(connfd, status, strlen(status),0) == -1)
	       {
		 perror("[zapit] could not send any return\n");
		 return;
	       }
	    }
      	} else {
	  if (!allchans_tv.empty())
	    {
	    status = "005";
	    if (send(connfd, status, strlen(status),0) == -1)
	      {
		perror("[zapit] could not send any return\n");
		return;
	      }
	    for (sit = allnumchannels_tv.begin(); sit != allnumchannels_tv.end(); sit++)
		{
		  cit = allchans_tv.find(sit->second);
		  channel_msg chanmsg;
		  strncpy(chanmsg.name, cit->second.name.c_str(),29);
		  chanmsg.chan_nr = sit->first;
		  chanmsg.mode = 't';
		  if (send(connfd, &chanmsg, sizeof(chanmsg),0) == -1)
		    {
		      perror("[zapit] could not send any return\n");
		      return;
		    }
		}
	    }
	  else
	    {
	      printf("[zapit] tv_channellist is empty\n");
	      status = "-05";
	      if (send(connfd, status, strlen(status),0) == -1)
		{
		  perror("[zapit] could not send any return\n");
		  return;
		}
	    }
	}
      usleep(200000);
      break;
    case 6:
      status = "006";
      setRadioMode();
      if (!allchans_radio.empty())
      	status = "-06";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("[zapit] could not send any return\n");
	return;
      }
      break;
    case 7:
      status = "007";
      setTVMode();
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("[zapit] could not send any return\n");
	return;
      }
      break;
    case 8:
      status = "008";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("[zapit] could not send any return\n");
	return;
      }
      if (send(connfd, &pids_desc , sizeof(pids),0) == -1) {
	perror("[zapit] could not send any return\n");
	return;
      }
      break;
    case 9:
      if (changeapid(atoi((const char*) &rmsg.param)) > 0)
	status = "009";
      else
	status = "-09";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("[zapit] could not send any return\n");
	return;
      }
      break;
    case 'a':
      status = "00a";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("[zapit] could not send any return\n");
	return;
      }
      {
	channel_msg settings = load_settings();
      	if (send(connfd, &settings, sizeof(settings),0) == -1) {
	  perror("[zapit] could not send any return\n");
	  return;
	}
      }
      break;
    case 'b':
      if (Radiomode_on)
	cit = allchans_radio.find(curr_onid_sid);
      else
       	if (current_is_nvod)
        	cit = nvodchannels.find(curr_onid_sid);
      	else
		cit =allchans_tv.find(curr_onid_sid);

      if (curr_onid_sid == 0)
	{
	  status = "-0b";
	  break;
	}
      else
	status = "00b";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("[zapit] could not send any return\n");
	return;
      }
      carsten = (short) cit->second.vpid;
      if (send(connfd, &carsten, 2,0) == -1) {
	perror("[zapit] could not send any return\n");
	return;
      }
      carsten = (short) cit->second.apid;
      if (send(connfd, &carsten, 2,0) == -1) {
	perror("[zapit] could not send any return\n");
	return;
      }
      break;

	case 'c':
		if (Radiomode_on)
		{
			if (!allchans_radio.empty())
			{
				status = "00c";
				if (send(connfd, status, strlen(status),0) == -1)
				{
					perror("[zapit] could not send any return\n");
					return;
				}
				for ( CBouquetManager::radioChannelIterator radiocit = g_BouquetMan->radioChannelsBegin(); radiocit != g_BouquetMan->radioChannelsEnd(); radiocit++)
				{
					channel_msg_2 chanmsg;

					strncpy(chanmsg.name, (*radiocit)->name.c_str(),30);
					chanmsg.chan_nr = (*radiocit)->chan_nr;
					chanmsg.onid_tsid = (*radiocit)->OnidSid();

					if (send(connfd, &chanmsg, sizeof(chanmsg),0) == -1)
					{
						perror("[zapit] could not send any return\n");
						return;
					}
				}
			}
			else
			{
				printf("[zapit] tv_channellist is empty\n");
				status = "-0c";
				if (send(connfd, status, strlen(status),0) == -1)
				{
					perror("[zapit] could not send any return\n");
					return;
				}
			}
		}
		else
		{
			if (!allchans_tv.empty())
			{
				status = "00c";
				if (send(connfd, status, strlen(status),0) == -1)
				{
					perror("[zapit] could not send any return\n");
					return;
				}
				for ( CBouquetManager::tvChannelIterator tvcit = g_BouquetMan->tvChannelsBegin(); tvcit != g_BouquetMan->tvChannelsEnd(); tvcit++)
				{
					channel_msg_2 chanmsg;
					strncpy(chanmsg.name, (*tvcit)->name.c_str(),30);
					chanmsg.chan_nr = (*tvcit)->chan_nr;
					chanmsg.onid_tsid = (*tvcit)->OnidSid();
					if (send(connfd, &chanmsg, sizeof(chanmsg),0) == -1)
					{
						perror("[zapit] could not send any return\n");
						return;
					}
				}
			}
			else
			{
				printf("tv_channellist is empty\n");
				status = "-0c";
				if (send(connfd, status, strlen(status),0) == -1)
				{
					perror("[zapit] could not send any return\n");
					return;
				}
			}
		}
	break;

    case 'd':
        dprintf("[zapit] zapping by number\n");
        number = 0;
        sscanf((const char*) &rmsg.param3, "%x", &number);
        char m_status[4];

        if (zapit(number,false) > 0)
        {
            strcpy(m_status, "00d");
            m_status[1]= pids_desc.count_apids & 0x0f;
            if ( current_is_nvod )
                m_status[1]|= zapped_chan_is_nvod;
/*            if ( pids_desc.ecmpid != 0)
                m_status[1]|= 0x40;
*/
        }
        else
            strcpy(m_status, "-0d");

        //printf("zapit is sending back a status-msg %s\n", status);
        if (send(connfd, m_status, 3, 0) <0)
        {
            perror("[zapit] could not send any return\n");
            return;
        }
        if (send(connfd, &pids_desc , sizeof(pids),0) == -1)
        {
            perror("[zapit] could not send any return\n");
            return;
        }
        break;
    case 'e':
        dprintf("[zapit] changing nvod\n");
        number = 0;
        sscanf((const char*) &rmsg.param3, "%x", &number);

        if (zapit(number,true) > 0)
        {
            strcpy(m_status, "00e");
            m_status[1]= pids_desc.count_apids & 0x0f;
            if ( current_is_nvod )
                m_status[1]|= zapped_chan_is_nvod;
/*            if ( pids_desc.ecmpid != 0)
                m_status[1]|= 0x40;
*/
        }
        else
            strcpy(m_status, "-0e");

        //printf("zapit is sending back a status-msg %s\n", status);
        if (send(connfd, m_status, 3, 0) <0)
        {
            perror("[zapit] could not send any return\n");
            return;
        }
        if (send(connfd, &pids_desc , sizeof(pids),0) == -1)
        {
            perror("[zapit] could not send any return\n");
            return;
        }
        break;
    case 'f':
      if (current_is_nvod)
	status = "00f";
      else
	status = "-0f";
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("[zapit] could not send any return\n");
	return;
      }
      if (current_is_nvod)
	{
	  number = 1;
	  for (cit = nvodchannels.begin(); cit != nvodchannels.end(); cit++)
	    {
	      channel_msg_2 chanmsg;
	      strncpy(chanmsg.name, cit->second.name.c_str(),30);
	      chanmsg.chan_nr = number++;
	      chanmsg.onid_tsid = (cit->second.onid<<16)|cit->second.sid;

	      if (send(connfd, &chanmsg, sizeof(chanmsg),0) == -1)
		{
		  perror("[zapit] could not send any return\n");
		  return;
		}
	    }
	}
      break;
      case 'g':
        start_scan(rmsg.param2);

      	status = "00g";
      	//dprintf("[zapit] sending back a staus %s\n", status);
      	if (send(connfd, status, strlen(status),0) == -1) {
		perror("[zapit] could not send any return\n");
		return;
	}
      break;
      case 'h':
      	if (scan_runs>0)
      		status = "00h";
      	else
      		status = "-0h";
      	if (send(connfd, status, strlen(status),0) == -1) {
		perror("[zapit] could not send any return\n");
		return;
	}
		if (send(connfd, &curr_sat, sizeof(short),0) == -1)
		{
		perror("[zapit] could not send any return\n");
		return;
		}
		if (send(connfd, &found_transponders, sizeof(int),0) == -1)
		{
		perror("[zapit] could not send any return\n");
		return;
		}
		if (send(connfd, &found_channels, sizeof(int),0) == -1) {
		perror("[zapit] could not send any return\n");
		return;
	}
	break;
       case 'i':
       uint nvod_onidsid;
       ushort  nvod_tsid, cnt_nvods;
       //quick hack
       current_is_nvod= true;

       	if (current_is_nvod)
       		status = "00i";
       	else
       		status = "-0i";
       	if (send(connfd, status, strlen(status),0) == -1)
		{
		  perror("[zapit] could not send any return\n");
		  return;
		}

    if (recv(connfd, &cnt_nvods, 2,0)==-1)
    {
        perror("[zapit] receiving nvod_channels cnt_nvods?");
        return;
    }
    else
    {
        dprintf("[zapit] receiving nvods (%d)\n", cnt_nvods);
        for (int cnt= 0; cnt<cnt_nvods; cnt++)
        {
            if (recv(connfd, &nvod_onidsid, 4,0)==-1)
			{
				perror("[zapit] receiving nvod_channels");
				return;
			}
    		if (recv(connfd, &nvod_tsid, 2,0) == -1)
			{
				perror("[zapit] receiving nvod_channels");
				return;
			}
    		//printf("Received onid_sid %x. tsid: %x, sid: %x, onid: %x\n", nvod_onidsid, nvod_tsid, (nvod_onidsid&0xFFFF), (nvod_onidsid>>16));
    		nvodchannels.insert(std::pair<int,channel>(nvod_onidsid,channel("NVOD",0,0,0,0,0,(nvod_onidsid&0xFFFF),nvod_tsid,(nvod_onidsid>>16),1)));
        }

    }
	break;

	case 'p':
		status = "00p";
		prepare_channels();
		if (send(connfd, status, strlen(status),0) == -1) {
			perror("[zapit] could not send any return\n");
			return;
		}
		break;


	case 'q' :
		sendBouquetList();
		return;
	break;

	case 'r' :
		sendChannelListOfBouquet(rmsg.param);
	break;
	case 's':
		status = "00s";
		if (send(connfd, status, strlen(status),0) == -1) {
			perror("[zapit] could not send any return\n");
			return;
		}
		if (send(connfd, &curr_onid_sid, sizeof(uint),0) == -1) {
			perror("[zapit] could not send any return\n");
			return;
		}
		break;
	case 't':
		status = "00t";

		switch (caid)
		{
			case 0x1722 : caid_ver = 1;
				break;
			case 0x1702 : caid_ver = 2;
				break;
			case 0x1762 : caid_ver = 4;
				break;
			default : caid_ver = 8;
		}
		caid_ver |= caver;
		if (send(connfd, status, strlen(status),0) == -1) {
			perror("[zapit] could not send any return\n");
			return;
		}
		if (send(connfd, &caid_ver, sizeof(int),0) == -1) {
			perror("[zapit] could not send any return\n");
			return;
		}
		break;
	case 'u':
		status = "00u";
		if (send(connfd, status, strlen(status),0) == -1) {
			perror("[zapit] could not send any return\n");
			return;
		}
		if (send(connfd, &vtxt_pid, sizeof(int),0) == -1) {
			perror("[zapit] could not send any return\n");
			return;
		}
		break;
	default:
      status = "000";
      //printf("zapit is sending back a status-msg %s\n", status);
      if (send(connfd, status, strlen(status),0) == -1) {
	perror("[zapit] could not send any return\n");
	return;
      }
      printf("[zapit] unknown command\n");
    }
	} // if (rmsg.version==1)




/********************************************/
/*                                          */
/*  new command handling via CZapitClient   */
/*                                          */
/********************************************/

	else if (rmsg.version == CZapitClient::ACTVERSION)
	{
		printf("[zapit] command version 2\n");
		CZapitClient::responseCmd response;
		switch( rmsg.cmd)
		{
			case CZapitClient::CMD_ZAPTO :
				CZapitClient::commandZapto msgZapto;
				read( connfd, &msgZapto, sizeof(msgZapto));
				zapTo(msgZapto.bouquet, msgZapto.channel);
			break;

			case CZapitClient::CMD_ZAPTO_CHANNELNR :
				CZapitClient::commandZaptoChannelNr msgZaptoChannelNr;
				read( connfd, &msgZaptoChannelNr, sizeof(msgZaptoChannelNr));
				zapTo(msgZaptoChannelNr.channel);
			break;

			case CZapitClient::CMD_GET_BOUQUETS :
				CZapitClient::commandGetBouquets msgGetBouquets;
				read( connfd, &msgGetBouquets, sizeof(msgGetBouquets));
				sendBouquets(msgGetBouquets.emptyBouquetsToo);
			break;

			case CZapitClient::CMD_GET_BOUQUET_CHANNELS :
				CZapitClient::commandGetBouquetChannels msgGetBouquetChannels;
				read( connfd, &msgGetBouquetChannels, sizeof(msgGetBouquetChannels));
				sendBouquetChannels(msgGetBouquetChannels.bouquet, msgGetBouquetChannels.mode);
			break;

			case CZapitClient::CMD_GET_CHANNELS :
				CZapitClient::commandGetChannels msgGetChannels;
				read( connfd, &msgGetChannels, sizeof(msgGetChannels));
				sendChannels( msgGetChannels.mode, msgGetChannels.order);
			break;

			case CZapitClient::CMD_RESTORE_BOUQUETS :
				g_BouquetMan->restoreBouquets();
				response.cmd = CZapitClient::CMD_READY;
				send(connfd, &response, sizeof(response), 0);
			break;

			case CZapitClient::CMD_REINIT_CHANNELS :
				prepare_channels();
				response.cmd = CZapitClient::CMD_READY;
				send(connfd, &response, sizeof(response), 0);
			break;

			case CZapitClient::CMD_BQ_ADD_BOUQUET :
				CZapitClient::commandAddBouquet msgAddBouquet;
				read( connfd, &msgAddBouquet, sizeof(msgAddBouquet));
				g_BouquetMan->addBouquet(msgAddBouquet.name);
			break;

			case CZapitClient::CMD_BQ_DELETE_BOUQUET :
				CZapitClient::commandDeleteBouquet msgDeleteBouquet;
				read( connfd, &msgDeleteBouquet, sizeof(msgDeleteBouquet));
				g_BouquetMan->deleteBouquet(msgDeleteBouquet.bouquet-1);
			break;

			case CZapitClient::CMD_BQ_RENAME_BOUQUET :
				CZapitClient::commandRenameBouquet msgRenameBouquet;
				read( connfd, &msgRenameBouquet, sizeof(msgRenameBouquet));
				g_BouquetMan->Bouquets[msgRenameBouquet.bouquet-1]->Name = msgRenameBouquet.name;
			break;

			case CZapitClient::CMD_BQ_MOVE_BOUQUET :
				CZapitClient::commandMoveBouquet msgMoveBouquet;
				read( connfd, &msgMoveBouquet, sizeof(msgMoveBouquet));
				g_BouquetMan->moveBouquet(msgMoveBouquet.bouquet-1, msgMoveBouquet.newPos-1);
			break;

			case CZapitClient::CMD_BQ_ADD_CHANNEL_TO_BOUQUET :
				CZapitClient::commandAddChannelToBouquet msgAddChannelToBouquet;
				read( connfd, &msgAddChannelToBouquet, sizeof(msgAddChannelToBouquet));
				addChannelToBouquet(msgAddChannelToBouquet.bouquet, msgAddChannelToBouquet.onid_sid);
			break;

			case CZapitClient::CMD_BQ_REMOVE_CHANNEL_FROM_BOUQUET :
				CZapitClient::commandRemoveChannelFromBouquet msgRemoveChannelFromBouquet;
				read( connfd, &msgRemoveChannelFromBouquet, sizeof(msgRemoveChannelFromBouquet));
				removeChannelFromBouquet(msgRemoveChannelFromBouquet.bouquet, msgRemoveChannelFromBouquet.onid_sid);
			break;

			case CZapitClient::CMD_BQ_MOVE_CHANNEL :
				CZapitClient::commandMoveChannel msgMoveChannel;
				read( connfd, &msgMoveChannel, sizeof(msgMoveChannel));
				g_BouquetMan->Bouquets[ msgMoveChannel.bouquet-1]->moveService(
					msgMoveChannel.oldPos-1,
					msgMoveChannel.newPos-1,
					((Radiomode_on && msgMoveChannel.mode == CZapitClient::MODE_CURRENT ) || (msgMoveChannel.mode==CZapitClient::MODE_RADIO)) ? 2 : 1);
			break;

			case CZapitClient::CMD_BQ_RENUM_CHANNELLIST :
				g_BouquetMan->renumServices();
			break;

			case CZapitClient::CMD_BQ_SAVE_BOUQUETS :
				g_BouquetMan->saveBouquets();
				response.cmd = CZapitClient::CMD_READY;
				send(connfd, &response, sizeof(response), 0);
			break;

			case CZapitClient::CMD_SB_START_PLAYBACK :
				startPlayBack();
			break;

			case CZapitClient::CMD_SB_STOP_PLAYBACK :
				stopPlayBack();
			break;

			default:
				printf("[zapit] unknown command (version %d)\n", CZapitClient::ACTVERSION);
		}
	}
	else
	{
		perror("[zapit] unknown cmd version\n");
		return;
	}
}

void sendBouquetList()
{
	char* status = "00q";
	if (send(connfd, status, strlen(status),0) == -1)
	{
		perror("[zapit] could not send any return\n");
		return;
	}

	uint nBouquetCount = 0;
	for (uint i=0; i<g_BouquetMan->Bouquets.size(); i++)
	{
		if ( (Radiomode_on) && (g_BouquetMan->Bouquets[i]->radioChannels.size()> 0) ||
			!(Radiomode_on) && (g_BouquetMan->Bouquets[i]->tvChannels.size()> 0))
			{
				nBouquetCount++;
			}
	}

	if (send(connfd, &nBouquetCount, sizeof(nBouquetCount),0) == -1)
	{
		perror("[zapit] could not send any return\n");
		return;
	}
	else
	{
		for (uint i=0; i<g_BouquetMan->Bouquets.size(); i++)
		{
			// send bouquet only if there are channels in it
			if ( (Radiomode_on) && (g_BouquetMan->Bouquets[i]->radioChannels.size()> 0) ||
				!(Radiomode_on) && (g_BouquetMan->Bouquets[i]->tvChannels.size()> 0))
			{
				bouquet_msg msgBouquet;
				// we'll send name and i+1 as bouquet number
				strncpy(msgBouquet.name, g_BouquetMan->Bouquets[i]->Name.c_str(),30);
				msgBouquet.bouquet_nr = i+1;

				if (send(connfd, &msgBouquet, sizeof(msgBouquet),0) == -1)
				{
					perror("[zapit] could not send any return\n");
					return;
				}
			}
		}
	}

}

void sendChannelListOfBouquet( uint nBouquet)
{
	char* status;

	// we get the bouquet number as 1-beginning but need it 0-beginning
	nBouquet--;
	if (nBouquet < 0 || nBouquet>g_BouquetMan->Bouquets.size())
	{
		printf("[zapit] invalid bouquet number: %d",nBouquet);
		status = "-0r";
	}
	else
	{
		status = "00r";
	}

	if (send(connfd, status, strlen(status),0) == -1)
	{
		perror("[zapit] could not send any return\n");
		return;
	}

	std::vector<channel*> channels;
	if (Radiomode_on)
		channels = g_BouquetMan->Bouquets[nBouquet]->radioChannels;
	else
		channels = g_BouquetMan->Bouquets[nBouquet]->tvChannels;

	if (!channels.empty())
	{
		for (uint i = 0; i < channels.size();i++)
		{
			channel_msg_2 chanmsg;
			strncpy(chanmsg.name, channels[i]->name.c_str(),30);
			chanmsg.onid_tsid = (channels[i]->onid<<16)|channels[i]->sid;
			chanmsg.chan_nr = channels[i]->chan_nr;

			if (send(connfd, &chanmsg, sizeof(chanmsg),0) == -1)
			{
				perror("[zapit] could not send any return\n");
				return;
			}
		}
	}
	else
	{
		printf("[zapit] channel list of bouquet %d is empty\n", nBouquet + 1);
		status = "-0r";
		if (send(connfd, status, strlen(status),0) == -1)
		{
			perror("[zapit] could not send any return\n");
			return;
		}
	}
}

int network_setup() {
  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(1505);

  if ( bind(listenfd, (SA *) &servaddr, sizeof(servaddr)) !=0)
    {
      perror("[zapit] bind failed...\n");
      exit(-1);
    }


  if (listen(listenfd, 5) !=0)
    {
      perror("[zapit] listen failed...\n");
      exit( -1 );
    }

  return 0;
}

int main (int argc, char **argv)
{
	channel_msg testmsg;
	int channelcount = 0;
	int i;

	if (argc > 1)
	{
		for (i = 1; i < argc ; i++)
		{
			if (!strcmp(argv[i], "-d"))
			{
				debug=1;
			}
			else if (!strcmp(argv[i], "-o"))
			{
				offset = atoi(argv[++i]);
			}
			else if (!strcmp(argv[i], "-v"))
			{
				printf("[zapit] using vtxtd\n");
				use_vtxtd = true;
			}
			else
			{
				printf("Usage: zapit [-d] [-o offset in Hz] [-v]\n");
				exit(0);
			}
		}
	}

	system("cp " CONFIGDIR "/zapit/last_chan /tmp/zapit_last_chan");
	printf("Zapit $Id: zapit.cpp,v 1.80 2002/02/20 18:48:33 obi Exp $\n\n");
	scan_runs = 0;
	found_transponders = 0;
	found_channels = 0;
	curr_sat = -1;

	g_BouquetMan = new CBouquetManager();

	testmsg = load_settings();

	if (testmsg.mode== 'r')
	{
		Radiomode_on = true;
	}

#ifndef USE_EXTERNAL_CAMD
	caver = get_caver();
	caid = get_caid();
#endif /* USE_EXTERNAL_CAMD */

	memset(&pids_desc, 0, sizeof(pids));

	if (prepare_channels() < 0)
	{
		printf("[zapit] error parsing services!\n");
		//exit(-1);
	}

	printf("[zapit] channels have been loaded succesfully\n");

#if 0
	printf("[zapit] we have got ");
	if (!allnumchannels_tv.empty())
	{
		channelcount = allnumchannels_tv.rbegin()->first;
	}
	printf("%d tv- and ", channelcount);
	if (!allnumchannels_radio.empty())
	{
		channelcount = allnumchannels_radio.rbegin()->first;
	}
	else
	{
		channelcount = 0;
	}
	printf("%d radio-channels\n", channelcount);
#endif
	if (network_setup() != 0)
	{
		printf("[zapit] error during network_setup\n");
		exit(0);
	}

	signal(SIGHUP,termination_handler);
	signal(SIGTERM,termination_handler);

	if (debug == 0)
	{
		switch (fork())
		{
		case -1: /* can't fork */
			perror ("[zapit] fork()");
			exit (3);
		case 0: /* child, process becomes a daemon */
			//close (STDIN_FILENO);
			//close (STDOUT_FILENO);
			//close (STDERR_FILENO);

			// request a new session (job control)
			if (setsid () == -1)
			{
				exit (4);
			}
			break;
		default: /* parent returns to calling process */
			return 0;
		}
	}

#ifndef USE_EXTERNAL_CAMD
	pids _pids;
	_pids.count_vpids= 1;
	_pids.vpid= 0xffff;
	_pids.count_apids= 1;
	_pids.apids[0].pid= 0xffff;
	descramble(0xffff,0xffff,0xffff,0xffff,0xffff, &_pids);
#endif /* USE_EXTERNAL_CAMD */

	while (keep_going)
	{
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (SA *) &cliaddr, &clilen);
		memset(&rmsg, 0, sizeof(rmsg));
		read(connfd,&rmsg,sizeof(rmsg));
		parse_command();
		close(connfd);
	}
	
	return 0;
}

/**************************************************************/
/*                                                            */
/*  functions for new command handling via CZapitClient       */
/*                                                            */
/*  these functions should be encapsulated in a class CZapit  */
/*                                                            */
/**************************************************************/

void addChannelToBouquet(unsigned int bouquet, unsigned int onid_sid)
{
	printf("addChannelToBouquet(%d, %d)\n", bouquet, onid_sid);
	channel* chan = g_BouquetMan->copyChannelByOnidSid( onid_sid);
	if (chan != NULL)
		g_BouquetMan->Bouquets[bouquet-1]->addService( chan);
	else
		printf("onid_sid not found in channellist!\n");
}

void removeChannelFromBouquet(unsigned int bouquet, unsigned int onid_sid)
{
	printf("removing %d in bouquet %d \n", onid_sid, bouquet);
	g_BouquetMan->Bouquets[bouquet-1]->removeService( onid_sid);
	printf("removing %d in bouquet %d done\n", onid_sid, bouquet);
}

void sendBouquets(bool emptyBouquetsToo)
{
	for (uint i=0; i<g_BouquetMan->Bouquets.size(); i++)
	{
		if ( emptyBouquetsToo ||
			 (Radiomode_on) && (g_BouquetMan->Bouquets[i]->radioChannels.size()> 0) ||
			!(Radiomode_on) && (g_BouquetMan->Bouquets[i]->tvChannels.size()> 0))
		{
			CZapitClient::responseGetBouquets msgBouquet;
			// we'll send name and i+1 as bouquet number
			strncpy(msgBouquet.name, g_BouquetMan->Bouquets[i]->Name.c_str(),30);
			msgBouquet.bouquet_nr = i+1;

			if (send(connfd, &msgBouquet, sizeof(msgBouquet),0) == -1)
			{
				perror("[zapit] could not send any return\n");
				return;
			}
		}
	}
}

void internalSendChannels( ChannelList* channels)
{
	for (uint i = 0; i < channels->size();i++)
	{
		CZapitClient::responseGetBouquetChannels response;
		strncpy(response.name, (*channels)[i]->name.c_str(),30);
		response.onid_sid = (*channels)[i]->OnidSid();
		response.nr = (*channels)[i]->chan_nr;

		if (send(connfd, &response, sizeof(response),0) == -1)
		{
			perror("[zapit] could not send any return\n");
			return;
		}
	}
}

void sendBouquetChannels(unsigned int bouquet, CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT)
{

	bouquet--;
	if (bouquet < 0 || bouquet>g_BouquetMan->Bouquets.size())
	{
		printf("[zapit] invalid bouquet number: %d",bouquet);
		return;
	}

	ChannelList channels;

	if ((Radiomode_on && mode == CZapitClient::MODE_CURRENT ) || (mode==CZapitClient::MODE_RADIO))
		channels = g_BouquetMan->Bouquets[bouquet]->radioChannels;
	else //if ((tvmode_on && mode == CZapitClient::MODE_CURRENT ) || (mode==CZapitClient::MODE_TV))
		channels = g_BouquetMan->Bouquets[bouquet]->tvChannels;

	internalSendChannels( &channels);
}

void sendChannels( CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, CZapitClient::channelsOrder order = CZapitClient::SORT_BOUQUET)
{
	ChannelList channels;
	if (order == CZapitClient::SORT_BOUQUET)
	{
		if ((Radiomode_on && mode == CZapitClient::MODE_CURRENT ) || (mode==CZapitClient::MODE_RADIO))
		{
			for ( CBouquetManager::radioChannelIterator radiocit = g_BouquetMan->radioChannelsBegin(); radiocit != g_BouquetMan->radioChannelsEnd(); radiocit++)
			{
				channels.insert( channels.end(), (*radiocit));
			}
		}
		else
		{
			for ( CBouquetManager::tvChannelIterator tvcit = g_BouquetMan->tvChannelsBegin(); tvcit != g_BouquetMan->tvChannelsEnd(); tvcit++)
			{
				channels.insert( channels.end(), (*tvcit));
			}
		}
	}
	else if (order == CZapitClient::SORT_ALPHA)
	{
		if ((Radiomode_on && mode == CZapitClient::MODE_CURRENT ) || (mode==CZapitClient::MODE_RADIO))
		{
			for ( map<uint, channel>::iterator it=allchans_radio.begin(); it!=allchans_radio.end(); it++)
			{
				channels.insert( channels.end(), &(it->second));
			}
		}
		else
		{
			for ( map<uint, channel>::iterator it=allchans_tv.begin(); it!=allchans_tv.end(); it++)
			{
				channels.insert( channels.end(), &(it->second));
			}
		}
		sort(channels.begin(), channels.end(), CmpChannelByChName());
	}

	internalSendChannels( &channels);
}

void startPlayBack()
{
	zapit( curr_onid_sid, current_is_nvod);
}

void stopPlayBack()
{
	endzap();
}

void zapTo(unsigned int bouquet, unsigned int channel)
{
	if ((bouquet < 1) || (bouquet > g_BouquetMan->Bouquets.size()))
	{
		printf( "[zapit] Invalid bouquet %d\n", bouquet);
		return;
	}

	ChannelList channels;
	if (Radiomode_on)
		channels = g_BouquetMan->Bouquets[bouquet-1]->radioChannels;
	else
		channels = g_BouquetMan->Bouquets[bouquet-1]->tvChannels;

	if ((channel < 1) || (channel > channels.size()))
	{
		printf( "[zapit] Invalid channel %d in bouquet %d\n", channel, bouquet);
		return;
	}

	g_BouquetMan->saveAsLast( bouquet-1, channel-1);
	zapit( channels[channel-1]->OnidSid() , false);
}

void zapTo(unsigned int channel)
{
	if (Radiomode_on)
	{
		CBouquetManager::radioChannelIterator radiocit = g_BouquetMan->radioChannelsBegin();
		while ((radiocit != g_BouquetMan->radioChannelsEnd()) && (channel>1))
		{
			radiocit++;
			channel--;
		}
	//	g_BouquetMan->saveAsLast( bouquet-1, channel-1);
		if (radiocit != g_BouquetMan->radioChannelsEnd())
			zapit( (*radiocit)->OnidSid() , false);
	}
	else
	{
		CBouquetManager::tvChannelIterator tvcit = g_BouquetMan->tvChannelsBegin();
		while ((tvcit != g_BouquetMan->tvChannelsEnd()) && (channel>1))
		{
			tvcit++;
			channel--;
		}
	//	g_BouquetMan->saveAsLast( bouquet-1, channel-1);
		if (tvcit != g_BouquetMan->tvChannelsEnd())
			zapit( (*tvcit)->OnidSid() , false);
	}
}

