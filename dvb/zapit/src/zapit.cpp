/*
 * $Id: zapit.cpp,v 1.170 2002/05/04 14:39:57 McClean Exp $
 *
 * zapit - d-box2 linux project
 *
 * (C) 2001, 2002 by Philipp Leusmann <faralla@berlios.de>
 *
 * based on code from older applications of the d-box2 linux project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

// TODO: write a CZapit class

#include <dbox/avia_gt_vbi.h>
#include <fcntl.h>
#include <ost/audio.h>
#include <ost/video.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "configfile.h"
#include "cam.h"
#include "cat.h"
#include "frontend.h"
#include "getservices.h"
#include "lcddclient.h"
#include "pat.h"
#include "pmt.h"
#include "zapit.h"

#define debug(fmt, args...) { if (debug) printf(fmt, ## args); }

#define AUDIO_DEV "/dev/ost/audio0"
#define DEMUX_DEV "/dev/ost/demux0"
#define VBI_DEV   "/dev/dbox/vbi0"
#define VIDEO_DEV "/dev/ost/video0"

#define CONFIGFILE CONFIGDIR "/zapit/zapit.conf"

/* the conditional access module */
CCam *cam = NULL;
/* the configuration file */
CConfigFile *config = NULL;
/* the event server */
CEventServer *eventServer = NULL;
/* the dvb frontend */
CFrontend *frontend = NULL;
/* the current channel */
CZapitChannel *channel = NULL;
/* the transponder scan xml input */
XMLTreeParser *scanInputParser = NULL;
/* the bouquet manager */
CBouquetManager* g_BouquetMan = NULL;

/* the map which stores the wanted cable/satellites */
std::map <uint8_t, std::string> scanProviders;

/* current zapit mode */
enum
{
	TV_MODE = 0x01,
	RADIO_MODE = 0x02,
	RECORD_MODE = 0x04
};

int currentMode;

struct rmsg
{
	uint8_t version;
	uint8_t cmd;
	uint8_t param;
	unsigned short param2;
	char param3[30];

} rmsg;

int connfd;

#ifndef DVBS
CLcddClient lcdd;
#endif /* DVBS */

bool debug = false;

/* previous ac3 state */
bool wasAc3 = false;

/* near video on demand */
extern map <uint, CZapitChannel> nvodchannels;
bool current_is_nvod;
std::string nvodname;

/* file descriptors */
int audio_fd = -1;
int video_fd = -1;
int dmx_audio_fd = -1;
int dmx_general_fd = -1;
int dmx_pcr_fd = -1;
int dmx_video_fd = -1;
int vbi_fd = -1;

/* channellists */
std::map <uint32_t, uint32_t> allnumchannels_tv;
std::map <uint32_t, uint32_t> allnumchannels_radio;
std::map <std::string, uint32_t> allnamechannels_tv;
std::map <std::string, uint32_t> allnamechannels_radio;

std::map <uint32_t, transponder>transponders;

std::map <uint32_t, CZapitChannel> allchans_tv;
std::map <uint32_t, uint32_t> numchans_tv;
std::map <std::string, uint32_t> namechans_tv;

std::map <uint32_t, CZapitChannel> allchans_radio;
std::map <uint32_t, uint32_t> numchans_radio;
std::map <std::string, uint32_t> namechans_radio;

/* transponder scan */
pthread_t scan_thread;
extern int found_transponders;
extern int found_channels;
extern short curr_sat;
extern short scan_runs;
CZapitClient::bouquetMode bouquetMode = CZapitClient::BM_CREATEBOUQUETS;

#ifndef USE_EXTERNAL_CAMD
pthread_t dec_thread;
#endif /* USE_EXTERNAL_CAMD */

void CZapitDestructor()
{
	save_settings(true);

	if (connfd != -1)
	{
		close(connfd);
	}

	stopPlayBack();

	delete config;
	delete frontend;
	delete cam;

	// remove this in class
	exit(0);
}

void signal_handler (int signum)
{
	switch (signum)
	{
	case SIGUSR1:
		debug = (debug ? false : true);
		break;
	default:
		CZapitDestructor();
	}
}

#ifdef DBOX2
int startVbi (int fd, dvb_pid_t teletext_pid)
{
	if ((teletext_pid == NONE) || (teletext_pid >= INVALID))
	{
		return fd;
	}

	if ((fd == -1) && ((fd = open(VBI_DEV, O_RDWR)) < 0))
	{
		perror ("[zapit] " VBI_DEV);
		return -1;
	}

	if (ioctl(fd, AVIA_VBI_START_VTXT, teletext_pid) < 0)
	{
		perror("[zapit] VBI_START_VTXT");
		close(fd);
		return -1;
	}

	return fd;
}

int stopVbi (int fd)
{
	if (fd == -1)
	{
		return fd;
	}

	if (ioctl(fd, AVIA_VBI_STOP_VTXT, 0) < 0)
	{
		perror("[zapit] VBI_STOP_VTXT");
	}

	close(fd);
	return -1;
}
#endif /* DBOX2 */

