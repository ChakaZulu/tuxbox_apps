/*
 * $Id: zapit.cpp,v 1.349 2004/04/07 19:33:21 thegoodguy Exp $
 *
 * zapit - d-box2 linux project
 *
 * (C) 2001, 2002 by Philipp Leusmann <faralla@berlios.de>
 * (C) 2002, 2003 by Andreas Oberritter <obi@tuxbox.org>
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

/* system headers */
#include <csignal>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* tuxbox headers */
#include <configfile.h>
#include <connection/basicserver.h>

/* zapit headers */
#include <zapit/audio.h>
#include <zapit/cam.h>
#include <zapit/client/msgtypes.h>
#include <zapit/dmx.h>
#include <zapit/debug.h>
#include <zapit/frontend.h>
#include <zapit/getservices.h>
#include <zapit/pat.h>
#include <zapit/pmt.h>
#include <zapit/scan.h>
#include <zapit/settings.h>
#include <zapit/video.h>
#include <zapit/xmlinterface.h>
#include <zapit/zapit.h>


/* the conditional access module */
CCam *cam = NULL;
/* the configuration file */
CConfigFile config(',', false);
/* the event server */
CEventServer *eventServer = NULL;
/* the dvb audio device */
CAudio *audioDecoder = NULL;
/* the dvb frontend device */
CFrontend *frontend = NULL;
/* the dvb video device */
CVideo *videoDecoder = NULL;
/* the current channel */
CZapitChannel *channel = NULL;
/* the transponder scan xml input */
xmlDocPtr scanInputParser = NULL;
/* the bouquet manager */
CBouquetManager *bouquetManager = NULL;
/* the mpeg2 ts->pes demux devices */
CDemux *audioDemux = NULL;
CDemux *pcrDemux = NULL;
CDemux *teletextDemux = NULL;
CDemux *videoDemux = NULL;

/* current zapit mode */
enum {
	TV_MODE = 0x01,
	RADIO_MODE = 0x02,
	RECORD_MODE = 0x04
};

int currentMode;
bool playbackStopForced = false;
int debug = 0;

int waitForMotor = 0;
int motorRotationSpeed = 0; //in 0.1 degrees per second
diseqc_t diseqcType;

/* near video on demand */
tallchans nvodchannels;         //  tallchans defined in "bouquets.h"
bool current_is_nvod = false;

/* list of all channels (services) */
tallchans allchans;             //  tallchans defined in "bouquets.h"

/* transponder scan */
std::map <uint32_t, transponder>transponders;
pthread_t scan_thread;
extern int found_transponders;
extern int processed_transponders;
extern int found_channels;
extern short curr_sat;
extern short scan_runs;
extern void get_transponder (TP_params *TP);
extern void scan_transponder(TP_params *TP);

CZapitClient::bouquetMode bouquetMode = CZapitClient::BM_CREATEBOUQUETS;

/* the map which stores the wanted cable/satellites */
std::map<uint8_t, std::string> scanProviders;
/* the map which stores the diseqc 1.2 motor positions */
extern std::map<t_satellite_position, uint8_t> motorPositions;
extern std::map<t_satellite_position, uint8_t>::iterator mpos_it;

extern std::map<string, t_satellite_position> satellitePositions;
extern std::map<string, t_satellite_position>::iterator spos_it;

extern std::map<string, int> satelliteDiseqcs; //diseqcs per satellite
extern std::map<string, int>::iterator satdiseqc_it;

bool standby = true;

uint32_t lastChannelRadio;
uint32_t lastChannelTV;

void saveSettings(bool write)
{
	if (channel) {
		// now save the lowest channel number with the current channel_id
		int c = ((currentMode & RADIO_MODE) ? bouquetManager->radioChannelsBegin() : bouquetManager->tvChannelsBegin()).getLowestChannelNumberWithChannelID(channel->getChannelID());

		if (c >= 0)
			if ((currentMode & RADIO_MODE))
				lastChannelRadio = c;
			else
				lastChannelTV = c;
	}

	if (write) {
		if (config.getBool("saveLastChannel", true))
		{
			config.setInt32("lastChannelMode", (currentMode & RADIO_MODE) ? 1 : 0);

			config.setInt32("lastChannelRadio", lastChannelRadio);
			config.setInt32("lastChannelTV", lastChannelTV);
		}

		config.setInt32("lastSatellitePosition", frontend->getCurrentSatellitePosition());
		config.setInt32("diseqcRepeats", frontend->getDiseqcRepeats());
		config.setInt32("diseqcType", frontend->getDiseqcType());

		if (config.getModifiedFlag())
			config.saveConfig(CONFIGFILE);
	}
}

CZapitClient::responseGetLastChannel load_settings(void)
{
	CZapitClient::responseGetLastChannel lastchannel;

	if (currentMode & RADIO_MODE)
		lastchannel.mode = 'r';
	else
		lastchannel.mode = 't';

	lastchannel.channelNumber = (currentMode & RADIO_MODE) ? lastChannelRadio : lastChannelTV;
	return lastchannel;
}


/*
 * - find transponder
 * - stop teletext, video, audio, pcr
 * - tune
 * - set up pids
 * - start pcr, audio, video, teletext
 * - start descrambler
 *
 * return 0 on success
 * return -1 otherwise
 *
 */

static uint32_t tuned_transponder_id = 0;
static int pmt_update_fd = -1;
static bool update_pmt = false;

