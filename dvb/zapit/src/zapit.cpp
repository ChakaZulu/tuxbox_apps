/*

  Zapit  -   DBoxII-Project

  $Id: zapit.cpp,v 1.121 2002/04/05 16:29:46 obi Exp $

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


$Log: zapit.cpp,v $
Revision 1.121  2002/04/05 16:29:46  obi
small fixes

Revision 1.120  2002/04/05 15:12:13  rasc
-- existsChannelInBouquet  (True/False)

Revision 1.119  2002/04/05 01:14:18  rasc
-- Favorites Bouquet handling (Easy Add Channels)

Revision 1.118  2002/04/04 23:40:55  obi
support for new camd

Revision 1.117  2002/04/04 21:26:08  obi
some more code sorting

Revision 1.116  2002/04/04 19:36:49  obi
some code sorting

Revision 1.115  2002/04/04 14:41:08  rasc
- New functions in zapitclient for handling favorites
  - test if a bouquet exists
- Some Log - CVS Entries in modules


*/

#include "zapit.h"
#include "lcddclient.h"

#define debug(fmt, args...) { if (debug) printf(fmt, ## args); }
#define dputs(str) { if (debug) puts(str); }

#define VBI_DEV	"/dev/dbox/vbi0"

struct rmsg {
	uint8_t version;
	uint8_t cmd;
	uint8_t param;
	unsigned short param2;
	char param3[30];
} rmsg;

struct {
	char mode;          // 't' TV, 'r' Radio
	unsigned int tv;
	unsigned int radio;
} lastChannel;

int connfd;

/* lnb offsets */
uint32_t lnb_offset_low = 9750000;
uint32_t lnb_offset_high = 10600000;

/* ca stuff */
uint16_t caid = 0;
int caver = 0;

#ifndef DVBS
CLcddClient lcdd;
#endif /* DVBS */

CEventServer *eventServer;

static int debug = 0;

extern uint16_t old_tsid;
uint32_t curr_onid_sid = 0;
bool OldAC3 = false;

/* near video on demand */
extern map<uint, channel> nvodchannels;
bool current_is_nvod;
std::string nvodname;

/* file descriptors */
int audio_fd = -1;
int video_fd = -1;
int dmx_audio_fd = -1;
int dmx_pcr_fd = -1;
int dmx_video_fd = -1;

/* channellists */
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

bool Radiomode_on = false;
pids pids_desc;

/* transponder scan */
pthread_t scan_thread;
extern int found_transponders;
extern int found_channels;
extern short curr_sat;
extern short scan_runs;

/* videotext */
bool use_vtxtd = false;
dvb_pid_t vtxt_pid;

CBouquetManager* g_BouquetMan;

#ifdef USE_EXTERNAL_CAMD
static int camdpid = -1;
#else
pthread_t dec_thread;
#endif /* USE_EXTERNAL_CAMD */


void termination_handler (int signum)
{
#ifdef USE_EXTERNAL_CAMD
	if (camdpid != -1)
	{
		kill(camdpid, SIGTERM);
		waitpid(camdpid, 0, 0);
	}
#endif
	if (connfd != -1)
		close(connfd);

	system("cp /tmp/zapit_last_chan " CONFIGDIR "/zapit/last_chan");
	exit(0);
}

#ifndef DVBS
int set_vtxt (dvb_pid_t teletext_pid)
{
	int fd;
	int vtxtsock;
	FILE *vtxtfd;
	struct sockaddr_un vtxtsrv;
	char vtxtbuf[255];
	char hexpid[20];

	vtxt_pid = teletext_pid;

	if (use_vtxtd)
	{
		memset(&hexpid, 0, sizeof(hexpid));
		sprintf(hexpid, "%x", teletext_pid);
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
			if (teletext_pid == 0)
			{
				fprintf(vtxtfd,"stop\n");
				debug("[zapit] vtxtd returned: %s\n", fgets(vtxtbuf, 255, vtxtfd));
			}
			else
			{
				fprintf(vtxtfd,"stop\n");
				debug("[zapit] vtxtd returned: %s\n", fgets(vtxtbuf, 255, vtxtfd));
				fprintf(vtxtfd,"pid %s\n", hexpid);
				debug("[zapit] vtxtd returned: %s\n", fgets(vtxtbuf, 255, vtxtfd));
			}
			fclose(vtxtfd);
		}
	}
	else
	{
		if ((fd = open(VBI_DEV, O_RDWR)) < 0)
		{
			perror ("[zapit] " VBI_DEV);
			return -1;
		}

		if (teletext_pid == 0)
		{
			if (ioctl(fd, AVIA_VBI_STOP_VTXT, teletext_pid) < 0)
			{
				close(fd);
				perror("[zapit] VBI_STOP_VTXT");
				return -1;
			}
		}
		else
		{
			if (ioctl(fd, AVIA_VBI_START_VTXT, teletext_pid) < 0)
			{
				close(fd);
				perror("[zapit] VBI_START_VTXT");
				return -1;
			}
		}

		close(fd);
	}

	return 0;
}
#endif /* DVBS */

int save_settings()
{
	FILE *channel_settings;
	channel_settings = fopen("/tmp/zapit_last_chan", "w");

	if (channel_settings == NULL)
	{
		perror("[zapit] fopen: /tmp/zapit_last_chan");
		return -1;
	}

	if (Radiomode_on)
	{
		CBouquetManager::radioChannelIterator cit = g_BouquetMan->radioChannelsFind(curr_onid_sid);
		if (cit != g_BouquetMan->radioChannelsEnd())
		{
			lastChannel.mode = 'r';
			lastChannel.radio = (*cit)->chan_nr;
		}
	}
	else
	{
		CBouquetManager::tvChannelIterator cit = g_BouquetMan->tvChannelsFind(curr_onid_sid);
		if (cit != g_BouquetMan->tvChannelsEnd())
		{
			lastChannel.mode = 't';
			lastChannel.tv = (*cit)->chan_nr;
		}
	}
	if (fwrite(&lastChannel, sizeof(lastChannel), 1, channel_settings) != 1)
	{
		printf("[zapit] couldn't write settings correctly\n");
		fclose(channel_settings);
		return -1;
	}

	fclose(channel_settings);
	return 0;
}