int save_settings (bool write)
{
	if (currentMode & RADIO_MODE)
	{
		CBouquetManager::radioChannelIterator cit = g_BouquetMan->radioChannelsFind(channel->getOnidSid());
		if (cit != g_BouquetMan->radioChannelsEnd())
		{
			config->setInt("lastChannelRadio", (*cit)->getChannelNumber());
			config->setInt("lastChannelMode", 1);
		}
	}
	else
	{
		CBouquetManager::tvChannelIterator cit = g_BouquetMan->tvChannelsFind(channel->getOnidSid());
		if (cit != g_BouquetMan->tvChannelsEnd())
		{
			config->setInt("lastChannelTV", (*cit)->getChannelNumber());
			config->setInt("lastChannelMode", 0);
		}
	}

	if (write)
	{
		config->setInt("diseqcRepeats", frontend->getDiseqcRepeats());
		config->setInt("diseqcType", frontend->getDiseqcType());
		config->saveConfig(CONFIGFILE);
	}

	return 0;
}

channel_msg load_settings()
{
	channel_msg output_msg;
	string valueName;
	if (config->getInt("lastChannelMode"))
	{
		output_msg.mode = 'r';
	}
	else
	{
		output_msg.mode = 't';
	}
	valueName = (currentMode & RADIO_MODE) ? "lastChannelRadio" : "lastChannelTV";
	output_msg.chan_nr = config->getInt( valueName, 1);

	return output_msg;
}

#ifndef USE_EXTERNAL_CAMD
void *decode_thread(void *ptr)
{
	decode_vals *vals = (decode_vals *) ptr;

	debug("[zapit] starting decode_thread\n");

	if ((channel->getEcmPid() != NONE) && (channel->getEcmPid() != INVALID))
	{
		debug("[zapit] setting ecm pid %04x\n", channel->getEcmPid());
		cam->setEcm(channel);
	}

	if ((vals->new_tp == true) && (channel->getEmmPid() != NONE) && (channel->getEmmPid() != INVALID))
	{
		debug("[zapit] setting emm pid %04x\n", channel->getEmmPid());
		cam->setEmm(channel);
	}

	delete vals;

	debug("[zapit] ending decode_thread\n");
	dec_thread = 0;
	pthread_exit(0);
}
#endif /* USE_EXTERNAL_CAMD */

int setPesFilter (int fd, dmxOutput_t output, dmxPesType_t pesType, dvb_pid_t pid)
{
	dmxPesFilterParams pesFilterParams;

	if ((pid == NONE) || (pid >= INVALID))
	{
		return fd;
	}

	/* open demux device if needed */
	if ((fd == -1) && ((fd = open(DEMUX_DEV, O_RDWR)) < 0))
	{
		perror("[zapit] " DEMUX_DEV);
		return -1;
	}

	/* setup vpid */
	pesFilterParams.pid = pid;
	pesFilterParams.input = DMX_IN_FRONTEND;
	pesFilterParams.output = output;
	pesFilterParams.pesType = pesType;
	pesFilterParams.flags = DMX_IMMEDIATE_START;

	if (ioctl(fd, DMX_SET_PES_FILTER, &pesFilterParams) < 0)
	{
		perror("[zapit] DMX_SET_PES_FILTER");
		close(fd);
		return -1;
	}

	return fd;
}

int unsetPesFilter (int fd)
{
	if (fd != -1)
	{
		close(fd);
	}

	return -1;
}

bool setAudioBypassMode (bool isAc3)
{
	if (isAc3 == wasAc3)
	{
		return false;
	}

	if (audio_fd != -1)
	{
		close(audio_fd);
	}

	if ((audio_fd = open(AUDIO_DEV, O_RDWR)) < 0)
	{
		perror("[zapit] " AUDIO_DEV);
		return false;
	}

	if (ioctl(audio_fd, AUDIO_SET_BYPASS_MODE, isAc3 ? 0 : 1) < 0)
	{
		perror("[zapit] AUDIO_SET_BYPASS_MODE");
		close(audio_fd);
		return false;
	}

	wasAc3 = isAc3;

	return true;
}

/*
 * - find transponder
 * - stop teletext, video, audio, pcr
 * - tune
 * - set up pids
 * - send channel name to lcdd
 * - start descrambler
 * - start pcr, audio, video, teletext
 */