int zapit(const t_channel_id channel_id, bool in_nvod, uint32_t tsid_onid)
{
	bool transponder_change;
	tallchans_iterator cit;
	uint32_t current_transponder_id;

	DBG("tuned_transponder_id: %08x", tuned_transponder_id);

	/* usual zap */
	if (!tsid_onid) {
		if (in_nvod) {
			current_is_nvod = true;
			cit = nvodchannels.find(channel_id);

			if (cit == nvodchannels.end()) {
				DBG("channel_id " PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS " not found", channel_id);
				return -1;
			}
		}
		else {
			current_is_nvod = false;
			cit = allchans.find(channel_id);

			if (currentMode & RADIO_MODE) {
				if ((cit == allchans.end()) || (cit->second.getServiceType() != ST_DIGITAL_RADIO_SOUND_SERVICE)) {
					DBG("channel_id " PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS " not found", channel_id);
					return -1;
				}
			}
			else {
				if (cit == allchans.end() || (cit->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE)) {
					DBG("channel_id " PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS " not found", channel_id);
					return -1;
				}
			}
		}

		/* store the new channel */
		if ((!channel) || (channel_id != channel->getChannelID()))
			channel = &(cit->second);

		current_transponder_id = channel->getTsidOnid();
	}

	/* nvod subservice zap */
	else {
		current_transponder_id = tsid_onid;
	}

	stopPlayBack();

	/* have motor move satellite dish to satellite's position if necessary */
	if ((diseqcType == DISEQC_1_2) && (motorPositions[channel->getSatellitePosition()] != 0))
	{
		if ((frontend->getCurrentSatellitePosition() != channel->getSatellitePosition()))
		{
			printf("[zapit] currentSatellitePosition = %d, satellitePosition = %d\n", frontend->getCurrentSatellitePosition(), channel->getSatellitePosition());
			printf("[zapit] motorPosition = %d\n", motorPositions[channel->getSatellitePosition()]);
			frontend->positionMotor(motorPositions[channel->getSatellitePosition()]);

			waitForMotor = abs(channel->getSatellitePosition() - frontend->getCurrentSatellitePosition()) / motorRotationSpeed; //assuming 1.8 degrees/second motor rotation speed for the time being...
			printf("[zapit] waiting %d seconds for motor to turn satellite dish.\n", waitForMotor);
			eventServer->sendEvent(CZapitClient::EVT_ZAP_MOTOR, CEventServer::INITID_ZAPIT, &waitForMotor, sizeof(waitForMotor));
			sleep(waitForMotor);

			frontend->setCurrentSatellitePosition(channel->getSatellitePosition());
		}
	}

	/* if channel's transponder does not match the transponder tuned before ... */
	if (current_transponder_id != tuned_transponder_id) {

		/* ... tune to it if not in record mode ... */
		if (currentMode & RECORD_MODE)
			return -1;

		/* find transponder which matches tsid onid pair */
		std::map <uint32_t, transponder>::iterator t;
		t = transponders.find(current_transponder_id);
		if (t == transponders.end())
			return -1;

		int diff = frontend->setParameters(
				&t->second.feparams, t->second.polarization, t->second.DiSEqC);
		switch (diff) {
		case -1:
			WARN("tuning failed\n");
			tuned_transponder_id = 0;
			return -1;
		case 0:
			break;
		default:
			WARN("tuned frequency does not match requested frequency. difference: %u\n", diff);
			break;
		}

		transponder_change = true;

		tuned_transponder_id = current_transponder_id;
	}
	else {
		transponder_change = false;
	}

	CZapitChannel *thisChannel;

	if (!tsid_onid) {
		thisChannel = channel;
		if (thisChannel->getServiceType() == ST_NVOD_REFERENCE_SERVICE) {
			current_is_nvod = true;
			saveSettings(false);
			return 0;
		}
	}

	else {
		thisChannel = new CZapitChannel(channel->getName(),
					channel_id & 0xffff, (tsid_onid >> 16) & 0xffff,
					tsid_onid & 0xffff, 1, frontend->getDiseqcPosition(),
					channel->getSatellitePosition());
	}

	/* search pids if they are unknown */
#ifdef USE_PID_CACHE
	if (thisChannel->getPidsFlag() == false)
#endif
	{
		bool failed = false;
      unsigned char audioChannel = thisChannel->getAudioChannelIndex();

		thisChannel->resetPids();

		DBG("looking up pids for channel_id " PRINTF_CHANNEL_ID_TYPE, thisChannel->getChannelID());

		/* get program map table pid from program association table */
		if (thisChannel->getPmtPid() == NONE)
			if (parse_pat(thisChannel) < 0) {
				WARN("pat parsing failed");
				failed = true;
			}

		/* parse program map table and store pids */
		if ((!failed) && (parse_pmt(thisChannel) < 0)) {
			WARN("pmt parsing failed");
			failed = true;
		}

      thisChannel->setAudioChannel(audioChannel);

		if ((!failed) && (thisChannel->getAudioPid() == NONE) && (thisChannel->getVideoPid() == NONE)) {
			WARN("neither audio nor video pid found");
			failed = true;
		}

		if (failed) {
			thisChannel->resetPids();
			if (channel != thisChannel)
				delete thisChannel;
			return -1;
		}
	}

	if (transponder_change == true)
		thisChannel->getCaPmt()->ca_pmt_list_management = 0x03;
	else
		thisChannel->getCaPmt()->ca_pmt_list_management = 0x04;

	startPlayBack(thisChannel);
	cam->setCaPmt(thisChannel->getCaPmt());
	saveSettings(false);

	if (update_pmt)
		pmt_set_update_filter(thisChannel, &pmt_update_fd);

	if (channel != thisChannel)
		delete thisChannel;

	return 0;
}

int select_nvod_subservice_num(int num)
{
	t_original_network_id original_network_id;
	t_transport_stream_id transport_stream_id;
	t_service_id service_id;

	if ((!channel) || (channel->getServiceType() != ST_NVOD_REFERENCE_SERVICE) || (num < 0))
		return -1;

	if (channel->getTsidOnid() != tuned_transponder_id)
		zapit(channel->getChannelID(), false, 0);

	if (nvod_service_ids(channel->getTransportStreamId(), channel->getOriginalNetworkId(), channel->getServiceId(),
				num, &transport_stream_id, &original_network_id, &service_id) < 0)
		return -1;

	DBG("tsid: %04x, onid: %04x, sid: %04x\n", transport_stream_id, original_network_id, service_id);

	return zapit(CREATE_CHANNEL_ID, false, (transport_stream_id << 16) | original_network_id);
}

int change_audio_pid(uint8_t index)
{
	if ((!audioDemux) || (!audioDecoder) || (!channel))
		return -1;

	/* stop demux filter */
	if (audioDemux->stop() < 0)
		return -1;

	/* stop audio playback */
	if (audioDecoder->stop() < 0)
		return -1;

	/* update current channel */
	channel->setAudioChannel(index);

	/* set bypass mode */
	CZapitAudioChannel *currentAudioChannel = channel->getAudioChannel();

	if (!currentAudioChannel) {
		WARN("No current audio channel");
		return -1;
	}

	if (currentAudioChannel->isAc3)
		audioDecoder->enableBypass();
	else
		audioDecoder->disableBypass();

	/* set demux filter */
	if (audioDemux->pesFilter(channel->getAudioPid(), DMX_OUT_DECODER, DMX_PES_AUDIO) < 0)
		return -1;

	/* start audio playback */
	if (audioDecoder->start() < 0)
		return -1;

	/* start demux filter */
	if (audioDemux->start() < 0)
		return -1;

	return 0;
}

void setRadioMode(void)
{
	currentMode |= RADIO_MODE;
	currentMode &= ~TV_MODE;
}

void setTVMode(void)
{
	currentMode |= TV_MODE;
	currentMode &= ~RADIO_MODE;
}

int getMode(void)
{
	if (currentMode & TV_MODE)
		return CZapitClient::MODE_TV;
	if (currentMode & RADIO_MODE)
		return CZapitClient::MODE_RADIO;
	return 0;
}

void setRecordMode(void)
{
	currentMode |= RECORD_MODE;
	eventServer->sendEvent(CZapitClient::EVT_RECORDMODE_ACTIVATED, CEventServer::INITID_ZAPIT );
}

void unsetRecordMode(void)
{
	currentMode &= ~RECORD_MODE;
	eventServer->sendEvent(CZapitClient::EVT_RECORDMODE_DEACTIVATED, CEventServer::INITID_ZAPIT );
}

int prepare_channels(fe_type_t frontendType, diseqc_t diseqcType)
{
	// for the case this function is NOT called for the first time (by main())
	// we clear all cannel lists, they are refilled
	// by LoadServices() and LoadBouquets()
	transponders.clear();
	bouquetManager->clearAll();
	allchans.clear();  // <- this invalidates all bouquets, too!
	if (LoadServices(frontendType, diseqcType) < 0)
		return -1;

	INFO("LoadServices: success");
	bouquetManager->loadBouquets();
	bouquetManager->storeBouquets();

	return 0;
}


void parseScanInputXml(void)
{
	switch (frontend->getInfo()->type) {
	case FE_QPSK:
		scanInputParser = parseXmlFile(SATELLITES_XML);
		break;

	case FE_QAM:
		scanInputParser = parseXmlFile(CABLES_XML);
		break;

	case FE_OFDM:
		scanInputParser = parseXmlFile(TERRESTRIAL_XML);
		break;

	default:
		WARN("Unknown type %d", frontend->getInfo()->type);
		return;
	}
}

/*
 * return 0 on success
 * return -1 otherwise
 */
int start_scan(void)
{
	if (!scanInputParser) {
		parseScanInputXml();
		if (!scanInputParser) {
			WARN("scan not configured");
			return -1;
		}
	}

	transponders.clear();
	bouquetManager->clearAll();
	allchans.clear();  // <- this invalidates all bouquets, too!
	stopPlayBack();

	tuned_transponder_id = 0;
	found_transponders = 0;
	found_channels = 0;
	scan_runs = 1;

	if (pthread_create(&scan_thread, 0, start_scanthread, NULL)) {
		ERROR("pthread_create");
		scan_runs = 0;
		return -1;
	}

	return 0;
}