channel_msg load_settings()
{
	FILE *channel_settings;
	channel_msg output_msg;

	memset(&output_msg, 0, sizeof(output_msg));

	channel_settings = fopen("/tmp/zapit_last_chan", "r");

	if (channel_settings == NULL)
	{
		perror("[zapit] fopen: /tmp/zapit_last_chan");
		output_msg.mode = 't';
		output_msg.chan_nr = 1;
		return output_msg;
	}

	if (fread(&lastChannel, sizeof(lastChannel), 1, channel_settings) != 1)
	{
		printf("[zapit] no valid settings found\n");
		output_msg.mode = 't';
		output_msg.chan_nr = 1;
		return output_msg;
	}

	output_msg.mode = lastChannel.mode;
	output_msg.chan_nr = (Radiomode_on) ? lastChannel.radio : lastChannel.tv;

	fclose(channel_settings);

	return output_msg;
}

#ifndef USE_EXTERNAL_CAMD
void *decode_thread(void *ptr)
{
	decode_vals *vals;
	vals = (decode_vals *) ptr;
	dvb_pid_t emmpid = 0x0000;

	debug("[zapit] starting decode_thread\n");

	if (vals->do_cam_reset)
	{
		cam_reset();
	}

	descramble(vals->onid, vals->tsid, 0x104, caid, vals->ecmpid, vals->parse_pmt_pids);

	if (vals->do_search_emmpids)
	{
		if(parse_cat(caid, &emmpid) == 0)
		{
			debug("[zapit] emmpid >0x%04x< found for caid 0x%04x\n", emmpid, caid);
			setemm(0x104, caid, emmpid);
		}
		else
		{
			debug("[zapit] no emmpid found...\n");
		}
	}

	free(vals);
	debug("[zapit] ending decode_thread\n");
	dec_thread= 0;
	pthread_exit(0);
}
#endif /* USE_EXTERNAL_CAMD */