int zapit (uint32_t onid_sid, bool in_nvod)
{
	std::map <uint, CZapitChannel>::iterator cit;
	bool new_transponder;

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

		if (currentMode & RADIO_MODE)
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
				nvodname = cit->second.getName();
			}
			else
			{
				debug("[zapit] onid_sid %08x not found\n", onid_sid);
				return -3;
			}
		}
	}

	stopPlayBack();

	/* store the new channel */
	if ((channel == NULL) || (onid_sid != channel->getOnidSid()))
	{
		channel = &(cit->second);
	}

	/* if channel's transponder does not match frontend's tuned transponder ... */
	if (channel->getTsidOnid() != frontend->getTsidOnid())
	{
		/* ... tune to it if not in record mode ... */
		if ((!(currentMode & RECORD_MODE)) && (frontend->tuneChannel(channel) == true))
		{
			/* ... and succeed ... */
			new_transponder = true;
		}
		else
		{
			/* ... or fail. */
			return -3;
		}

		cam->reset();
	}
	else
	{
		new_transponder = false;
	}

	if (channel->getServiceType() == NVOD_REFERENCE_SERVICE)
	{
		current_is_nvod = true;
		save_settings(false);
		return 3;
	}

	int fd_pmt = 0;

	/* search pids if they are unknown */
	if (channel->knowsPids() == false)
	{
		/* get program map table pid from program association table */
		debug("[zapit] trying to find pmt for sid %04x, onid %04x\n", channel->getServiceId(), channel->getOriginalNetworkId());

        int fd_cat = init_parse_cat();

		if (channel->knowsPmtId() == false)
		{
			debug("parsing pat\n");
			if (in_nvod)
			{
				parse_pat(channel->getOriginalNetworkId(), &nvodchannels);
			}
			else if (currentMode & RADIO_MODE)
			{
				parse_pat(channel->getOriginalNetworkId(), &allchans_radio);
			}
			else
			{
				parse_pat(channel->getOriginalNetworkId(), &allchans_tv);
			}
		}

		/* parse program map table and store pids */

		fd_pmt = init_parse_pmt(channel->getPmtPid(), channel->getServiceId());
		channel->setPids(parse_pmt(channel->getPmtPid(), cam->getCaSystemId(), fd_pmt));
		fd_pmt = 0;

		/* parse conditional access table and store emm pid */
		channel->setEmmPid(parse_cat(cam->getCaSystemId(), fd_cat ));

		if ((channel->getAudioPid() == NONE) && (channel->getVideoPid() == NONE))
		{
			debug("[zapit] neither audio nor video pid found.\n");
			channel->resetPids();
			return -3;
		}
	}
	else
	{
		debug("[zapit] pids already known\n");

		fd_pmt = init_parse_pmt(channel->getPmtPid(), channel->getServiceId());
	}

#ifdef DBOX2
	if (in_nvod)
	{
		lcdd.setServiceName(nvodname);
	}
	else
	{
		lcdd.setServiceName(cit->second.getName());
	}
#endif /* DBOX2 */


#ifdef USE_EXTERNAL_CAMD
	if ((channel->getEcmPid() != NONE) && (channel->getEcmPid() != INVALID))
	{
		debug("[zapit] setting ecm pid %04x\n", channel->getEcmPid());
		cam->setEcm(channel);
	}

	if ((new_transponder == true) && (channel->getEmmPid() != NONE) && (channel->getEmmPid() != INVALID))
	{
		debug("[zapit] setting emm pid %04x\n", channel->getEmmPid());
		cam->setEmm(channel);
	}
#else
	decode_vals *vals = (decode_vals*) malloc(sizeof(decode_vals));
	vals->new_tp = new_transponder;

	if (dec_thread != 0)
	{
		debug("[zapit] waiting for decode_thread\n");
		pthread_join(dec_thread, 0);
	}

	pthread_create(&dec_thread, 0, decode_thread, (void*) vals);
#endif /* USE_EXTERNAL_CAMD */

	startPlayBack();

	if ( fd_pmt )
		channel->setPids(parse_pmt(channel->getPmtPid(), cam->getCaSystemId(), fd_pmt));

	save_settings(false);
	return 3;
}

int numzap (int channel_number)
{
	std::map<uint, uint>::iterator cit;

	nvodchannels.clear();

	if (currentMode & RADIO_MODE)
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
	{
		return 2;
	}
	else
	{
		return -2;
	}
}

int namezap (std::string channel_name)
{
	std::map<std::string,uint>::iterator cit;

	nvodchannels.clear();

	// Search current channel
	if (currentMode & RADIO_MODE)
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
	{
		return 3;
	}
	else
	{
		return -3;
	}
}

int changeapid (uint8_t pid_nr)
{
	/* invalid pid number? */
	if (pid_nr >= channel->getPids()->count_apids)
	{
		return -8;
	}

	/* stop demux filter */
	dmx_audio_fd = unsetPesFilter(dmx_audio_fd);

	/* set bypass mode */
	setAudioBypassMode(channel->getPids()->apids[pid_nr].is_ac3);

	/* start demux filter */
	dmx_audio_fd = setPesFilter(dmx_audio_fd, DMX_OUT_DECODER, DMX_PES_AUDIO, channel->getPids()->apids[pid_nr].pid);

	/* update current channel */
	channel->setAudioChannel(pid_nr);

	return 8;
}

void shutdownBox ()
{
	if (execlp("/sbin/halt", "halt", NULL) < 0)
	{
		perror("[zapit] execlp");
	}
}

void setRadioMode ()
{
	currentMode |= RADIO_MODE;
	currentMode &= ~TV_MODE;
}

void setTVMode ()
{
	currentMode |= TV_MODE;
	currentMode &= ~RADIO_MODE;
}

void setRecordMode ()
{
	currentMode |= RECORD_MODE;
	eventServer->sendEvent(CZapitClient::EVT_RECORDMODE_ACTIVATED, CEventServer::INITID_ZAPIT );
}

void unsetRecordMode ()
{
	currentMode &= ~RECORD_MODE;
	eventServer->sendEvent(CZapitClient::EVT_RECORDMODE_DEACTIVATED, CEventServer::INITID_ZAPIT );
}

int prepare_channels ()
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