bool parse_command(CBasicMessage::Header &rmsg, int connfd)
{
	DBG("cmd %d (version %d) received", rmsg.cmd, rmsg.version);

	if ((standby) && 
			((rmsg.cmd != CZapitMessages::CMD_SET_STANDBY) &&
			(rmsg.cmd != CZapitMessages::CMD_SHUTDOWN))) {
		WARN("cmd %d refused in standby mode", rmsg.cmd);
		return true;
	}

	switch (rmsg.cmd) {
	case CZapitMessages::CMD_SHUTDOWN:
		return false;

	case CZapitMessages::CMD_ZAPTO:
	{
		CZapitMessages::commandZapto msgZapto;
		CBasicServer::receive_data(connfd, &msgZapto, sizeof(msgZapto)); // bouquet & channel number are already starting at 0!
		zapTo(msgZapto.bouquet, msgZapto.channel);
		break;
	}

	case CZapitMessages::CMD_ZAPTO_CHANNELNR:
	{
		CZapitMessages::commandZaptoChannelNr msgZaptoChannelNr;
		CBasicServer::receive_data(connfd, &msgZaptoChannelNr, sizeof(msgZaptoChannelNr)); // bouquet & channel number are already starting at 0!
		zapTo(msgZaptoChannelNr.channel);
		break;
	}

	case CZapitMessages::CMD_ZAPTO_SERVICEID:
	case CZapitMessages::CMD_ZAPTO_SUBSERVICEID:
	{
		CZapitMessages::commandZaptoServiceID msgZaptoServiceID;
		CZapitMessages::responseZapComplete msgResponseZapComplete;
		CBasicServer::receive_data(connfd, &msgZaptoServiceID, sizeof(msgZaptoServiceID));
		msgResponseZapComplete.zapStatus = zapTo_ChannelID(msgZaptoServiceID.channel_id, (rmsg.cmd == CZapitMessages::CMD_ZAPTO_SUBSERVICEID));
		CBasicServer::send_data(connfd, &msgResponseZapComplete, sizeof(msgResponseZapComplete));
		break;
	}

	case CZapitMessages::CMD_ZAPTO_SERVICEID_NOWAIT:
	case CZapitMessages::CMD_ZAPTO_SUBSERVICEID_NOWAIT:
	{
		CZapitMessages::commandZaptoServiceID msgZaptoServiceID;
		CBasicServer::receive_data(connfd, &msgZaptoServiceID, sizeof(msgZaptoServiceID));
		zapTo_ChannelID(msgZaptoServiceID.channel_id, (rmsg.cmd == CZapitMessages::CMD_ZAPTO_SUBSERVICEID_NOWAIT));
		break;
	}

	case CZapitMessages::CMD_GET_LAST_CHANNEL:
	{
		CZapitClient::responseGetLastChannel responseGetLastChannel;
		responseGetLastChannel = load_settings();
		CBasicServer::send_data(connfd, &responseGetLastChannel, sizeof(responseGetLastChannel)); // bouquet & channel number are already starting at 0!
		break;
	}

	case CZapitMessages::CMD_GET_CURRENT_SATELLITE_POSITION:
	{
		int32_t currentSatellitePosition = frontend->getCurrentSatellitePosition();
		CBasicServer::send_data(connfd, &currentSatellitePosition, sizeof(currentSatellitePosition));
		break;
	}

	case CZapitMessages::CMD_SET_AUDIOCHAN:
	{
		CZapitMessages::commandSetAudioChannel msgSetAudioChannel;
		CBasicServer::receive_data(connfd, &msgSetAudioChannel, sizeof(msgSetAudioChannel));
		change_audio_pid(msgSetAudioChannel.channel);
		break;
	}

	case CZapitMessages::CMD_SET_MODE:
	{
		CZapitMessages::commandSetMode msgSetMode;
		CBasicServer::receive_data(connfd, &msgSetMode, sizeof(msgSetMode));
		if (msgSetMode.mode == CZapitClient::MODE_TV)
			setTVMode();
		else if (msgSetMode.mode == CZapitClient::MODE_RADIO)
			setRadioMode();
		break;
	}

	case CZapitMessages::CMD_GET_MODE:
	{
		CZapitMessages::responseGetMode msgGetMode;
		msgGetMode.mode = (CZapitClient::channelsMode) getMode();
		CBasicServer::send_data(connfd, &msgGetMode, sizeof(msgGetMode));
		break;
	}

	case CZapitMessages::CMD_GET_CURRENT_SERVICEID:
	{
		CZapitMessages::responseGetCurrentServiceID msgCurrentSID;
		msgCurrentSID.channel_id = tuned_transponder_id ? channel->getChannelID() : 0;
		CBasicServer::send_data(connfd, &msgCurrentSID, sizeof(msgCurrentSID));
		break;
	}

	case CZapitMessages::CMD_GET_CURRENT_SERVICEINFO:
	{
		CZapitClient::CCurrentServiceInfo msgCurrentServiceInfo;
		msgCurrentServiceInfo.onid = channel->getOriginalNetworkId();
		msgCurrentServiceInfo.sid = channel->getServiceId();
		msgCurrentServiceInfo.tsid = channel->getTransportStreamId();
		msgCurrentServiceInfo.vdid = channel->getVideoPid();
		msgCurrentServiceInfo.apid = channel->getAudioPid();
		msgCurrentServiceInfo.vtxtpid = channel->getTeletextPid();
		msgCurrentServiceInfo.pcrpid = channel->getPcrPid();
		msgCurrentServiceInfo.tsfrequency = frontend->getFrequency();
		if (frontend->getInfo()->type == FE_QPSK)
			msgCurrentServiceInfo.polarisation = frontend->getPolarization();
		else
			msgCurrentServiceInfo.polarisation = 2;
		msgCurrentServiceInfo.diseqc = channel->getDiSEqC();
		CBasicServer::send_data(connfd, &msgCurrentServiceInfo, sizeof(msgCurrentServiceInfo));
		break;
	}

	case CZapitMessages::CMD_GET_DELIVERY_SYSTEM:
	{
		CZapitMessages::responseDeliverySystem response;
		switch (frontend->getInfo()->type) {
		case FE_QAM:
			response.system = DVB_C;
			break;
		case FE_QPSK:
			response.system = DVB_S;
			break;
		case FE_OFDM:
			response.system = DVB_T;
			break;
		}
		CBasicServer::send_data(connfd, &response, sizeof(response));
		break;
	}

	case CZapitMessages::CMD_GET_BOUQUETS:
	{
		CZapitMessages::commandGetBouquets msgGetBouquets;
		CBasicServer::receive_data(connfd, &msgGetBouquets, sizeof(msgGetBouquets));
		sendBouquets(connfd, msgGetBouquets.emptyBouquetsToo); // bouquet & channel number are already starting at 0!
		break;
	}

	case CZapitMessages::CMD_GET_BOUQUET_CHANNELS:
	{
		CZapitMessages::commandGetBouquetChannels msgGetBouquetChannels;
		CBasicServer::receive_data(connfd, &msgGetBouquetChannels, sizeof(msgGetBouquetChannels));
		sendBouquetChannels(connfd, msgGetBouquetChannels.bouquet, msgGetBouquetChannels.mode); // bouquet & channel number are already starting at 0!
		break;
	}

	case CZapitMessages::CMD_GET_CHANNELS:
	{
		CZapitMessages::commandGetChannels msgGetChannels;
		CBasicServer::receive_data(connfd, &msgGetChannels, sizeof(msgGetChannels));
		sendChannels(connfd, msgGetChannels.mode, msgGetChannels.order); // bouquet & channel number are already starting at 0!
		break;
	}

	case CZapitMessages::CMD_GET_CURRENT_TP:
	{
		TP_params TP;
		get_transponder(&TP);
		INFO("current frequency: %lu", (long unsigned int)TP.feparams.frequency);
		CBasicServer::send_data(connfd, &TP, sizeof(TP));
		break;
	}

	case CZapitMessages::CMD_BQ_RESTORE:
	{
		CZapitMessages::responseCmd response;
		bouquetManager->restoreBouquets();
		response.cmd = CZapitMessages::CMD_READY;
		CBasicServer::send_data(connfd, &response, sizeof(response));
		break;
	}

	case CZapitMessages::CMD_REINIT_CHANNELS:
	{
		CZapitMessages::responseCmd response;
		prepare_channels(frontend->getInfo()->type, diseqcType);
		response.cmd = CZapitMessages::CMD_READY;
		CBasicServer::send_data(connfd, &response, sizeof(response));
		eventServer->sendEvent(CZapitClient::EVT_BOUQUETS_CHANGED, CEventServer::INITID_ZAPIT);
		break;
	}

	case CZapitMessages::CMD_BQ_COMMIT_CHANGE:
	{
		CZapitMessages::responseCmd response;
		bouquetManager->renumServices();
		bouquetManager->storeBouquets();
		response.cmd = CZapitMessages::CMD_READY;
		CBasicServer::send_data(connfd, &response, sizeof(response));
		eventServer->sendEvent(CZapitClient::EVT_BOUQUETS_CHANGED, CEventServer::INITID_ZAPIT);
		break;
	}

	case CZapitMessages::CMD_SCANSTART:
		if (start_scan() == -1)
			eventServer->sendEvent(CZapitClient::EVT_SCAN_FAILED, CEventServer::INITID_ZAPIT);
		break;

	case CZapitMessages::CMD_SCAN_TP:
	{
		TP_params TP;
		CBasicServer::receive_data(connfd, &TP, sizeof(TP));
		scan_transponder(&TP);
		break;
	}

	case CZapitMessages::CMD_SCANREADY:
	{
		CZapitMessages::responseIsScanReady msgResponseIsScanReady;
		msgResponseIsScanReady.satellite = curr_sat;
		msgResponseIsScanReady.transponder = found_transponders;
		msgResponseIsScanReady.processed_transponder = processed_transponders;
		msgResponseIsScanReady.services = found_channels;
		if (scan_runs > 0)
			msgResponseIsScanReady.scanReady = false;
		else
			msgResponseIsScanReady.scanReady = true;
		CBasicServer::send_data(connfd, &msgResponseIsScanReady, sizeof(msgResponseIsScanReady));
		break;
	}

	case CZapitMessages::CMD_SCANGETSATLIST:
	{
		if (!scanInputParser)
		{
			parseScanInputXml();
			if (!scanInputParser)
				break;
		}

		uint32_t satlength;
		char * satname;
		xmlNodePtr search = xmlDocGetRootElement(scanInputParser)->xmlChildrenNode;
		char * frontendname = getFrontendName();
		CZapitClient::responseGetSatelliteList sat;

		if (frontendname != NULL)
			while ((search = xmlGetNextOccurence(search, frontendname)) != NULL)
			{
				satname = xmlGetAttribute(search, "name");
				strncpy(sat.satName, satname, 29);
				sat.satPosition = satellitePositions[satname];
				sat.motorPosition = motorPositions[sat.satPosition];
				sat.satDiseqc = satelliteDiseqcs[satname];
				satlength = sizeof(sat);
				//printf("[zapit] sending %s, %d, %d\n", sat.satName, sat.satPosition, sat.motorPosition);
				CBasicServer::send_data(connfd, &satlength, sizeof(satlength));
				CBasicServer::send_data(connfd, (char *)&sat, satlength);
				search = search->xmlNextNode;
			}
		satlength = SATNAMES_END_MARKER;
		CBasicServer::send_data(connfd, &satlength, sizeof(satlength));
		break;
	}

	case CZapitMessages::CMD_SCANSETSCANSATLIST:
	{
		CZapitClient::commandSetScanSatelliteList sat;
		scanProviders.clear();
		while (CBasicServer::receive_data(connfd, &sat, sizeof(sat))) {
			DBG("adding %s (diseqc %d)", sat.satName, sat.diseqc);
			scanProviders[sat.diseqc] = sat.satName;
		}
		break;
	}

	case CZapitMessages::CMD_SCANSETSCANMOTORPOSLIST:
	{
		CZapitClient::commandSetScanMotorPosList pos;
		bool changed = false;
		FILE * fd;

		while (CBasicServer::receive_data(connfd, &pos, sizeof(pos)))
		{
			//printf("adding %d (motorPos %d)\n", pos.satPosition, pos.motorPos);
			changed |= (motorPositions[pos.satPosition] != pos.motorPos);
			motorPositions[pos.satPosition] = pos.motorPos;
		}

		if (changed)
		{
			// save to motor.conf
			//printf("[zapit] saving motor.conf\n");
			fd = fopen(MOTORCONFIGFILE, "w");
			for (mpos_it = motorPositions.begin(); mpos_it != motorPositions.end(); mpos_it++)
			{
				//printf("[zapit] saving %d %d\n", mpos_it->first, mpos_it->second);
				fprintf(fd, "%d %d\n", mpos_it->first, mpos_it->second);
			}
			fclose(fd);
			chmod(MOTORCONFIGFILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		}
		break;
	}

	case CZapitMessages::CMD_SCANSETDISEQCTYPE:
	{
		CBasicServer::receive_data(connfd, &diseqcType, sizeof(diseqcType));
		frontend->setDiseqcType(diseqcType);
		DBG("set diseqc type %d", diseqcType);
		break;
	}

	case CZapitMessages::CMD_SCANSETDISEQCREPEAT:
	{
		uint32_t repeats;
		CBasicServer::receive_data(connfd, &repeats, sizeof(repeats));
		frontend->setDiseqcRepeats(repeats);
		DBG("set diseqc repeats to %d", repeats);
		break;
	}

	case CZapitMessages::CMD_SCANSETBOUQUETMODE:
		CBasicServer::receive_data(connfd, &bouquetMode, sizeof(bouquetMode));
		break;

	case CZapitMessages::CMD_SET_RECORD_MODE:
	{
		CZapitMessages::commandSetRecordMode msgSetRecordMode;
		CBasicServer::receive_data(connfd, &msgSetRecordMode, sizeof(msgSetRecordMode));
		if (msgSetRecordMode.activate)
			setRecordMode();
		else
			unsetRecordMode();
		break;
	}

	case CZapitMessages::CMD_GET_RECORD_MODE:
	{
		CZapitMessages::responseGetRecordModeState msgGetRecordModeState;
		msgGetRecordModeState.activated = (currentMode & RECORD_MODE);
		CBasicServer::send_data(connfd, &msgGetRecordModeState, sizeof(msgGetRecordModeState));
		break;
	}

	case CZapitMessages::CMD_SB_GET_PLAYBACK_ACTIVE:
	{
		CZapitMessages::responseGetPlaybackState msgGetPlaybackState;
		if (videoDecoder->getPlayState() == VIDEO_PLAYING)
			msgGetPlaybackState.activated = 1;
		else
			msgGetPlaybackState.activated = 0;
		CBasicServer::send_data(connfd, &msgGetPlaybackState, sizeof(msgGetPlaybackState));
		break;
	}

	case CZapitMessages::CMD_BQ_ADD_BOUQUET:
	{
		CZapitMessages::commandAddBouquet msgAddBouquet;
		CBasicServer::receive_data(connfd, &msgAddBouquet, sizeof(msgAddBouquet));
		bouquetManager->addBouquet(msgAddBouquet.name);
		break;
	}

	case CZapitMessages::CMD_BQ_DELETE_BOUQUET:
	{
		CZapitMessages::commandDeleteBouquet msgDeleteBouquet;
		CBasicServer::receive_data(connfd, &msgDeleteBouquet, sizeof(msgDeleteBouquet)); // bouquet & channel number are already starting at 0!
		bouquetManager->deleteBouquet(msgDeleteBouquet.bouquet);
		break;
	}

	case CZapitMessages::CMD_BQ_RENAME_BOUQUET:
	{
		CZapitMessages::commandRenameBouquet msgRenameBouquet;
		CBasicServer::receive_data(connfd, &msgRenameBouquet, sizeof(msgRenameBouquet)); // bouquet & channel number are already starting at 0!
		if (msgRenameBouquet.bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[msgRenameBouquet.bouquet]->Name = msgRenameBouquet.name;
		break;
	}

	case CZapitMessages::CMD_BQ_EXISTS_BOUQUET:
	{
		CZapitMessages::commandExistsBouquet msgExistsBouquet;
		CZapitMessages::responseGeneralInteger responseInteger;
		CBasicServer::receive_data(connfd, &msgExistsBouquet, sizeof(msgExistsBouquet));
		responseInteger.number = bouquetManager->existsBouquet(msgExistsBouquet.name);
		CBasicServer::send_data(connfd, &responseInteger, sizeof(responseInteger)); // bouquet & channel number are already starting at 0!
		break;
	}

	case CZapitMessages::CMD_BQ_EXISTS_CHANNEL_IN_BOUQUET:
	{
		CZapitMessages::commandExistsChannelInBouquet msgExistsChInBq;
		CZapitMessages::responseGeneralTrueFalse responseBool;
		CBasicServer::receive_data(connfd, &msgExistsChInBq, sizeof(msgExistsChInBq)); // bouquet & channel number are already starting at 0!
		responseBool.status = bouquetManager->existsChannelInBouquet(msgExistsChInBq.bouquet, msgExistsChInBq.channel_id);
		CBasicServer::send_data(connfd, &responseBool, sizeof(responseBool));
		break;
	}

	case CZapitMessages::CMD_BQ_MOVE_BOUQUET:
	{
		CZapitMessages::commandMoveBouquet msgMoveBouquet;
		CBasicServer::receive_data(connfd, &msgMoveBouquet, sizeof(msgMoveBouquet)); // bouquet & channel number are already starting at 0!
		bouquetManager->moveBouquet(msgMoveBouquet.bouquet, msgMoveBouquet.newPos);
		break;
	}

	case CZapitMessages::CMD_BQ_ADD_CHANNEL_TO_BOUQUET:
	{
		CZapitMessages::commandAddChannelToBouquet msgAddChannelToBouquet;
		CBasicServer::receive_data(connfd, &msgAddChannelToBouquet, sizeof(msgAddChannelToBouquet)); // bouquet & channel number are already starting at 0!
		addChannelToBouquet(msgAddChannelToBouquet.bouquet, msgAddChannelToBouquet.channel_id);
		break;
	}

	case CZapitMessages::CMD_BQ_REMOVE_CHANNEL_FROM_BOUQUET:
	{
		CZapitMessages::commandRemoveChannelFromBouquet msgRemoveChannelFromBouquet;
		CBasicServer::receive_data(connfd, &msgRemoveChannelFromBouquet, sizeof(msgRemoveChannelFromBouquet)); // bouquet & channel number are already starting at 0!
		if (msgRemoveChannelFromBouquet.bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[msgRemoveChannelFromBouquet.bouquet]->removeService(msgRemoveChannelFromBouquet.channel_id);
		break;
	}

	case CZapitMessages::CMD_BQ_MOVE_CHANNEL:
	{
		CZapitMessages::commandMoveChannel msgMoveChannel;
		CBasicServer::receive_data(connfd, &msgMoveChannel, sizeof(msgMoveChannel)); // bouquet & channel number are already starting at 0!
		if (msgMoveChannel.bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[msgMoveChannel.bouquet]->moveService(msgMoveChannel.oldPos, msgMoveChannel.newPos,
					(((currentMode & RADIO_MODE) && msgMoveChannel.mode == CZapitClient::MODE_CURRENT)
					 || (msgMoveChannel.mode==CZapitClient::MODE_RADIO)) ? 2 : 1);
		break;
	}

	case CZapitMessages::CMD_BQ_SET_LOCKSTATE:
	{
		CZapitMessages::commandBouquetState msgBouquetLockState;
		CBasicServer::receive_data(connfd, &msgBouquetLockState, sizeof(msgBouquetLockState)); // bouquet & channel number are already starting at 0!
		if (msgBouquetLockState.bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[msgBouquetLockState.bouquet]->bLocked = msgBouquetLockState.state;
		break;
	}

	case CZapitMessages::CMD_BQ_SET_HIDDENSTATE:
	{
		CZapitMessages::commandBouquetState msgBouquetHiddenState;
		CBasicServer::receive_data(connfd, &msgBouquetHiddenState, sizeof(msgBouquetHiddenState)); // bouquet & channel number are already starting at 0!
		if (msgBouquetHiddenState.bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[msgBouquetHiddenState.bouquet]->bHidden = msgBouquetHiddenState.state;
		break;
	}

	case CZapitMessages::CMD_BQ_RENUM_CHANNELLIST:
		bouquetManager->renumServices();
		bouquetManager->storeBouquets();
		break;

	case CZapitMessages::CMD_BQ_SAVE_BOUQUETS:
	{
		CZapitMessages::responseCmd response;
		bouquetManager->saveBouquets();
		response.cmd = CZapitMessages::CMD_READY;
		CBasicServer::send_data(connfd, &response, sizeof(response));
		break;
	}

	case CZapitMessages::CMD_SET_PAL:
	{
		setVideoSystem_t(0);
		break;
	}
	
	case CZapitMessages::CMD_SET_NTSC:
	{
		setVideoSystem_t(1);
		break;
	}

	case CZapitMessages::CMD_SB_START_PLAYBACK:
		playbackStopForced = false;
		/* FIXME: nvod */
		startPlayBack(channel);
		break;

	case CZapitMessages::CMD_SB_STOP_PLAYBACK:
		stopPlayBack();
		playbackStopForced = true;
		break;

	case CZapitMessages::CMD_SET_DISPLAY_FORMAT:
	{
		CZapitMessages::commandInt msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		videoDecoder->setCroppingMode((video_displayformat_t) msg.val);
		break;
	}

	case CZapitMessages::CMD_SET_AUDIO_MODE:
	{
		CZapitMessages::commandInt msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		audioDecoder->setChannel((audio_channel_select_t) msg.val);
		break;
	}

	case CZapitMessages::CMD_GETPIDS:
	{
		if (channel)
		{
			CZapitClient::responseGetOtherPIDs responseGetOtherPIDs;
			responseGetOtherPIDs.vpid = channel->getVideoPid();
			responseGetOtherPIDs.ecmpid = NONE; // TODO: remove
			responseGetOtherPIDs.vtxtpid = channel->getTeletextPid();
			responseGetOtherPIDs.pcrpid = channel->getPcrPid();
			responseGetOtherPIDs.selected_apid = channel->getAudioChannelIndex();
			CBasicServer::send_data(connfd, &responseGetOtherPIDs, sizeof(responseGetOtherPIDs));
			sendAPIDs(connfd);
		}
		break;
	}

	case CZapitMessages::CMD_SETSUBSERVICES:
	{
		CZapitClient::commandAddSubServices msgAddSubService;

		while (CBasicServer::receive_data(connfd, &msgAddSubService, sizeof(msgAddSubService)))
		{
			t_original_network_id original_network_id = msgAddSubService.original_network_id;
			t_service_id          service_id          = msgAddSubService.service_id;
			nvodchannels.insert
			(
				std::pair <t_channel_id, CZapitChannel>
				(
					CREATE_CHANNEL_ID_FROM_SERVICE_ORIGINALNETWORK_TRANSPORTSTREAM_ID(msgAddSubService.service_id, msgAddSubService.original_network_id, msgAddSubService.transport_stream_id),
					CZapitChannel
					(
					    "NVOD",
					    service_id,
					    msgAddSubService.transport_stream_id,
					    original_network_id,
					    1,
					    channel->getDiSEqC(),
					    channel->getSatellitePosition()
					)
				)
			);
		}

		current_is_nvod = true;
		break;
	}

	case CZapitMessages::CMD_REGISTEREVENTS:
		eventServer->registerEvent(connfd);
		break;

	case CZapitMessages::CMD_UNREGISTEREVENTS:
		eventServer->unRegisterEvent(connfd);
		break;

	case CZapitMessages::CMD_MUTE:
	{
		CZapitMessages::commandBoolean msgBoolean;
		CBasicServer::receive_data(connfd, &msgBoolean, sizeof(msgBoolean));
		if (msgBoolean.truefalse)
			audioDecoder->mute();
		else
			audioDecoder->unmute();
		break;
	}

	case CZapitMessages::CMD_SET_VOLUME:
	{
		CZapitMessages::commandVolume msgVolume;
		CBasicServer::receive_data(connfd, &msgVolume, sizeof(msgVolume));
		audioDecoder->setVolume(msgVolume.left, msgVolume.right);
		break;
	}

	case CZapitMessages::CMD_SET_STANDBY:
	{
		CZapitMessages::commandBoolean msgBoolean;
		CBasicServer::receive_data(connfd, &msgBoolean, sizeof(msgBoolean));
		if (msgBoolean.truefalse)
			enterStandby();
		else
			leaveStandby();
		break;
	}

	case CZapitMessages::CMD_NVOD_SUBSERVICE_NUM:
	{
		CZapitMessages::commandInt msg;
		CBasicServer::receive_data(connfd, &msg, sizeof(msg));
		select_nvod_subservice_num(msg.val);
		break;
	}

	case CZapitMessages::CMD_SEND_MOTOR_COMMAND:
	{
		CZapitMessages::commandMotor msgMotor;
		CBasicServer::receive_data(connfd, &msgMotor, sizeof(msgMotor));
		printf("[zapit] received motor command: %x %x %x %x %x %x\n", msgMotor.cmdtype, msgMotor.address, msgMotor.cmd, msgMotor.num_parameters, msgMotor.param1, msgMotor.param2);
		frontend->sendMotorCommand(msgMotor.cmdtype, msgMotor.address, msgMotor.cmd, msgMotor.num_parameters, msgMotor.param1, msgMotor.param2);
		break;
	}

	case CZapitMessages::CMD_GET_CHANNEL_NAME:
	{
		t_channel_id                           requested_channel_id;
		CZapitMessages::responseGetChannelName response;
		CBasicServer::receive_data(connfd, &requested_channel_id, sizeof(requested_channel_id));
		tallchans_iterator it = allchans.find(requested_channel_id);
		if (it == allchans.end())
			response.name[0] = 0;
		else
			strncpy(response.name, it->second.getName().c_str(), 30);

		CBasicServer::send_data(connfd, &response, sizeof(response));
		break;
	}

	case CZapitMessages::CMD_IS_TV_CHANNEL:
	{
		t_channel_id                             requested_channel_id;
		CZapitMessages::responseGeneralTrueFalse response;
		CBasicServer::receive_data(connfd, &requested_channel_id, sizeof(requested_channel_id));
		tallchans_iterator it = allchans.find(requested_channel_id);
		if (it == allchans.end())
			/* if in doubt (i.e. unknown channel) answer no */
			response.status = false;
		else
			/* FIXME: the following check is no even remotely accurate */
			response.status = (it->second.getServiceType() != ST_DIGITAL_RADIO_SOUND_SERVICE);

		CBasicServer::send_data(connfd, &response, sizeof(response));
		break;
	}

	default:
		WARN("unknown command %d (version %d)", rmsg.cmd, CZapitMessages::ACTVERSION);
		break;
	}

	DBG("cmd %d processed", rmsg.cmd);

	return true;
}

/****************************************************************/
/*								*/
/*  functions for new command handling via CZapitClient		*/
/*								*/
/*  these functions should be encapsulated in a class CZapit	*/
/*								*/
/****************************************************************/

void addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	DBG("addChannelToBouquet(%d, " PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS ")", bouquet, channel_id);

	CZapitChannel* chan = bouquetManager->findChannelByChannelID(channel_id);

	if (chan != NULL)
		if (bouquet < bouquetManager->Bouquets.size())
			bouquetManager->Bouquets[bouquet]->addService(chan);
		else
			WARN("bouquet not found");
	else
		WARN("channel_id not found in channellist");
}

void sendBouquets(int connfd, const bool emptyBouquetsToo)
{
	CZapitClient::responseGetBouquets msgBouquet;

	for (uint i = 0; i < bouquetManager->Bouquets.size(); i++)
	{
		if (emptyBouquetsToo ||
		    ((!bouquetManager->Bouquets[i]->bHidden) &&
		     (((currentMode & RADIO_MODE) && !bouquetManager->Bouquets[i]->radioChannels.empty()) ||
		      ((currentMode & TV_MODE) && !bouquetManager->Bouquets[i]->tvChannels.empty()))))
		{
// ATTENTION: in RECORD_MODE empty bouquets are not send!
			if ((!(currentMode & RECORD_MODE)) ||
			    ((channel != NULL) &&
			     (((currentMode & RADIO_MODE) && (bouquetManager->Bouquets[i]->recModeRadioSize(channel->getTsidOnid()) > 0)) ||
			      ((currentMode & TV_MODE)    && (bouquetManager->Bouquets[i]->recModeTVSize   (channel->getTsidOnid()) > 0)))))
			{
				msgBouquet.bouquet_nr = i;
				strncpy(msgBouquet.name, bouquetManager->Bouquets[i]->Name.c_str(), 30);
				msgBouquet.locked     = bouquetManager->Bouquets[i]->bLocked;
				msgBouquet.hidden     = bouquetManager->Bouquets[i]->bHidden;
				if (CBasicServer::send_data(connfd, &msgBouquet, sizeof(msgBouquet)) == false)
				{
					ERROR("could not send any return");
					return;
				}
			}
		}
	}
	msgBouquet.bouquet_nr = RESPONSE_GET_BOUQUETS_END_MARKER;
	if (CBasicServer::send_data(connfd, &msgBouquet, sizeof(msgBouquet)) == false)
	{
		ERROR("could not send end marker");
		return;
	}
}

bool send_data_count(int connfd, int data_count)
{
	CZapitMessages::responseGeneralInteger responseInteger;
	responseInteger.number = data_count;
	if (CBasicServer::send_data(connfd, &responseInteger, sizeof(responseInteger)) == false)
	{
		ERROR("could not send any return");
		return false;
	}
	return true;
}

void internalSendChannels(int connfd, ChannelList* channels, const unsigned int first_channel_nr)
{
	if (!send_data_count(connfd, channels->size()))
		return;

	for (uint32_t i = 0; i < channels->size();i++)
	{
		if ((currentMode & RECORD_MODE) && ((*channels)[i]->getTsidOnid() != channel->getTsidOnid()))
			continue;

		CZapitClient::responseGetBouquetChannels response;
		strncpy(response.name, ((*channels)[i]->getName()).c_str(), 30);
		response.satellitePosition = (*channels)[i]->getSatellitePosition();
		response.channel_id = (*channels)[i]->getChannelID();
		response.nr = first_channel_nr + i;
		response.service_type = (*channels)[i]->getServiceType();

		if (CBasicServer::send_data(connfd, &response, sizeof(response)) == false)
		{
			ERROR("could not send any return");
			return;
		}
	}
}

void sendAPIDs(int connfd)
{
	if (!send_data_count(connfd, channel->getAudioChannelCount()))
		return;

	for (uint32_t i = 0; i < channel->getAudioChannelCount(); i++)
	{
		CZapitClient::responseGetAPIDs response;
		response.pid = channel->getAudioPid(i);
		strncpy(response.desc, channel->getAudioChannel(i)->description.c_str(), 25);
		response.is_ac3 = channel->getAudioChannel(i)->isAc3;
		response.component_tag = channel->getAudioChannel(i)->componentTag;

		if (CBasicServer::send_data(connfd, &response, sizeof(response)) == false)
		{
			ERROR("could not send any return");
			return;
		}
	}
}


void sendBouquetChannels(int connfd, const unsigned int bouquet, const CZapitClient::channelsMode mode)
{
	if (bouquet >= bouquetManager->Bouquets.size())
	{
		WARN("invalid bouquet number: %d", bouquet);
		return;
	}

	if (((currentMode & RADIO_MODE) && (mode == CZapitClient::MODE_CURRENT)) || (mode == CZapitClient::MODE_RADIO))
		internalSendChannels(connfd, &(bouquetManager->Bouquets[bouquet]->radioChannels), bouquetManager->radioChannelsBegin().getNrofFirstChannelofBouquet(bouquet));
	else
		internalSendChannels(connfd, &(bouquetManager->Bouquets[bouquet]->tvChannels), bouquetManager->tvChannelsBegin().getNrofFirstChannelofBouquet(bouquet));
}

void sendChannels(int connfd, const CZapitClient::channelsMode mode, const CZapitClient::channelsOrder order)
{
	ChannelList channels;

	if (order == CZapitClient::SORT_BOUQUET)
	{
		CBouquetManager::ChannelIterator cit = (((currentMode & RADIO_MODE) && (mode == CZapitClient::MODE_CURRENT)) || (mode==CZapitClient::MODE_RADIO)) ? bouquetManager->radioChannelsBegin() : bouquetManager->tvChannelsBegin();
		for (; !(cit.EndOfChannels()); cit++)
			channels.push_back(*cit);
	}
	else if (order == CZapitClient::SORT_ALPHA)   // ATTENTION: in this case response.nr does not return the actual number of the channel for zapping!
	{
		if (((currentMode & RADIO_MODE) && (mode == CZapitClient::MODE_CURRENT)) || (mode==CZapitClient::MODE_RADIO))
		{
			for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE)
					channels.push_back(&(it->second));
		}
		else
		{
			for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				if (it->second.getServiceType() != ST_DIGITAL_RADIO_SOUND_SERVICE)
					channels.push_back(&(it->second));
		}
		sort(channels.begin(), channels.end(), CmpChannelByChName());
	}

	internalSendChannels(connfd, &channels, 0);
}

int startPlayBack(CZapitChannel *thisChannel)
{
	bool have_pcr = false;
	bool have_audio = false;
	bool have_video = false;
	bool have_teletext = false;

	if ((playbackStopForced == true) || (!thisChannel))
		return -1;

	if (thisChannel->getPcrPid() != 0)
		have_pcr = true;
	if (thisChannel->getAudioPid() != 0)
		have_audio = true;
	if ((thisChannel->getVideoPid() != 0) && (currentMode & TV_MODE))
		have_video = true;
	if (thisChannel->getTeletextPid() != 0)
		have_teletext = true;

	if ((!have_audio) && (!have_video) && (!have_teletext))
		return -1;

	/* set demux filters */
	if (have_pcr) {
		if (!pcrDemux)
			pcrDemux = new CDemux();
		if (pcrDemux->pesFilter(thisChannel->getPcrPid(), DMX_OUT_DECODER, DMX_PES_PCR) < 0)
			return -1;
		if (pcrDemux->start() < 0)
			return -1;
	}
	if (have_audio) {
		if (!audioDemux)
			audioDemux = new CDemux();
		if (audioDemux->pesFilter(thisChannel->getAudioPid(), DMX_OUT_DECODER, DMX_PES_AUDIO) < 0)
			return -1;
		if (audioDemux->start() < 0)
			return -1;
	}
	if (have_video) {
		if (!videoDemux)
			videoDemux = new CDemux();
		if (videoDemux->pesFilter(thisChannel->getVideoPid(), DMX_OUT_DECODER, DMX_PES_VIDEO) < 0)
			return -1;
		if (videoDemux->start() < 0)
			return -1;
	}
	if (have_teletext) {
		if (!teletextDemux)
			teletextDemux = new CDemux();
		if (teletextDemux->pesFilter(thisChannel->getTeletextPid(), DMX_OUT_DECODER, DMX_PES_TELETEXT) < 0)
			return -1;
		if (teletextDemux->start() < 0)
			return -1;
	}

	/* start video */
	if (have_video) {
		videoDecoder->setSource(VIDEO_SOURCE_DEMUX);
		videoDecoder->start();
	}

	/* select audio output and start audio */
	if (have_audio) {
		if (thisChannel->getAudioChannel()->isAc3)
			audioDecoder->enableBypass();
		else
			audioDecoder->disableBypass();

		audioDecoder->setSource(AUDIO_SOURCE_DEMUX);
		audioDecoder->start();
	}

	return 0;
}

int stopPlayBack(void)
{
	if (playbackStopForced)
		return -1;

	if (teletextDemux)
		teletextDemux->stop();
	if (videoDemux)
		videoDemux->stop();
	if (audioDemux)
		audioDemux->stop();
	if (pcrDemux)
		pcrDemux->stop();
	if (audioDecoder)
		audioDecoder->stop();
	if (videoDecoder)
		videoDecoder->stop();

	return 0;
}

void setVideoSystem_t(int video_system)
{
	if (video_system == 0) 
		videoDecoder->setVideoSystem(PAL);
	else
		videoDecoder->setVideoSystem(NTSC);
}

void enterStandby(void)
{
	if (standby) {
		sleep(1);
		return;
	}

	standby = true;

	saveSettings(true);
	stopPlayBack();

	if (audioDemux) {
		delete audioDemux;
		audioDemux = NULL;
	}
	if (pcrDemux) {
		delete pcrDemux;
		pcrDemux = NULL;
	}
	if (teletextDemux) {
		delete teletextDemux;
		teletextDemux = NULL;
	}
	if (videoDemux) {
		delete videoDemux;
		videoDemux = NULL;
	}
	if (audioDecoder) {
		delete audioDecoder;
		audioDecoder = NULL;
	}
	if (cam) {
		delete cam;
		cam = NULL;
	}
	if (frontend) {
		delete frontend;
		frontend = NULL;
	}
	if (videoDecoder) {
		delete videoDecoder;
		videoDecoder = NULL;
	}

	tuned_transponder_id = 0;
}

void leaveStandby(void)
{
	if (!audioDecoder) {
		audioDecoder = new CAudio();
		audioDecoder->unmute();
	}
	if (!cam) {
		cam = new CCam();
	}
	if (!frontend) {
		frontend = new CFrontend();
	}
	if (!videoDecoder) {
		videoDecoder = new CVideo();
	}

	frontend->setCurrentSatellitePosition(config.getInt32("lastSatellitePosition", 192));
	frontend->setDiseqcRepeats(config.getInt32("diseqcRepeats", 0));
	motorRotationSpeed = config.getInt32("motorRotationSpeed", 18); // default: 1.8 degrees per second
	diseqcType = (diseqc_t)config.getInt32("diseqcType", NO_DISEQC);
	frontend->setDiseqcType(diseqcType);

	for (unsigned int i = 0; i < MAX_LNBS; i++) {
		char tmp[16];
		sprintf(tmp, "lnb%d_OffsetLow", i);
		frontend->setLnbOffset(false, i, config.getInt32(tmp, 9750000));
		sprintf(tmp, "lnb%d_OffsetHigh", i);
		frontend->setLnbOffset(true, i, config.getInt32(tmp, 10600000));
	}

	if (channel)
		zapit(channel->getChannelID(), current_is_nvod, 0);

	standby = false;
}

unsigned zapTo(const unsigned int bouquet, const unsigned int channel)
{
	if (bouquet >= bouquetManager->Bouquets.size()) {
		WARN("Invalid bouquet %d", bouquet);
		return CZapitClient::ZAP_INVALID_PARAM;
	}

	ChannelList *channels;

	if (currentMode & RADIO_MODE)
		channels = &(bouquetManager->Bouquets[bouquet]->radioChannels);
	else
		channels = &(bouquetManager->Bouquets[bouquet]->tvChannels);

	if (channel >= channels->size()) {
		WARN("Invalid channel %d in bouquet %d", channel, bouquet);
		return CZapitClient::ZAP_INVALID_PARAM;
	}

	return zapTo_ChannelID((*channels)[channel]->getChannelID(), false);
}

unsigned int zapTo_ChannelID(t_channel_id channel_id, bool isSubService)
{
	unsigned int result = 0;

	if (zapit(channel_id, isSubService, 0) < 0)
	{
		eventServer->sendEvent((isSubService ? CZapitClient::EVT_ZAP_SUB_FAILED : CZapitClient::EVT_ZAP_FAILED), CEventServer::INITID_ZAPIT, &channel_id, sizeof(channel_id));
		return result;
	}

	result |= CZapitClient::ZAP_OK;

	if (isSubService)
	{
		eventServer->sendEvent(CZapitClient::EVT_ZAP_SUB_COMPLETE, CEventServer::INITID_ZAPIT, &channel_id, sizeof(channel_id));
	}
	else if (current_is_nvod)
	{
		eventServer->sendEvent(CZapitClient::EVT_ZAP_COMPLETE_IS_NVOD, CEventServer::INITID_ZAPIT, &channel_id, sizeof(channel_id));
		result |= CZapitClient::ZAP_IS_NVOD;
	}
	else
		eventServer->sendEvent(CZapitClient::EVT_ZAP_COMPLETE, CEventServer::INITID_ZAPIT, &channel_id, sizeof(channel_id));

	return result;
}

unsigned zapTo(const unsigned int channel)
{
	CBouquetManager::ChannelIterator cit = ((currentMode & RADIO_MODE) ? bouquetManager->radioChannelsBegin() : bouquetManager->tvChannelsBegin()).FindChannelNr(channel);
	if (!(cit.EndOfChannels()))
		return zapTo_ChannelID((*cit)->getChannelID(), false);
	else
		return 0;
}

void signal_handler(int signum)
{
	switch (signum) {
	case SIGUSR1:
		debug = !debug;
		break;
	default:
                CZapitClient zapit;
                zapit.shutdown();
                break;
	}
}

int main(int argc, char **argv)
{
	fprintf(stdout, "$Id: zapit.cpp,v 1.349 2004/04/07 19:33:21 thegoodguy Exp $\n");

	for (int i = 1; i < argc ; i++) {
		if (!strcmp(argv[i], "-d")) {
			debug = true;
		}
		else if (!strcmp(argv[i], "-q")) {
			/* don't say anything */
			int fd;

			close(STDOUT_FILENO);
			if ((fd = open("/dev/null", O_WRONLY)) != STDOUT_FILENO)
				close(fd);

			close(STDERR_FILENO);
			if ((fd = open("/dev/null", O_WRONLY)) != STDERR_FILENO)
				close(fd);
		}
		else if (!strcmp(argv[i], "-u")) {
			update_pmt=true;
			printf("[zapit] PMT update enabled\n");
		}
		else {
			fprintf(stderr,
				"Usage: %s [-d] [-q]\n"
				"-d : debug mode\n"
				"-q : quiet mode\n"
				"-u : enable update on PMT change\n"
				"\n"
				"Keys in config file "	CONFIGFILE ":\n"
				"saveLastChannel" ", "
				"lastChannelMode" ", "
				"lastChannelRadio" ", "
				"lastChannelTV" ", "
				"lastSatellitePosition" ", "
			        "diseqcRepeats" ", "
				"diseqcType" ", "
				"motorRotationSpeed" ", "
				"lnb0_OffsetLow" ", ..., " "lnb63_OffsetLow" ", "
				"lnb0_OffsetHigh" ", ..., " "lnb63_OffsetHigh" "."
				"\n", argv[0]);
			return EXIT_FAILURE;
		}
	}

	scan_runs = 0;
	found_channels = 0;
	curr_sat = -1;

	/* load configuration or set defaults if no configuration file exists */
	if (!config.loadConfig(CONFIGFILE))
		WARN("%s not found", CONFIGFILE);

	/* create bouquet manager */
	bouquetManager = new CBouquetManager();

	lastChannelRadio = config.getInt32("lastChannelRadio", 0);
	lastChannelTV    = config.getInt32("lastChannelTV", 0);

	if (config.getInt32("lastChannelMode", 0))
		setRadioMode();
	else
		setTVMode();

	if (!frontend)
		frontend = new CFrontend();

	diseqcType = (diseqc_t)config.getInt32("diseqcType", NO_DISEQC);
	if (prepare_channels(frontend->getInfo()->type, diseqcType) < 0)
		WARN("error parsing services");
	else
		INFO("channels have been loaded succesfully");

	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGUSR1, signal_handler);

	CBasicServer zapit_server;

	if (!zapit_server.prepare(ZAPIT_UDS_NAME))
		return -1;

	if (!debug)
		switch (fork()) {
		case -1: /* can't fork */
			ERROR("fork");
			return -1;
		case 0: /* child, process becomes a daemon */
			if (setsid() == -1) {
				ERROR("setsid");
				return -1;
			}
			break;
		default: /* parent returns to calling process */
			return 0;
		}

	// create eventServer
	eventServer = new CEventServer;

	leaveStandby();

	if (update_pmt) {
		while (zapit_server.run(parse_command, CZapitMessages::ACTVERSION, true)) {
			unsigned int i, pollnum = 0;
			struct pollfd pfd[1];

			if (pmt_update_fd != -1) {
				pfd[pollnum].fd = pmt_update_fd;
				pfd[pollnum].events = (POLLIN | POLLPRI);
				pollnum++;
			}

			if (pollnum) {
				if (poll(pfd, pollnum, 0) > 0) {
					for (i = 0; i < pollnum; i++) {
						if (pfd[i].fd == pmt_update_fd) {
							zapit(channel->getChannelID(), current_is_nvod, 0);
						}
					}
				}
			}
			/* yuck, don't waste that much cpu time :) */
			usleep(0);
		}
	}
	else {
		zapit_server.run(parse_command, CZapitMessages::ACTVERSION);
	}
	
	enterStandby();

	if (scanInputParser)
		xmlFreeDoc(scanInputParser);

	delete bouquetManager;
	delete eventServer;

	INFO("shutdown complete");

	return 0;
}