int zapit (uint32_t onid_sid, bool in_nvod)
{
	dmxPesFilterParams pes_filter;
	videoStatus video_status;
	pids parse_pmt_pids;
	std::map<uint, channel>::iterator cit;
	bool do_search_emmpid;
#ifndef USE_EXTERNAL_CAMD
	bool do_cam_reset = false;
#else
	char *vpidbuf;
	char *apidbuf;
	char *pmtpidbuf;
	char *cadescrbuf;
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
			debug("[zapit] onid_sid %08x not found\n", onid_sid);
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
				debug("[zapit] onid_sid %08x not found\n", onid_sid);
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
				debug("[zapit] onid_sid %08x not found\n", onid_sid);
				return -3;
			}
		}
	}

	/* close video device */
	if (video_fd != -1)
	{
		debug("[zapit] close video device\n");
		if (ioctl(video_fd, VIDEO_STOP, 1) < 0)
			perror("[zapit] VIDEO_STOP");

		close(video_fd);
		video_fd = -1;
	}
	else
	{
		debug("[zapit] video device already closed\n");
	}

	/* close demux devices */
	if (dmx_video_fd != -1)
	{
		debug("[zapit] close demux device (video)\n");
		close(dmx_video_fd);
		dmx_video_fd = -1;
	}
	else
	{
		debug("[zapit] demux device already closed (video)\n");
	}
	if (dmx_pcr_fd != -1)
	{
		debug("[zapit] close demux device (pcr)\n");
		close(dmx_pcr_fd);
		dmx_pcr_fd = -1;
	}
	else
	{
		debug("[zapit] demux device already closed (pcr)\n");
	}
	if (dmx_audio_fd != -1)
	{
		debug("[zapit] close demux device (audio)\n");
		close(dmx_audio_fd);
		dmx_audio_fd = -1;
	}
	else
	{
		debug("[zapit] demux device already closed (audio)\n");
	}

	/* Tune to new transponder if necessary */
	if (cit->second.tsid != old_tsid)
	{
#ifndef USE_EXTERNAL_CAMD
		do_cam_reset = true;
#endif /* USE_EXTERNAL_CAMD */
		debug("[zapit] tunig to tsid %04x\n", cit->second.tsid);

		if (tune(cit->second.tsid) < 0)
		{
			debug("[zapit] no transponder with tsid %04x found\nHave to look it up in nit\n", cit->second.tsid);
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
		curr_onid_sid = onid_sid;
		save_settings();
		return 3;
	}

#ifndef USE_EXTERNAL_CAMD
	if (caid == 0)
	{
		caid = get_caid();
	}
#else
	caid = 0x1702;
	if (camdpid != -1)
	{
		kill(camdpid, SIGTERM);
		waitpid(camdpid, 0, 0);
		camdpid = -1;
	}
#endif /* USE_EXTERNAL_CAMD */

	if ((cit->second.pmt == 0) && (cit->second.service_type != 4))
	{
		debug("[zapit] trying to find pmt for sid %04x, onid %04x\n", cit->second.sid, cit->second.onid);

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

	memset(&parse_pmt_pids, 0, sizeof(parse_pmt_pids));
	parse_pmt_pids = parse_pmt(cit->second.pmt, caid, cit->second.sid);

	if (parse_pmt_pids.count_vpids > 0)
	{
		cit->second.vpid = parse_pmt_pids.vpid;
	}
	else
	{
		cit->second.vpid = 0x1fff;
	}

	if (parse_pmt_pids.pcrpid > 0)
	{
		cit->second.pcrpid = parse_pmt_pids.pcrpid;
	}
	else
	{
		cit->second.pcrpid = 0x1fff;
	}

	if (parse_pmt_pids.count_apids > 0)
	{
		cit->second.apid = parse_pmt_pids.apids[0].pid;
	}
	else
	{
		cit->second.apid = 0x1fff;
	}

	cit->second.ecmpid = parse_pmt_pids.ecmpid;

#ifndef DVBS
	if (in_nvod)
		lcdd.setServiceName(nvodname);
	else
		lcdd.setServiceName(cit->second.name);
#endif /* DVBS */

	if ((cit->second.vpid != 0x1fff) || (cit->second.apid != 0x1fff))
	{
		pids_desc = parse_pmt_pids;

		debug("[zapit] zapping to sid: %04x %s. VPID: 0x%04x. APID: 0x%04x, PCRPID: 0x%04x, PMT: 0x%04x\n", cit->second.sid, cit->second.name.c_str(), cit->second.vpid, cit->second.apid, cit->second.pcrpid, cit->second.pmt);

#ifdef USE_EXTERNAL_CAMD
		switch ((camdpid = fork()))
		{
		case -1:
			perror("[zapit] unable to fork() for camd");
			break;
		case 0:
			vpidbuf = (char*) malloc(5);
			sprintf(vpidbuf, "%x", cit->second.vpid);
			apidbuf = (char*) malloc(5);
			sprintf(apidbuf, "%x", cit->second.apid);
			pmtpidbuf = (char*) malloc(5);
			sprintf(pmtpidbuf, "%x", cit->second.pmt);
			if ((cit->second.ecmpid > 0x0000) && (cit->second.ecmpid < 0x1FFF))
			{
				cadescrbuf = (char*) malloc(13);
				sprintf(cadescrbuf, "0904%04x%04x", caid, cit->second.ecmpid);
			}
			else
			{
				cadescrbuf = NULL;
			}

			if (execlp("/bin/camd", "camd", vpidbuf, apidbuf, pmtpidbuf, cadescrbuf, NULL) < 0)
			{
				perror("[zapit] execlp");
				exit(0);
			}
			break;
		default:
			debug("[zapit] camd pid: %d\n", camdpid);
			break;
		}
#else
		if ((cit->second.ecmpid > 0x0000) && (cit->second.ecmpid < 0x1FFF))
		{
			decode_vals *vals = (decode_vals*) malloc(sizeof(decode_vals));
			vals->onid = cit->second.onid;
			vals->tsid = cit->second.tsid;
			vals->ecmpid = cit->second.ecmpid;
			vals->parse_pmt_pids = &parse_pmt_pids;
			vals->do_search_emmpids = do_search_emmpid;
			vals->do_cam_reset = do_cam_reset;

			if (dec_thread!= 0)
			{
				debug("[zapit] waiting for decode_thread\n");
				pthread_join(dec_thread,0);
			}

			pthread_create(&dec_thread, 0,decode_thread, (void*) vals);
		}
#endif /* USE_EXTERNAL_CAMD */

		/* Open video device */
		if (video_fd == -1)
		{
			if ((video_fd = open(VIDEO_DEV, O_RDWR)) < 0)
			{
				perror("[zapit] unable to open video device");
				return -3;
			}
			else
			{
				debug("[zapit] opened video device\n");
			}
		}
		else
		{
			debug("[zapit] video device already open\n");
		}

		/* get video status */
		if (ioctl(video_fd, VIDEO_GET_STATUS, &video_status) < 0)
		{
			perror("[zapit] VIDEO_GET_STATUS");
		}

		/* select video source */
		if (video_status.streamSource != VIDEO_SOURCE_DEMUX)
		{
			if (ioctl(video_fd, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_DEMUX) < 0)
			{
				perror("[zapit] VIDEO_SELECT_SOURCE");
			}
			else
			{
				debug("[zapit] selected video source\n");
			}
		}
		else
		{
			debug("[zepit] video source already demux\n");
		}

		/* start video play */
		if (ioctl(video_fd, VIDEO_PLAY) < 0)
		{
			perror("[zapit] VIDEO_PLAY");
		}
		else
		{
			debug("[zapit] video playing\n");
		}

		/* Set audio ac3 mode */
		if (parse_pmt_pids.apids[0].is_ac3 != OldAC3)
		{
			OldAC3 = parse_pmt_pids.apids[0].is_ac3;

			if (audio_fd == -1)
			{
				if ((audio_fd = open(AUDIO_DEV, O_RDWR)) < 0)
				{
					perror("[zapit] unable to open audio device");
					return -3;
				}
				else
				{
					debug("[zapit] opened audio device\n");
				}
			}
			else
			{
				debug("[zapit] audio device already open\n");
			}

			if(ioctl(audio_fd, AUDIO_SET_BYPASS_MODE, OldAC3 ? 0 : 1) < 0)
			{
				perror("[zapit] AUDIO_SET_BYPASS_MODE");
			}
			else
			{
				debug("[zapit] audio bypass mode set\n");
			}
		}
	}
	else
	{
		debug("[zapit] not a channel. Won´t zap\n");
		return -3;
	}


	if (cit->second.vpid != 0x1fff)
	{
		/* Open demux device (video) */
		if (dmx_video_fd == -1)
		{
			if ((dmx_video_fd = open(DEMUX_DEV, O_RDWR)) < 0)
			{
				perror("[zapit] unable to open demux device (video)");
				return -3;
			}
			else
			{
				debug("[zapit] opened demux device (video)\n");
			}
		}
		else
		{
			debug("[zapit] demux device (video) already open\n");
		}

		/* setup vpid */
		pes_filter.pid = cit->second.vpid;
		pes_filter.input = DMX_IN_FRONTEND;
		pes_filter.output = DMX_OUT_DECODER;
		pes_filter.pesType = DMX_PES_VIDEO;
		pes_filter.flags = 0;

		if (ioctl(dmx_video_fd, DMX_SET_PES_FILTER, &pes_filter) < 0)
		{
			perror("[zapit] DMX_SET_PES_FILTER (video)");
			return -3;
		}
		else
		{
			debug("[zapit] video pid set to %04x\n", cit->second.vpid);
		}
	}

	if (cit->second.pcrpid != 0x1fff)
	{
		/* Open demux device (pcr) */
		if (dmx_pcr_fd == -1)
		{
			if ((dmx_pcr_fd = open(DEMUX_DEV, O_RDWR)) < 0)
			{
				perror("[zapit] unable to open demux device (pcr)");
				return -3;
			}
			else
			{
				debug("[zapit] opened demux device (pcr)\n");
			}
		}
		else
		{
			debug("[zapit] demux device (pcr) already open\n");
		}

		/* setup pcrpid and start demuxing */
		pes_filter.pid = cit->second.pcrpid;
		pes_filter.input = DMX_IN_FRONTEND;
		pes_filter.output = DMX_OUT_DECODER;
		pes_filter.pesType = DMX_PES_PCR;
		pes_filter.flags = DMX_IMMEDIATE_START;

		if (ioctl(dmx_pcr_fd, DMX_SET_PES_FILTER, &pes_filter) < 0)
		{
			perror("[zapit] DMX_SET_PES_FILTER (pcr)");
			return -3;
		}
		else
		{
			debug("[zapit] pcr pid set to %04x and demux started\n", cit->second.pcrpid);
		}
	}

	if (cit->second.apid != 0x1FFF)
	{
		/* Open demux device (audio) */
		if (dmx_audio_fd == -1)
		{
			if ((dmx_audio_fd = open(DEMUX_DEV, O_RDWR)) < 0)
			{
				perror("[zapit] unable to open demux device (audio)");
				return -3;
			}
			else
			{
				debug("[zapit] opened demux device (audio)\n");
			}
		}
		else
		{
			debug("[zapit] demux device (audio) already open\n");
		}

		/* setup apid and start demuxing */
		pes_filter.pid     = cit->second.apid;
		pes_filter.input   = DMX_IN_FRONTEND;
		pes_filter.output  = DMX_OUT_DECODER;
		pes_filter.pesType = DMX_PES_AUDIO;
		pes_filter.flags   = DMX_IMMEDIATE_START;

		if (ioctl(dmx_audio_fd, DMX_SET_PES_FILTER, &pes_filter) < 0)
		{
			perror("[zapit] DMX_SET_PES_FILTER (audio)");
			return -3;
		}
		else
		{
			debug("[zapit] audio pid set to %04x and demux started\n", cit->second.apid);
		}
	}

	if (cit->second.vpid != 0x1FFF)
	{
		/* start demux (video) */
		if (ioctl(dmx_video_fd, DMX_START, 0) < 0)
		{
			perror("[zapit] DMX_START (video)");
		}
		else
		{
			debug("[zapit] started demux (video)\n");
		}
	}

#ifndef DVBS
	debug("[zapit] setting vtxt\n");
	set_vtxt(parse_pmt_pids.vtxtpid);
#endif /* DVBS */

	debug("[zapit] saving settings\n");
	curr_onid_sid = onid_sid;

	if (!in_nvod)
	{
		save_settings();
	}

	return 3;
}


int numzap(int channel_number)
{
	std::map<uint, uint>::iterator cit;

	nvodchannels.clear();
	if (Radiomode_on)
	{
		if (allnumchannels_radio.count(channel_number) == 0)
		{
			debug("[zapit] given channel not found\n");
			return -2;
		}
		cit  = allnumchannels_radio.find(channel_number);
	}
	else
	{
		if (allnumchannels_tv.count(channel_number) == 0)
		{
			debug("[zapit] given channel not found\n");
			return -2;
		}
		cit = allnumchannels_tv.find(channel_number);
	}

	debug("[zapit] zapping to onid_sid %04x\n", cit->second);

	if (zapit(cit->second, false) > 0)
		return 2;
	else
		return -2;
}


int namezap(std::string channel_name)
{
	std::map<std::string,uint>::iterator cit;

	nvodchannels.clear();

	// Search current channel
	if (Radiomode_on)
	{
		if (allnamechannels_radio.count(channel_name) == 0)
		{
			debug("[zapit] given channel not found\n");
			return -2;
		}
		cit  = allnamechannels_radio.find(channel_name);
	}
	else
	{
		if (allnamechannels_tv.count(channel_name) == 0)
		{
			debug("[zapit] given channel not found\n");
			return -2;
		}
		cit = allnamechannels_tv.find(channel_name);
	}

	debug("[zapit] zapping to onid_sid %04x\n", cit->second);

	if (zapit(cit->second, false) > 0)
		return 3;
	else
		return -3;
}

int changeapid(uint8_t pid_nr)
{
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
		if (dmx_audio_fd == -1)
		{
			if ((dmx_audio_fd = open(DEMUX_DEV, O_RDWR)) < 0)
			{
				perror("[zapit] unable to open demux device (audio / changeapid)");
				return -8;
			}
			else
			{
				debug("[zapit] opened demux device (audio)\n");
			}
		}

		if (ioctl(dmx_audio_fd, DMX_STOP, 0) < 0)
		{
			perror("[zapit] unable to stop audio");
			return -8;
		}

		/* Set audio ac3 mode */
		if (pids_desc.apids[pid_nr].is_ac3 != OldAC3)
		{
			OldAC3 = pids_desc.apids[pid_nr].is_ac3;

			if (audio_fd == -1)
			{
				if ((audio_fd = open(AUDIO_DEV, O_RDWR)) < 0)
				{
					perror("[zapit] unable to open audio device");
					return -8;
				}
				else
				{
					debug("[zapit] opened audio device\n");
				}
			}
			else
			{
				debug("[zapit] audio device already open\n");
			}

			if(ioctl(audio_fd, AUDIO_SET_BYPASS_MODE, OldAC3 ? 0 : 1) < 0)
			{
				perror("[zapit] AUDIO_SET_BYPASS_MODE");
				return -8;
			}
			else
			{
				debug("[zapit] audio bypass mode set\n");
			}
		}

		/* apid */
		pes_filter.pid = pids_desc.apids[pid_nr].pid;
		pes_filter.input = DMX_IN_FRONTEND;
		pes_filter.output = DMX_OUT_DECODER;
		pes_filter.pesType = DMX_PES_AUDIO;
		pes_filter.flags = DMX_IMMEDIATE_START;

		if (ioctl(dmx_audio_fd, DMX_SET_PES_FILTER, &pes_filter) < 0)
		{
			perror("[zapit] unable to set demux pes filter");
			return -8;
		}
		else
		{
			debug("[zapit] apid changed to %04x\n", pids_desc.apids[pid_nr].pid);
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
	if (audio_fd != -1)
	{
		close(audio_fd);
		audio_fd = -1;
	}
	if (video_fd != -1)
	{
		close(video_fd);
		video_fd = -1;
	}
	if (dmx_video_fd != -1)
	{
		close(dmx_video_fd);
		dmx_video_fd = -1;
	}
	if (dmx_audio_fd != -1)
	{
		close(dmx_audio_fd);
		dmx_audio_fd = -1;
	}
	if (dmx_pcr_fd != -1)
	{
		close(dmx_pcr_fd);
		dmx_pcr_fd = -1;
	}
	return;
}


int shutdownBox()
{
	if (execlp("/sbin/halt", "halt", NULL) < 0)
	{
		perror("[zapit] execlp");
		return -1;
	}
	return 0;
}

int sleepBox()
{
	int device;
	if((device = open(FRONT_DEV, O_RDWR)) < 0)
	{
		perror("[zapit] " FRONT_DEV);
		return -4;
	}

	if (ioctl(device, FE_SET_POWER_STATE, FE_POWER_OFF) != 0)
	{
		perror("[zapit] FE_SET_POWER_STATE");
		return -4;
	}

	return(4);
}

void setRadioMode()
{
	debug("[zapit] switching to radio-mode\n");
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

	//std::map<uint, uint>::iterator numit;
	//std::map<std::string, uint>::iterator nameit;
	//std::map<uint, channel>::iterator cit;

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

	return 23;
}

int start_scan(unsigned short do_diseqc)
{
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

#ifdef USE_EXTERNAL_CAMD
	if (camdpid != -1)
	{
		kill(camdpid, SIGTERM);
		waitpid(camdpid, 0, 0);
	}
#endif
	if (audio_fd != -1)
	{
		close(audio_fd);
		audio_fd = -1;
	}

	if (video_fd != -1)
	{
		/* blank screen */
		if(ioctl(video_fd, VIDEO_STOP, 1) < 0)
			perror("[zapit] VIDEO_STOP");

		close(video_fd);
		video_fd = -1;
	}

	if (dmx_audio_fd != -1)
	{
		close(dmx_audio_fd);
		dmx_audio_fd = -1;
	}

	if (dmx_video_fd != -1)
	{
		close(dmx_video_fd);
		dmx_video_fd = -1;
	}

	if (dmx_pcr_fd != -1)
	{
		close(dmx_pcr_fd);
		dmx_pcr_fd = -1;
	}

	if (pthread_create(&scan_thread, 0, start_scanthread, &do_diseqc))
	{
		perror("[zapit] pthread_create: scan_thread");
		return -1;
	}

	while (scan_runs == 0)
		debug("[zapit] waiting for scan to start\n");

	return 0;
}

void parse_command()
{
	char *status;
	short carsten; // who the fuck is short carsten? :)

	std::map<uint,uint>::iterator sit;
	std::map<uint,channel>::iterator cit;
	int number = 0;
	int caid_ver = 0;

	//byteorder!!!!!!
	//rmsg.param2 = ((rmsg.param2 & 0x00ff) << 8) | ((rmsg.param2 & 0xff00) >> 8);

#ifdef DEBUG
	debug("Command received\n");
	debug("  Version: %d\n", rmsg.version);
	debug("  Command: %d\n", rmsg.cmd);
	debug("  Param: %c\n", rmsg.param);
	debug("  Param2: %d\n", rmsg.param2);
	debug("  Param3: %s\n", rmsg.param3);
#endif

	if (rmsg.version == 1)
	{
		switch (rmsg.cmd)
		{
		case 1:
			debug("[zapit] zapping by number\n");
			if (numzap(atoi((const char*) &rmsg.param)) > 0)
			{
				status = "001";
			}
			else
			{
				status = "-01";
			}
			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 2:
			debug("[zapit] killing zap\n");
			endzap();
			status = "002";
			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 3:
			debug("[zapit] zapping by name\n");
			if (namezap(rmsg.param3) == 3)
			{
				status = "003";
			}
			else
			{
				status = "-03";
			}
			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 4:
			status = "004";
			debug("[zapit] shutdown\n");
			shutdownBox();
			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 5:
			if (Radiomode_on)
			{
				if (!allchans_radio.empty())
				{
					status = "005";
					if (send(connfd, status, strlen(status), 0) == -1)
					{
						perror("[zapit] send");
						return;
					}
					for (sit = allnumchannels_radio.begin(); sit != allnumchannels_radio.end(); sit++)
					{
						cit = allchans_radio.find(sit->second);
						channel_msg chanmsg;
						strncpy(chanmsg.name, cit->second.name.c_str(), 29);
						chanmsg.chan_nr = sit->first;
						chanmsg.mode = 'r';
						if (send(connfd, &chanmsg, sizeof(chanmsg), 0) == -1)
						{
							perror("[zapit] send");
							return;
						}
					}
				}
				else
				{
					printf("[zapit] radio_channellist is empty\n");
					status = "-05";
					if (send(connfd, status, strlen(status), 0) == -1)
					{
						perror("[zapit] send");
						return;
					}
				}
			}
			else
			{
				if (!allchans_tv.empty())
				{
					status = "005";
					if (send(connfd, status, strlen(status), 0) == -1)
					{
						perror("[zapit] send");
						return;
					}
					for (sit = allnumchannels_tv.begin(); sit != allnumchannels_tv.end(); sit++)
					{
						cit = allchans_tv.find(sit->second);
						channel_msg chanmsg;
						strncpy(chanmsg.name, cit->second.name.c_str(),29);
						chanmsg.chan_nr = sit->first;
						chanmsg.mode = 't';
						if (send(connfd, &chanmsg, sizeof(chanmsg), 0) == -1)
						{
							perror("[zapit] send");
							return;
						}
					}
				}
				else
				{
					printf("[zapit] tv_channellist is empty\n");
					status = "-05";
					if (send(connfd, status, strlen(status), 0) == -1)
					{
						perror("[zapit] send");
						return;
					}
				}
			}
			usleep(200000);
			break;
		case 6:
			status = "006";
			setRadioMode();
			if (allchans_radio.empty())
			{
				status = "-06";
			}
			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 7:
			status = "007";
			setTVMode();
			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 8:
			status = "008";
			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			if (send(connfd, &pids_desc, sizeof(pids), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 9:
			if (changeapid(atoi((const char*) &rmsg.param)) > 0)
			{
				status = "009";
			}
			else
			{
				status = "-09";
			}
			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 'a':
			status = "00a";
			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			else
			{
				channel_msg settings = load_settings();
				if (send(connfd, &settings, sizeof(settings), 0) == -1)
				{
					perror("[zapit] send");
					return;
				}
			}
			break;
		case 'b':
			if (Radiomode_on)
			{
				cit = allchans_radio.find(curr_onid_sid);
			}
			else
			{
				if (current_is_nvod)
				{
					cit = nvodchannels.find(curr_onid_sid);
				}
				else
				{
					cit = allchans_tv.find(curr_onid_sid);
				}
			}

			if (curr_onid_sid == 0)
			{
				status = "-0b";
				break;
			}
			else
			{
				status = "00b";
			}

			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}

			carsten = (short) cit->second.vpid;
			if (send(connfd, &carsten, 2, 0) == -1)
			{
				perror("[zapit] send");
				return;
			}

			carsten = (short) cit->second.apid;
			if (send(connfd, &carsten, 2, 0) == -1)
			{
				perror("[zapit] send");
				return;
			}

			break;
		case 'c':
			if (Radiomode_on)
			{
				if (!allchans_radio.empty())
				{
					status = "00c";
					if (send(connfd, status, strlen(status), 0) == -1)
					{
						perror("[zapit] send");
						return;
					}
					for (CBouquetManager::radioChannelIterator radiocit = g_BouquetMan->radioChannelsBegin(); radiocit != g_BouquetMan->radioChannelsEnd(); radiocit++)
					{
						channel_msg_2 chanmsg;

						strncpy(chanmsg.name, (*radiocit)->name.c_str(),30);
						chanmsg.chan_nr = (*radiocit)->chan_nr;
						chanmsg.onid_tsid = (*radiocit)->OnidSid();

						if (send(connfd, &chanmsg, sizeof(chanmsg), 0) == -1)
						{
							perror("[zapit] send");
							return;
						}
					}
				}
				else
				{
					printf("[zapit] tv_channellist is empty\n");
					status = "-0c";
					if (send(connfd, status, strlen(status), 0) == -1)
					{
						perror("[zapit] send");
						return;
					}
				}
			}
			else
			{
				if (!allchans_tv.empty())
				{
					status = "00c";
					if (send(connfd, status, strlen(status), 0) == -1)
					{
						perror("[zapit] send");
						return;
					}
					for (CBouquetManager::tvChannelIterator tvcit = g_BouquetMan->tvChannelsBegin(); tvcit != g_BouquetMan->tvChannelsEnd(); tvcit++)
					{
						channel_msg_2 chanmsg;
						strncpy(chanmsg.name, (*tvcit)->name.c_str(),30);
						chanmsg.chan_nr = (*tvcit)->chan_nr;
						chanmsg.onid_tsid = (*tvcit)->OnidSid();
						if (send(connfd, &chanmsg, sizeof(chanmsg), 0) == -1)
						{
							perror("[zapit] send");
							return;
						}
					}
				}
				else
				{
					printf("tv_channellist is empty\n");
					status = "-0c";
					if (send(connfd, status, strlen(status), 0) == -1)
					{
						perror("[zapit] send");
						return;
					}
				}
			}
			break;

		case 'd':
			debug("[zapit] zapping by number\n");
			number = 0;
			sscanf((const char*) &rmsg.param3, "%x", &number);
			char m_status[4];

			if (zapit(number, false) > 0)
			{
				strcpy(m_status, "00d");
				m_status[1] = pids_desc.count_apids & 0x0f;

				if (current_is_nvod)
				{
					m_status[1] |= zapped_chan_is_nvod;
				}
			}
			else
			{
				strcpy(m_status, "-0d");
			}

			if (send(connfd, m_status, 3, 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			if (send(connfd, &pids_desc, sizeof(pids), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 'e':
			debug("[zapit] changing nvod\n");
			number = 0;
			sscanf((const char*) &rmsg.param3, "%x", &number);

			if (zapit(number,true) > 0)
			{
				strcpy(m_status, "00e");
				m_status[1] = pids_desc.count_apids & 0x0f;

				if (current_is_nvod)
				{
					m_status[1] |= zapped_chan_is_nvod;
				}
			}
			else
			{
				strcpy(m_status, "-0e");
			}

			if (send(connfd, m_status, 3, 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			if (send(connfd, &pids_desc, sizeof(pids), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 'f':
			if (current_is_nvod)
			{
				status = "00f";
			}
			else
			{
				status = "-0f";
			}

			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
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

					if (send(connfd, &chanmsg, sizeof(chanmsg), 0) == -1)
					{
						perror("[zapit] send");
						return;
					}
				}
			}
			break;
		case 'g':
			start_scan(rmsg.param2);
			status = "00g";

			if (send(connfd, status, strlen(status),0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 'h':
			if (scan_runs > 0)
			{
				status = "00h";
			}
			else
			{
				status = "-0h";
			}

			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}

			if (send(connfd, &curr_sat, sizeof(short), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}

			if (send(connfd, &found_transponders, sizeof(int), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}

			if (send(connfd, &found_channels, sizeof(int), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 'i':
			uint nvod_onidsid;
			ushort nvod_tsid;
			ushort cnt_nvods;

			//quick hack /* sehr sinnvoll - obi */
			current_is_nvod = true;

			if (current_is_nvod)
			{
				status = "00i";
			}
			else
			{
				status = "-0i";
			}

			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}

			if (recv(connfd, &cnt_nvods, 2, 0) == -1)
			{
				perror("[zapit] recv");
				return;
			}
			else
			{
				int cnt;
				debug("[zapit] receiving nvods (%d)\n", cnt_nvods);
				for (cnt = 0; cnt < cnt_nvods; cnt++)
				{
					if (recv(connfd, &nvod_onidsid, 4, 0) == -1)
					{
						perror("[zapit] recv");
						return;
					}
					if (recv(connfd, &nvod_tsid, 2, 0) == -1)
					{
						perror("[zapit] recv");
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
			if (send(connfd, status, strlen(status), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;

		case 'q':
			sendBouquetList();
			break;

		case 'r':
			sendChannelListOfBouquet(rmsg.param);
			break;
		case 's':
			status = "00s";
			if (send(connfd, status, strlen(status),0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			if (send(connfd, &curr_onid_sid, sizeof(uint),0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 't':
			status = "00t";

			switch (caid)
			{
				case 0x1722 :
					caid_ver = 1;
					break;
				case 0x1702 :
					caid_ver = 2;
					break;
				case 0x1762 :
					caid_ver = 4;
					break;
				default :
					caid_ver = 8;
					break;
			}
			caid_ver |= caver;
			if (send(connfd, status, strlen(status),0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			if (send(connfd, &caid_ver, sizeof(int),0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		case 'u':
			status = "00u";
			if (send(connfd, status, strlen(status),0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			if (send(connfd, &vtxt_pid, sizeof(int),0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		default:
			status = "000";
			if (send(connfd, status, strlen(status),0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			printf("[zapit] unknown command\n");
		}
	}

/********************************************/
/*                                          */
/*  new command handling via CZapitClient   */
/*                                          */
/********************************************/

	else if (rmsg.version == CZapitClient::ACTVERSION)
	{
		CZapitClient::responseCmd              response;
		CZapitClient::responseGeneralInteger   responseInteger;		// 2002-04-03 rasc
		CZapitClient::responseGeneralTrueFalse responseBool;		// 2002-04-05 rasc

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

            case CZapitClient::CMD_ZAPTO_SERVICEID :
            case CZapitClient::CMD_ZAPTO_SUBSERVICEID :
            	CZapitClient::commandZaptoServiceID msgZaptoServiceID;
                CZapitClient::responseZapComplete msgResponseZapComplete;
				read( connfd, &msgZaptoServiceID, sizeof(msgZaptoServiceID));
               	msgResponseZapComplete.zapStatus = zapTo_ServiceID( msgZaptoServiceID.serviceID , ( rmsg.cmd == CZapitClient::CMD_ZAPTO_SUBSERVICEID ) );
				send(connfd, &msgResponseZapComplete, sizeof(msgResponseZapComplete), 0);
			break;

			case CZapitClient::CMD_ZAPTO_SERVICEID_NOWAIT :
            case CZapitClient::CMD_ZAPTO_SUBSERVICEID_NOWAIT :
            	CZapitClient::commandZaptoServiceID msgZaptoServiceID2;
				read( connfd, &msgZaptoServiceID2, sizeof(msgZaptoServiceID2));
               	zapTo_ServiceID( msgZaptoServiceID2.serviceID , ( rmsg.cmd == CZapitClient::CMD_ZAPTO_SUBSERVICEID_NOWAIT ) );
			break;

			case CZapitClient::CMD_GET_LAST_CHANNEL :
				channel_msg mysettings;
				mysettings = load_settings();
				CZapitClient::responseGetLastChannel responseGetLastChannel;
				strcpy( responseGetLastChannel.channelName, mysettings.name );
				responseGetLastChannel.channelNumber = mysettings.chan_nr;
				responseGetLastChannel.mode = mysettings.mode;
				send( connfd, &responseGetLastChannel, sizeof(responseGetLastChannel),0);
			break;

			case CZapitClient::CMD_SET_AUDIOCHAN :
				CZapitClient::commandSetAudioChannel msgSetAudioChannel;
				read( connfd, &msgSetAudioChannel, sizeof(msgSetAudioChannel));
				changeapid( msgSetAudioChannel.channel );
			break;

			case CZapitClient::CMD_SET_MODE :
				CZapitClient::commandSetMode msgSetMode;
				read( connfd, &msgSetMode, sizeof(msgSetMode));

				if ( msgSetMode.mode == CZapitClient::MODE_TV )
					setTVMode();
				else if ( msgSetMode.mode == CZapitClient::MODE_RADIO )
					setRadioMode();
			break;

			case CZapitClient::CMD_GET_CURRENT_SERVICEID :
				CZapitClient::responseGetCurrentServiceID msgCurrentSID;
				msgCurrentSID.serviceID = curr_onid_sid;
				send( connfd, &msgCurrentSID, sizeof(msgCurrentSID), 0);
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

			case CZapitClient::CMD_SCANSTART :
				CZapitClient::commandStartScan msgStartScan;
				read( connfd, &msgStartScan, sizeof(msgStartScan));
				start_scan(msgStartScan.satelliteMask);
			break;

			case CZapitClient::CMD_SCANREADY :
				CZapitClient::responseIsScanReady msgResponseIsScanReady;
				msgResponseIsScanReady.satellite   = curr_sat;
				msgResponseIsScanReady.transponder = found_transponders;
				msgResponseIsScanReady.services    = found_channels;
				if (scan_runs > 0)
				{
					msgResponseIsScanReady.scanReady = false;
				}
				else
				{
					msgResponseIsScanReady.scanReady = true;
				}
				send( connfd, &msgResponseIsScanReady, sizeof(msgResponseIsScanReady),0);
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
				g_BouquetMan->addBouquet(msgAddBouquet.name);
				g_BouquetMan->Bouquets[msgRenameBouquet.bouquet-1]->Name = msgRenameBouquet.name;
			break;

			case CZapitClient::CMD_BQ_EXISTS_BOUQUET :		// 2002-04-03 rasc
				CZapitClient::commandExistsBouquet msgExistsBouquet;
				read( connfd, &msgExistsBouquet, sizeof(msgExistsBouquet));
				// -- for some unknown reason BQ-IDs are externally  1..n
				// -- internally BQ-IDs are 0..n-1, so add 1!!
				// -- This also means "not found (-1)" get's to zero (0)
				responseInteger.number = g_BouquetMan->existsBouquet(msgExistsBouquet.name)+1;
				send( connfd, &responseInteger, sizeof(responseInteger),0);
			break;

			case CZapitClient::CMD_BQ_EXISTS_CHANNEL_IN_BOUQUET :	// 2002-04-05 rasc
				CZapitClient::commandExistsChannelInBouquet msgExistsChInBq;
				read( connfd, &msgExistsChInBq, sizeof(msgExistsChInBq));
				// -- for some unknown reason BQ-IDs are externally  1..n
				// -- internally BQ-IDs are 0..n-1, so subtract 1!!
				responseBool.status = g_BouquetMan->existsChannelInBouquet(
						msgExistsChInBq.bouquet-1, msgExistsChInBq.onid_sid);
				send( connfd, &responseBool, sizeof(responseBool),0);
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

			case CZapitClient::CMD_BQ_SET_LOCKSTATE :
				CZapitClient::commandBouquetState msgBouquetLockState;
				read( connfd, &msgBouquetLockState, sizeof(msgBouquetLockState));
				g_BouquetMan->Bouquets[msgBouquetLockState.bouquet-1]->bLocked = msgBouquetLockState.state;
			break;

			case CZapitClient::CMD_BQ_SET_HIDDENSTATE :
				CZapitClient::commandBouquetState msgBouquetHiddenState;
				read( connfd, &msgBouquetHiddenState, sizeof(msgBouquetHiddenState));
				g_BouquetMan->Bouquets[msgBouquetHiddenState.bouquet-1]->bHidden = msgBouquetHiddenState.state;
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

			case CZapitClient::CMD_GETPIDS :
				CZapitClient::responseGetOtherPIDs responseGetOtherPIDs;
				responseGetOtherPIDs.vpid = pids_desc.vpid;
				responseGetOtherPIDs.ecmpid = pids_desc.ecmpid;
				responseGetOtherPIDs.vtxtpid = pids_desc.vtxtpid;
				responseGetOtherPIDs.pcrpid = pids_desc.pcrpid;
				send( connfd, &responseGetOtherPIDs, sizeof(responseGetOtherPIDs),0);
				sendAPIDs();
			break;

			case CZapitClient::CMD_SETSUBSERVICES :
				CZapitClient::commandAddSubServices msgAddSubService;

				while ( read( connfd, &msgAddSubService, sizeof(msgAddSubService)))
				{
					//printf("got subchan %x %x\n", msgAddSubService.onidsid, msgAddSubService.tsid);
                	nvodchannels.insert(std::pair<int,channel>(msgAddSubService.onidsid,channel("NVOD",0,0,0,0,0,(msgAddSubService.onidsid&0xFFFF),msgAddSubService.tsid,(msgAddSubService.onidsid>>16),1)));
				}

				current_is_nvod = true;
			break;

			case CZapitClient::CMD_REGISTEREVENTS :
				eventServer->registerEvent( connfd );
			break;

			case CZapitClient::CMD_UNREGISTEREVENTS :
				eventServer->unRegisterEvent( connfd );
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

void sendChannelListOfBouquet(uint nBouquet)
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
		if (send(connfd, status, strlen(status), 0) < 0)
		{
			perror("[zapit] could not send any return\n");
			return;
		}
	}
}

int main (int argc, char **argv)
{
	int listenfd;
	struct sockaddr_un servaddr;
	int clilen;

	channel_msg testmsg;
	int i;
#if DEBUG
	int channelcount = 0;
#endif /* DEBUG */

	printf("$Id: zapit.cpp,v 1.121 2002/04/05 16:29:46 obi Exp $\n\n");

	if (argc > 1)
	{
		for (i = 1; i < argc ; i++)
		{
			if (!strcmp(argv[i], "-d"))
			{
				debug = 1;
			}
			else if (!strcmp(argv[i], "-lo"))
			{
				lnb_offset_low = atoi(argv[++i]) * 1000;
			}
			else if (!strcmp(argv[i], "-ho"))
			{
				lnb_offset_high = atoi(argv[++i]) * 1000;
			}
			else if (!strcmp(argv[i], "-q"))
			{
				/* don't say anything */
				int fd;

				close(STDOUT_FILENO);
				if ((fd = open("/dev/null", O_WRONLY)) != STDOUT_FILENO)
					close(fd);

				close(STDERR_FILENO);
				if ((fd = open("/dev/null", O_WRONLY)) != STDERR_FILENO)
					close(fd);
			}
			else if (!strcmp(argv[i], "-v"))
			{
				printf("[zapit] using vtxtd\n");
				use_vtxtd = true;
			}
			else
			{
				printf("Usage: zapit [-d] [-ho <high offset in kHz>] [-lo <low offset in kHz>] [-q] [-v]\n");
				exit(0);
			}
		}
	}

	system("cp " CONFIGDIR "/zapit/last_chan /tmp/zapit_last_chan");

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
	}

	printf("[zapit] channels have been loaded succesfully\n");

#ifdef DEBUG
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
#endif /* DEBUG */

	/* network setup */
	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, ZAPIT_UDS_NAME);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	unlink(ZAPIT_UDS_NAME);

	if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("[zapit] socket");
		return -1;
	}
	if (bind(listenfd, (struct sockaddr*) &servaddr, clilen) < 0)
	{
		perror("[zapit] bind");
		return -1;
	}
	if (listen(listenfd, 5) != 0)
	{
		perror("[zapit] listen");
		return -1;
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

	// create eventServer
	eventServer = new CEventServer;

	while (true)
	{
		connfd = accept(listenfd, (struct sockaddr*) &servaddr, (socklen_t*) &clilen);
		memset(&rmsg, 0, sizeof(rmsg));
		read(connfd, &rmsg, sizeof(rmsg));
		parse_command();
		close(connfd);
		connfd = -1;
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
			 (Radiomode_on) && (g_BouquetMan->Bouquets[i]->radioChannels.size()> 0) && (!g_BouquetMan->Bouquets[i]->bHidden) ||
			!(Radiomode_on) && (g_BouquetMan->Bouquets[i]->tvChannels.size()> 0) && (!g_BouquetMan->Bouquets[i]->bHidden))
		{
			CZapitClient::responseGetBouquets msgBouquet;
			// we'll send name and i+1 as bouquet number
			strncpy(msgBouquet.name, g_BouquetMan->Bouquets[i]->Name.c_str(),30);
			msgBouquet.bouquet_nr = i+1;
			msgBouquet.locked = g_BouquetMan->Bouquets[i]->bLocked;
			msgBouquet.hidden = g_BouquetMan->Bouquets[i]->bHidden;
			if (send(connfd, &msgBouquet, sizeof(msgBouquet),0) == -1)
			{
				perror("[zapit] could not send any return\n");
				return;
			}
		}
	}
}

void internalSendChannels(ChannelList* channels)
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

void sendAPIDs()
{
	for (uint i = 0; i < pids_desc.count_apids; i++)
	{
		CZapitClient::responseGetAPIDs response;
		response.pid= pids_desc.apids[i].pid;
		strncpy(response.desc, pids_desc.apids[i].desc, 25);
		response.is_ac3= pids_desc.apids[i].is_ac3;
		response.component_tag= pids_desc.apids[i].component_tag;

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

unsigned zapTo(unsigned int bouquet, unsigned int channel)
{
	unsigned result = 0;

	if ((bouquet < 1) || (bouquet > g_BouquetMan->Bouquets.size()))
	{
		printf( "[zapit] Invalid bouquet %d\n", bouquet);
		result|= CZapitClient::ZAP_INVALID_PARAM;
		return ( result );
	}

	ChannelList channels;
	if (Radiomode_on)
		channels = g_BouquetMan->Bouquets[bouquet-1]->radioChannels;
	else
		channels = g_BouquetMan->Bouquets[bouquet-1]->tvChannels;

	if ((channel < 1) || (channel > channels.size()))
	{
		printf( "[zapit] Invalid channel %d in bouquet %d\n", channel, bouquet);
		result|= CZapitClient::ZAP_INVALID_PARAM;
		return ( result );
	}

	g_BouquetMan->saveAsLast( bouquet-1, channel-1);


	result = zapTo_ServiceID ( channels[channel-1]->OnidSid(), false );

    return ( result );
}

unsigned int zapTo_ServiceID (unsigned int serviceID, bool isSubService )
{
	unsigned result = 0;

	if ( zapit( serviceID , isSubService ) > 0 )
	{
		result|= CZapitClient::ZAP_OK;

		if (current_is_nvod)
		 	result|= CZapitClient::ZAP_IS_NVOD;
	}

	if ( !( result & CZapitClient::ZAP_OK ) )
		eventServer->sendEvent(CZapitClient::EVT_ZAP_FAILED, CEventServer::INITID_ZAPIT, &serviceID, sizeof(serviceID) );
	else
	{
		if ( isSubService )
		{
			eventServer->sendEvent(CZapitClient::EVT_ZAP_SUB_COMPLETE, CEventServer::INITID_ZAPIT, &serviceID, sizeof(serviceID) );
		}
		else
		{
			if ( result & CZapitClient::ZAP_IS_NVOD )
				eventServer->sendEvent(CZapitClient::EVT_ZAP_COMPLETE_IS_NVOD, CEventServer::INITID_ZAPIT, &serviceID, sizeof(serviceID) );
			else
				eventServer->sendEvent(CZapitClient::EVT_ZAP_COMPLETE, CEventServer::INITID_ZAPIT, &serviceID, sizeof(serviceID) );
		}
	}

    return result;
}


unsigned zapTo (unsigned int channel)
{
	unsigned result = 0;

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
			result = zapTo_ServiceID ( (*radiocit)->OnidSid(), false );
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
			result = zapTo_ServiceID ( (*tvcit)->OnidSid(), false );
	}

	return result;
}