int start_scan ()
{
	if (scanInputParser == NULL)
	{
		printf("[zapit] scan not configured. won't scan.\n");
		return -1;
	}

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

	stopPlayBack();

	if (pthread_create(&scan_thread, 0, start_scanthread, NULL))
	{
		perror("[zapit] pthread_create: scan_thread");
		return -1;
	}

	debug("[zapit] waiting for scan to start\n");

	while (scan_runs == 0)
	{
		/*
		 * this stupid line of code is here just
		 * to workaround a stupid behaviour because
		 * the scan thread won't recognize that scan_runs
		 * changes. maybe it is the compiler's fault.
		 * or maybe i do not know enough about pthreads
		 * to fix this in a better way.
		 * - obi -
		 */

		printf("[zapit] something sucks here.\n");
	}

	return 0;
}

void parseScanInputXml ()
{
	char buffer[2048];
	char filename[32];
	size_t done;
	size_t length;
	FILE *transponder_xml;

	switch (frontend->getInfo()->type)
	{
	case FE_QPSK:
		strcpy(filename, CONFIGDIR "/satellites.xml");
		break;

	case FE_QAM:
		strcpy(filename, CONFIGDIR "/cables.xml");
		break;

	default:
		return;
	}

	transponder_xml = fopen(filename, "r");

	if (transponder_xml == NULL)
	{
		perror(filename);
		return;
	}

	scanInputParser = new XMLTreeParser("ISO-8859-1");

	do
	{
		length = fread(buffer, 1, sizeof(buffer), transponder_xml);
		done = length < sizeof(buffer);

		if (!scanInputParser->Parse(buffer, length, done))
		{
			printf("[zapit] parse error: %s at line %d\n",
				scanInputParser->ErrorString(scanInputParser->GetErrorCode()),
				scanInputParser->GetCurrentLineNumber());

			fclose(transponder_xml);
			delete scanInputParser;
			scanInputParser = NULL;
			return;
		}
	}
	while (!done);

	fclose(transponder_xml);

	if (!scanInputParser->RootNode())
	{
		delete scanInputParser;
		scanInputParser = NULL;
		return;
	}
}

/*
 * get current playback state
 *
 * -1: failure
 *  0: stopped
 *  1: playing
 */
int getPlaybackStatus ()
{
	videoStatus status;

	if (video_fd == -1)
	{
		return 0;
	}

	if (ioctl(video_fd, VIDEO_GET_STATUS, &status) < 0)
	{
		perror("[zapit] VIDEO_GET_STATUS");
		return -1;
	}

	if (status.playState == VIDEO_PLAYING)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


void parse_command ()
{
	char *status;
	short carsten; // who the fuck is short carsten? :)

	std::map<uint, uint>::iterator sit;
	std::map<uint, CZapitChannel>::iterator cit;
	int number = 0;

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
			stopPlayBack();
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
			if (currentMode & RADIO_MODE)
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
						strncpy(chanmsg.name, cit->second.getName().c_str(), 29);
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
						strncpy(chanmsg.name, cit->second.getName().c_str(),29);
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
			if (send(connfd, channel->getPids(), sizeof(pids), 0) == -1)
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

			if (channel == NULL)
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

			carsten = (short) channel->getVideoPid();
			if (send(connfd, &carsten, 2, 0) == -1)
			{
				perror("[zapit] send");
				return;
			}

			carsten = (short) channel->getAudioPid();
			if (send(connfd, &carsten, 2, 0) == -1)
			{
				perror("[zapit] send");
				return;
			}

			break;
		case 'c':
			if (currentMode & RADIO_MODE)
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

						strncpy(chanmsg.name, (*radiocit)->getName().c_str(),30);
						chanmsg.chan_nr = (*radiocit)->getChannelNumber();
						chanmsg.onid_tsid = (*radiocit)->getOnidSid();

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
						strncpy(chanmsg.name, (*tvcit)->getName().c_str(),30);
						chanmsg.chan_nr = (*tvcit)->getChannelNumber();
						chanmsg.onid_tsid = (*tvcit)->getOnidSid();
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
				m_status[1] = channel->getPids()->count_apids & 0x0f;

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
			if (send(connfd, channel->getPids(), sizeof(pids), 0) == -1)
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
				m_status[1] = channel->getPids()->count_apids & 0x0f;

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
			if (send(connfd, channel->getPids(), sizeof(pids), 0) == -1)
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
					strncpy(chanmsg.name, cit->second.getName().c_str(),30);
					chanmsg.chan_nr = number++;
					chanmsg.onid_tsid = cit->second.getOnidSid();

					if (send(connfd, &chanmsg, sizeof(chanmsg), 0) == -1)
					{
						perror("[zapit] send");
						return;
					}
				}
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
					nvodchannels.insert(std::pair<int, CZapitChannel>(nvod_onidsid,CZapitChannel("NVOD",(nvod_onidsid&0xFFFF),nvod_tsid,(nvod_onidsid>>16),1,0,channel->getDiSEqC())));
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
		{
			uint32_t onidsid = channel->getOnidSid();
			status = "00s";
			if (send(connfd, status, strlen(status),0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			if (send(connfd, &onidsid, sizeof(uint32_t), 0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		}
		case 'u':
		{
			dvb_pid_t teletextPid = channel->getTeletextPid();
			status = "00u";
			if (send(connfd, status, strlen(status),0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			if (send(connfd, &teletextPid, sizeof(int),0) == -1)
			{
				perror("[zapit] send");
				return;
			}
			break;
		}
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
				msgCurrentSID.serviceID = channel->getOnidSid();
				send( connfd, &msgCurrentSID, sizeof(msgCurrentSID), 0);
			break;

			case CZapitClient::CMD_GET_CURRENT_SERVICEINFO :
				CZapitClient::responseCurrentServiceInfo msgCurrentServiceInfo;
				msgCurrentServiceInfo.onid = channel->getOriginalNetworkId();
				msgCurrentServiceInfo.sid = channel->getServiceId();
				msgCurrentServiceInfo.tsid = channel->getTransportStreamId();
				msgCurrentServiceInfo.vdid = channel->getVideoPid();
				msgCurrentServiceInfo.apid = channel->getAudioPid();
				msgCurrentServiceInfo.vtxtpid = channel->getTeletextPid();
				msgCurrentServiceInfo.pcrpid = channel->getPcrPid();;

				send( connfd, &msgCurrentServiceInfo, sizeof(msgCurrentServiceInfo), 0);
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
				start_scan();
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

			case CZapitClient::CMD_SCANGETSATLIST:
			{
				if (scanInputParser == NULL)
				{
					parseScanInputXml();

					if (scanInputParser == NULL)
					{
						break;
					}
				}

				CZapitClient::responseGetSatelliteList msgResponseGetSatelliteList;
				XMLTreeNode *search = scanInputParser->RootNode()->GetChild();

				while (search != NULL)
				{
					strncpy(msgResponseGetSatelliteList.satName, search->GetAttributeValue("name"), sizeof(msgResponseGetSatelliteList.satName));
					send(connfd, &msgResponseGetSatelliteList, sizeof(msgResponseGetSatelliteList), 0);
					search = search->GetNext();
				}
				break;
			}
			case CZapitClient::CMD_SCANSETSCANSATLIST:
				CZapitClient::commandSetScanSatelliteList sat;
				scanProviders.clear();
				while (read(connfd, &sat, sizeof(sat)))
				{
					printf("[zapit] adding %s (diseqc %d)\n", sat.satName, sat.diseqc);
					scanProviders[sat.diseqc] = sat.satName;
				}
				break;

			case CZapitClient::CMD_SCANSETDISEQCTYPE :
				diseqc_t diseqc;
				read(connfd, &diseqc, sizeof(diseqc));
				frontend->setDiseqcType(diseqc);
				printf("[zapit] set diseqc type %d\n", diseqc);
				break;

			case CZapitClient::CMD_SCANSETDISEQCREPEAT :
				uint32_t repeats;
				read(connfd, &repeats, sizeof(repeats));
				frontend->setDiseqcRepeats(repeats);
				printf("[zapit] set diseqc repeats to %d\n", repeats);
				break;

			case CZapitClient::CMD_SCANSETBOUQUETMODE :
				read( connfd, &bouquetMode, sizeof(bouquetMode));
			break;

			case CZapitClient::CMD_SET_RECORD_MODE :
				CZapitClient::commandSetRecordMode msgSetRecordMode;
				read( connfd, &msgSetRecordMode, sizeof(msgSetRecordMode));
				if(msgSetRecordMode.activate)
					setRecordMode();
				else
					unsetRecordMode();
			break;

			case CZapitClient::CMD_GET_RECORD_MODE :
				CZapitClient::responseGetRecordModeState msgGetRecordModeState;
				msgGetRecordModeState.activated = (currentMode & RECORD_MODE);
				send( connfd, &msgGetRecordModeState, sizeof(msgGetRecordModeState),0);
			break;

			case CZapitClient::CMD_SB_GET_PLAYBACK_ACTIVE :
				CZapitClient::responseGetPlaybackState msgGetPlaybackState;
				msgGetPlaybackState.activated = getPlaybackStatus ();
				send( connfd, &msgGetPlaybackState, sizeof(msgGetPlaybackState),0);
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
					(((currentMode & RADIO_MODE) && msgMoveChannel.mode == CZapitClient::MODE_CURRENT ) || (msgMoveChannel.mode==CZapitClient::MODE_RADIO)) ? 2 : 1);
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
				responseGetOtherPIDs.vpid = channel->getVideoPid();
				responseGetOtherPIDs.ecmpid = channel->getEcmPid();
				responseGetOtherPIDs.vtxtpid = channel->getTeletextPid();
				responseGetOtherPIDs.pcrpid = channel->getPcrPid();
				responseGetOtherPIDs.selected_apid = channel->getAudioChannel();
				send( connfd, &responseGetOtherPIDs, sizeof(responseGetOtherPIDs),0);
				sendAPIDs();
			break;

			case CZapitClient::CMD_SETSUBSERVICES :
				CZapitClient::commandAddSubServices msgAddSubService;

				while ( read( connfd, &msgAddSubService, sizeof(msgAddSubService)))
				{
					//printf("got subchan %x %x\n", msgAddSubService.onidsid, msgAddSubService.tsid);
					nvodchannels.insert
					(
						std::pair <int, CZapitChannel>
						(
							msgAddSubService.onidsid,
							CZapitChannel
							(
								"NVOD",
								(msgAddSubService.onidsid&0xFFFF),
								msgAddSubService.tsid,
								(msgAddSubService.onidsid>>16),
								1,
								0,
								channel->getDiSEqC()
							)
						)
					);
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
	uint32_t i;
	char* status = "00q";

	if (send(connfd, status, strlen(status),0) == -1)
	{
		perror("[zapit] could not send any return\n");
		return;
	}

	uint nBouquetCount = 0;
	for (i = 0; i < g_BouquetMan->Bouquets.size(); i++)
	{
		if ((currentMode & RADIO_MODE) && (g_BouquetMan->Bouquets[i]->radioChannels.size() > 0) ||
			(currentMode & TV_MODE) && (g_BouquetMan->Bouquets[i]->tvChannels.size() > 0))
		{
			nBouquetCount++;
		}
	}

	if (send(connfd, &nBouquetCount, sizeof(nBouquetCount), 0) == -1)
	{
		perror("[zapit] could not send any return\n");
		return;
	}
	else
	{
		for (i = 0; i < g_BouquetMan->Bouquets.size(); i++)
		{
			// send bouquet only if there are channels in it
			if ((currentMode & RADIO_MODE) && (g_BouquetMan->Bouquets[i]->radioChannels.size() > 0) ||
				(currentMode & TV_MODE) && (g_BouquetMan->Bouquets[i]->tvChannels.size() > 0))
			{
				bouquet_msg msgBouquet;
				// we'll send name and i+1 as bouquet number
				strncpy(msgBouquet.name, g_BouquetMan->Bouquets[i]->Name.c_str(), 30);
				msgBouquet.bouquet_nr = i + 1;

				if (send(connfd, &msgBouquet, sizeof(msgBouquet), 0) == -1)
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
	uint32_t i;

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

	if (send(connfd, status, strlen(status), 0) == -1)
	{
		perror("[zapit] could not send any return\n");
		return;
	}

	std::vector<CZapitChannel*> channels;

	if (currentMode & RADIO_MODE)
	{
		channels = g_BouquetMan->Bouquets[nBouquet]->radioChannels;
	}
	else
	{
		channels = g_BouquetMan->Bouquets[nBouquet]->tvChannels;
	}

	if (!channels.empty())
	{
		for (i = 0; i < channels.size(); i++)
		{
			if ((currentMode & RECORD_MODE) && (channels[i]->getTsidOnid() != frontend->getTsidOnid()))
					continue;

			channel_msg_2 chanmsg;
			strncpy(chanmsg.name, channels[i]->getName().c_str(), 30);
			chanmsg.onid_tsid = channels[i]->getOnidSid();
			chanmsg.chan_nr = channels[i]->getChannelNumber();

			if (send(connfd, &chanmsg, sizeof(chanmsg), 0) == -1)
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

	printf("$Id: zapit.cpp,v 1.170 2002/05/04 14:39:57 McClean Exp $\n\n");

	if (argc > 1)
	{
		for (i = 1; i < argc ; i++)
		{
			if (!strcmp(argv[i], "-d"))
			{
				debug = true;
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
			else
			{
				printf("Usage: zapit [-d] [-q]\n");
				exit(0);
			}
		}
	}

	scan_runs = 0;
	found_transponders = 0;
	found_channels = 0;
	curr_sat = -1;

	/* load configuration */
	config = new CConfigFile(',');

	char tmp[16];

	if (!config->loadConfig(CONFIGDIR "/zapit/zapit.conf"))
	{
		/* set defaults if no configuration file exists */
		config->setInt("diseqcRepeats", 0);
		config->setInt("diseqcType", NO_DISEQC);
		config->setInt("lastChannel", 1);
		config->setInt("lastChannelMode", 0);
		for (i = 0; i < MAX_LNBS; i++)
		{
			sprintf(tmp, "lnb%d_OffsetHigh", i);
			config->setInt(tmp, 10600000);
			sprintf(tmp, "lnb%d_OffsetLow", i);
			config->setInt(tmp, 9750000);
		}
	}

	/* initialize frontend */
	frontend = new CFrontend();

	if (!frontend->isInitialized())
	{
		printf("[zapit] unable to open frontend devices. bye.\n");
		CZapitDestructor();
	}
	else
	{
		frontend->setDiseqcType((diseqc_t) config->getInt("diseqcType"));
		frontend->setDiseqcRepeats(config->getInt("diseqcRepeats"));

		for (i = 0; i < MAX_LNBS; i++)
		{
			/* low offset */
			sprintf(tmp, "lnb%d_OffsetLow", i);
			frontend->setLnbOffset(false, i, config->getInt(tmp));
			/* high offset */
			sprintf(tmp, "lnb%d_OffsetHigh", i);
			frontend->setLnbOffset(true, i, config->getInt(tmp));
		}
	}

	/* initialize cam */
	cam = new CCam();

	if (!cam->isInitialized())
	{
		printf("[zapit] unable to initialize cam. bye.\n");
		CZapitDestructor();
	}
	else
	{
		debug("[zapit] ca_system_id %04x\n", cam->getCaSystemId());
	}

	/* create bouquet manager */
	g_BouquetMan = new CBouquetManager();

	testmsg = load_settings();

	if (testmsg.mode == 'r')
	{
		setRadioMode();
	}
	else
	{
		setTVMode();
	}

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
	std::string filename = ZAPIT_UDS_NAME;
	filename += ".";
	filename += CZapitClient::getSystemId();

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, filename.c_str());
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	unlink(filename.c_str());

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

	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGUSR1, signal_handler);

	if (debug == false)
	{
		switch (fork())
		{
		case -1: /* can't fork */
			perror("[zapit] fork");
			exit(3);
		case 0: /* child, process becomes a daemon */
			// request a new session (job control)
			if (setsid() == -1)
			{
				exit(4);
			}
			break;
		default: /* parent returns to calling process */
			return 0;
		}
	}

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
	CZapitChannel* chan = g_BouquetMan->copyChannelByOnidSid( onid_sid);
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
		if (emptyBouquetsToo ||
			 ((currentMode & RADIO_MODE) && (g_BouquetMan->Bouquets[i]->radioChannels.size()> 0) && (!g_BouquetMan->Bouquets[i]->bHidden)) ||
			  (currentMode & TV_MODE) && (g_BouquetMan->Bouquets[i]->tvChannels.size()> 0) && (!g_BouquetMan->Bouquets[i]->bHidden))
		{
			if ((!(currentMode & RECORD_MODE)) || ((currentMode & RECORD_MODE) &&
			    (((currentMode & RADIO_MODE) && (g_BouquetMan->Bouquets[i]->recModeRadioSize( frontend->getTsidOnid())) > 0 ) ||
			      (currentMode & TV_MODE)    && (g_BouquetMan->Bouquets[i]->recModeTVSize( frontend->getTsidOnid())) > 0 )))
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
}

void internalSendChannels(ChannelList* channels)
{
	for (uint i = 0; i < channels->size();i++)
	{
		if ((currentMode & RECORD_MODE) && ((*channels)[i]->getTsidOnid() != frontend->getTsidOnid()))
			continue;

		CZapitClient::responseGetBouquetChannels response;
		strncpy(response.name, (*channels)[i]->getName().c_str(),30);
		response.onid_sid = (*channels)[i]->getOnidSid();
		response.nr = (*channels)[i]->getChannelNumber();

		if (send(connfd, &response, sizeof(response),0) == -1)
		{
			perror("[zapit] could not send any return\n");
			return;
		}
	}
}

void sendAPIDs()
{
	for (uint i = 0; i < channel->getPids()->count_apids; i++)
	{
		CZapitClient::responseGetAPIDs response;
		response.pid = channel->getPids()->apids[i].pid;
		strncpy(response.desc, channel->getPids()->apids[i].desc, 25);
		response.is_ac3 = channel->getPids()->apids[i].is_ac3;
		response.component_tag = channel->getPids()->apids[i].component_tag;

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

	if ((bouquet < 0) || (bouquet>g_BouquetMan->Bouquets.size()))
	{
		printf("[zapit] invalid bouquet number: %d",bouquet);
		return;
	}

	ChannelList channels;

	if (((currentMode & RADIO_MODE) && (mode == CZapitClient::MODE_CURRENT)) || (mode == CZapitClient::MODE_RADIO))
	{
		channels = g_BouquetMan->Bouquets[bouquet]->radioChannels;
	}
	else
	{
		channels = g_BouquetMan->Bouquets[bouquet]->tvChannels;
	}

	internalSendChannels( &channels);
}

void sendChannels( CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, CZapitClient::channelsOrder order = CZapitClient::SORT_BOUQUET)
{
	ChannelList channels;

	if (order == CZapitClient::SORT_BOUQUET)
	{
		if (((currentMode & RADIO_MODE) && (mode == CZapitClient::MODE_CURRENT)) || (mode==CZapitClient::MODE_RADIO))
		{
			for (CBouquetManager::radioChannelIterator radiocit = g_BouquetMan->radioChannelsBegin(); radiocit != g_BouquetMan->radioChannelsEnd(); radiocit++)
			{
				channels.insert(channels.end(), (*radiocit));
			}
		}
		else
		{
			for (CBouquetManager::tvChannelIterator tvcit = g_BouquetMan->tvChannelsBegin(); tvcit != g_BouquetMan->tvChannelsEnd(); tvcit++)
			{
				channels.insert(channels.end(), (*tvcit));
			}
		}
	}
	else if (order == CZapitClient::SORT_ALPHA)
	{
		if (((currentMode & RADIO_MODE) && (mode == CZapitClient::MODE_CURRENT)) || (mode==CZapitClient::MODE_RADIO))
		{
			for ( map<uint, CZapitChannel>::iterator it=allchans_radio.begin(); it!=allchans_radio.end(); it++)
			{
				channels.insert( channels.end(), &(it->second));
			}
		}
		else
		{
			for ( map<uint, CZapitChannel>::iterator it=allchans_tv.begin(); it!=allchans_tv.end(); it++)
			{
				channels.insert( channels.end(), &(it->second));
			}
		}
		sort(channels.begin(), channels.end(), CmpChannelByChName());
	}

	internalSendChannels( &channels);
}

int startPlayBack()
{
	videoStatus video_status;

	/* open video device */
	if ((video_fd == -1) && ((video_fd = open(VIDEO_DEV, O_RDWR)) < 0))
	{
		perror("[zapit] " VIDEO_DEV);
		return -1;
	}

	/* get video status */
	if (ioctl(video_fd, VIDEO_GET_STATUS, &video_status) < 0)
	{
		perror("[zapit] VIDEO_GET_STATUS");
		return -1;
	}

	/* select video source */
#ifndef ALWAYS_DO_VIDEO_SELECT_SOURCE
	if ((video_status.streamSource != VIDEO_SOURCE_DEMUX) && (video_status.playState == VIDEO_STOPPED))
#else
	if (video_status.playState == VIDEO_STOPPED)
#endif
	{
		if (ioctl(video_fd, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_DEMUX) < 0)
		{
			perror("[zapit] VIDEO_SELECT_SOURCE");
		}
	}

	/* start video */
	if ((video_status.playState != VIDEO_PLAYING) && (ioctl(video_fd, VIDEO_PLAY) < 0))
	{
		perror("[zapit] VIDEO_PLAY");
		return -1;
	}

	/* set bypass mode */
	setAudioBypassMode(channel->getPids()->apids[channel->getAudioChannel()].is_ac3);

	/* start demux filters */
	dmx_pcr_fd = setPesFilter(dmx_pcr_fd, DMX_OUT_DECODER, DMX_PES_PCR, channel->getPcrPid());
	dmx_audio_fd = setPesFilter(dmx_audio_fd, DMX_OUT_DECODER, DMX_PES_AUDIO, channel->getAudioPid());
	dmx_video_fd = setPesFilter(dmx_video_fd, DMX_OUT_DECODER, DMX_PES_VIDEO, channel->getVideoPid());

#if 0
	dmx_general_fd = setPesFilter(dmx_general_fd, DMX_OUT_TAP, DMX_PES_OTHER, channel->getAudioPid());

	bool indicator = true;
	struct pollfd pfd[1];
	uint8_t i;
	uint8_t buf[16384];
	uint16_t j;
	uint8_t k = 8;
	ssize_t len;

	pfd[0].fd = dmx_general_fd;
	pfd[0].events = POLLIN;

	if ((dmx_general_fd != -1) && (poll(pfd, 1, 1000) > 0) && (pfd[0].revents & POLLIN))
	{
		for (i = 0; i < k; i++)
		{
			if ((len = read(dmx_general_fd, buf, sizeof(buf))) < 0)
			{
				perror("[zapit] read");
				break;
			}

			for (j = 0; j < len; j++)
			{
				if ((buf[j] == 0x00) && (buf[j+1] == 0x00) && (buf[j+2] == 0x01))
				{
					switch (buf[j+3])
					{
					case 0xC0 ... 0xDF:
						if (((buf[j+6] >> 6) & 0x03) == 2)
						{
							if (((buf[j+6] >> 4) & 0x03) == 0)
							{
								indicator = false;
							}
						}
						break;
					}
				}

				if (indicator == false)
				{
					break;
				}
			}

			if (indicator == false)
			{
				break;
			}
		}
	}

	dmx_general_fd = unsetPesFilter(dmx_general_fd);

	printf("[zapit] indicator: %d\n", indicator);
#endif

#ifdef DBOX2
	vbi_fd = startVbi(vbi_fd, channel->getTeletextPid());
#endif /* DBOX2 */

	return 0;
}

int stopPlayBack()
{

#ifndef DBOX2
	vbi_fd = stopVbi(vbi_fd);
#endif /* DBOX2 */

	if (audio_fd != -1)
	{
		close(audio_fd);
		audio_fd = -1;
	}

	if (video_fd != -1)
	{
		if (ioctl(video_fd, VIDEO_STOP, 1) < 0)
		{
			perror("[zapit] VIDEO_STOP");
		}

		close(video_fd);
		video_fd = -1;
	}

	dmx_video_fd = unsetPesFilter(dmx_video_fd);
	dmx_audio_fd = unsetPesFilter(dmx_audio_fd);
	dmx_pcr_fd = unsetPesFilter(dmx_pcr_fd);

	return 0;
}

unsigned zapTo (unsigned int bouquet, unsigned int channel)
{
	if ((bouquet < 1) || (bouquet > g_BouquetMan->Bouquets.size()))
	{
		printf("[zapit] Invalid bouquet %d\n", bouquet);
		return CZapitClient::ZAP_INVALID_PARAM;
	}

	ChannelList channels;

	if (currentMode & RADIO_MODE)
	{
		channels = g_BouquetMan->Bouquets[bouquet - 1]->radioChannels;
	}
	else
	{
		channels = g_BouquetMan->Bouquets[bouquet - 1]->tvChannels;
	}

	if ((channel < 1) || (channel > channels.size()))
	{
		printf("[zapit] Invalid channel %d in bouquet %d\n", channel, bouquet);
		return CZapitClient::ZAP_INVALID_PARAM;
	}

	g_BouquetMan->saveAsLast(bouquet - 1, channel - 1);

	return zapTo_ServiceID(channels[channel - 1]->getOnidSid(), false);
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
		eventServer->sendEvent(( isSubService )?CZapitClient::EVT_ZAP_SUB_FAILED:CZapitClient::EVT_ZAP_FAILED, CEventServer::INITID_ZAPIT, &serviceID, sizeof(serviceID) );
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

	if (currentMode & RADIO_MODE)
	{
		CBouquetManager::radioChannelIterator radiocit = g_BouquetMan->radioChannelsBegin();
		while ((radiocit != g_BouquetMan->radioChannelsEnd()) && (channel>1))
		{
			radiocit++;
			channel--;
		}
	//	g_BouquetMan->saveAsLast( bouquet-1, channel-1);
		if (radiocit != g_BouquetMan->radioChannelsEnd())
			result = zapTo_ServiceID ( (*radiocit)->getOnidSid(), false );
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
			result = zapTo_ServiceID ( (*tvcit)->getOnidSid(), false );
	}

	return result;
}

