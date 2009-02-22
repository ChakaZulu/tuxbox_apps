/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/lib/zapitclient.cpp,v 1.122 2009/02/22 18:45:37 seife Exp $ *
 *
 * Zapit client interface - DBoxII-Project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de> & the DBoxII-Project
 *
 * (C) 2007, 2008, 2009 Stefan Seyfried
 *
 * License: GPL
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

#include <cstdio>
#include <cstring>

/* libevent */
#include <eventserver.h>


#include <zapit/client/zapitclient.h>
#include <zapit/client/msgtypes.h>
#include <zapit/client/zapittools.h>

const unsigned char   CZapitClient::getVersion   () const
{
	return CZapitMessages::ACTVERSION;
}

const          char * CZapitClient::getSocketName() const
{
	return ZAPIT_UDS_NAME;
}

void CZapitClient::shutdown()
{
	send(CZapitMessages::CMD_SHUTDOWN);
	close_connection();
}

//***********************************************/
/*					     */
/* general functions for zapping	       */
/*					     */
/***********************************************/

/* zaps to channel of specified bouquet */
/* bouquets are numbered starting at 0 */
void CZapitClient::zapTo(const unsigned int bouquet, const unsigned int channel)
{
	CZapitMessages::commandZapto msg;

	msg.bouquet = bouquet;
	msg.channel = channel - 1;

	send(CZapitMessages::CMD_ZAPTO, (char*)&msg, sizeof(msg));

	close_connection();
}

/* zaps to channel by nr */
void CZapitClient::zapTo(const unsigned int channel)
{
	CZapitMessages::commandZaptoChannelNr msg;

	msg.channel = channel - 1;

	send(CZapitMessages::CMD_ZAPTO_CHANNELNR, (const char *) & msg, sizeof(msg));

	close_connection();
}

t_channel_id CZapitClient::getCurrentServiceID()
{
	send(CZapitMessages::CMD_GET_CURRENT_SERVICEID);

	CZapitMessages::responseGetCurrentServiceID response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();

	return response.channel_id;
}

CZapitClient::CCurrentServiceInfo CZapitClient::getCurrentServiceInfo()
{
	send(CZapitMessages::CMD_GET_CURRENT_SERVICEINFO);

	CZapitClient::CCurrentServiceInfo response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response;
}

void CZapitClient::getLastChannel(unsigned int &channumber, char &mode)
{
	send(CZapitMessages::CMD_GET_LAST_CHANNEL);

	CZapitClient::responseGetLastChannel response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	channumber = response.channelNumber + 1;
	mode = response.mode;

	close_connection();
}

int32_t CZapitClient::getCurrentSatellitePosition(void)
{
	send(CZapitMessages::CMD_GET_CURRENT_SATELLITE_POSITION);

	int32_t response;
	CBasicClient::receive_data((char *)&response, sizeof(response));

	close_connection();
	return response;
}

void CZapitClient::setAudioChannel(const unsigned int channel)
{
	CZapitMessages::commandSetAudioChannel msg;

	msg.channel = channel;

	send(CZapitMessages::CMD_SET_AUDIOCHAN, (const char *) & msg, sizeof(msg));

	close_connection();
}

unsigned int CZapitClient::ReZap(void)
{
	send(CZapitMessages::CMD_REZAP);

	CZapitMessages::responseZapComplete response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();

	return response.zapStatus;
}

/* zaps to onid_sid, returns the "zap-status" */
unsigned int CZapitClient::zapTo_serviceID(const t_channel_id channel_id, bool nowait)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;
	msg.nowait = nowait;

	send(CZapitMessages::CMD_ZAPTO_SERVICEID, (const char *) & msg, sizeof(msg));

	CZapitMessages::responseZapComplete response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();

	return response.zapStatus;
}

unsigned int CZapitClient::zapTo_subServiceID(const t_channel_id channel_id, bool nowait)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;
	msg.nowait = nowait;

	send(CZapitMessages::CMD_ZAPTO_SUBSERVICEID, (const char *) & msg, sizeof(msg));

	CZapitMessages::responseZapComplete response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();

	return response.zapStatus;
}

/* zaps to channel, does NOT wait for completion (uses event) */
void CZapitClient::zapTo_serviceID_NOWAIT(const t_channel_id channel_id)
{
	(void)zapTo_serviceID(channel_id, true);
}

/* zaps to subservice, does NOT wait for completion (uses event) */
void CZapitClient::zapTo_subServiceID_NOWAIT(const t_channel_id channel_id)
{
	(void)zapTo_subServiceID(channel_id, true);
}


void CZapitClient::setMode(const channelsMode mode)
{
	CZapitMessages::commandSetMode msg;

	msg.mode = mode;

	send(CZapitMessages::CMD_SET_MODE, (const char *) & msg, sizeof(msg));

	close_connection();
}

int CZapitClient::getMode()
{
	send(CZapitMessages::CMD_GET_MODE);

	CZapitMessages::responseGetMode response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response.mode;
}

void CZapitClient::setSubServices( subServiceList& subServices )
{
	int i;
	CZapitMessages::commandInt numServices;

	send(CZapitMessages::CMD_SETSUBSERVICES);
	numServices.val = subServices.size();
	send_data((char* )&numServices, sizeof(numServices));
	for (i = 0; i < numServices.val; i++) {
		send_data((char*)&subServices[i], sizeof(subServices[i]));
	}
	close_connection();
}

void CZapitClient::getPIDS(responseGetPIDs& pids)
{
	CZapitMessages::responseGeneralInteger responseInteger;
	responseGetAPIDs                       responseAPID;
	responseGetSubPIDs                     responseSubPID;

	send(CZapitMessages::CMD_GETPIDS);

	CBasicClient::receive_data((char* )&(pids.PIDs), sizeof(pids.PIDs));

	pids.APIDs.clear();

	if (CBasicClient::receive_data((char* )&responseInteger, sizeof(responseInteger)))
	{
		pids.APIDs.reserve(responseInteger.number);

		while (responseInteger.number-- > 0)
		{
			CBasicClient::receive_data((char*)&responseAPID, sizeof(responseAPID));
			pids.APIDs.push_back(responseAPID);
		};
	}

	pids.SubPIDs.clear();
	if (CBasicClient::receive_data((char* )&responseInteger, sizeof(responseInteger)))
	{
		pids.SubPIDs.reserve(responseInteger.number);

		while (responseInteger.number-- > 0)
		{
			CBasicClient::receive_data((char*)&responseSubPID, sizeof(responseSubPID));
			pids.SubPIDs.push_back(responseSubPID);
		};
	}

	close_connection();
}

void CZapitClient::zaptoNvodSubService(const int num)
{
	CZapitMessages::commandInt msg;

	msg.val = num;

	send(CZapitMessages::CMD_NVOD_SUBSERVICE_NUM, (const char *) & msg, sizeof(msg));

	close_connection();
}

/* gets all bouquets */
/* bouquets are numbered starting at 0 */
void CZapitClient::getBouquets(BouquetList& bouquets, const bool emptyBouquetsToo, const bool utf_encoded, const channelsMode mode)
{
	CZapitMessages::commandGetBouquets msg;

	msg.emptyBouquetsToo = emptyBouquetsToo;
	msg.mode = mode;

	send(CZapitMessages::CMD_GET_BOUQUETS, (char*)&msg, sizeof(msg));

	responseGetBouquets response;
	while (CBasicClient::receive_data((char*)&response, sizeof(responseGetBouquets)))
	{
		if (response.bouquet_nr == RESPONSE_GET_BOUQUETS_END_MARKER)
			break;

		if (!utf_encoded)
		{
			strncpy(response.name, ZapitTools::UTF8_to_Latin1(response.name).c_str(), 30);
			response.name[29] = '\0';
		}
		bouquets.push_back(response);
	}

	close_connection();
}


bool CZapitClient::receive_channel_list(BouquetChannelList& channels, const bool utf_encoded)
{
	CZapitMessages::responseGeneralInteger responseInteger;
	responseGetBouquetChannels             response;

	channels.clear();

	if (CBasicClient::receive_data((char* )&responseInteger, sizeof(responseInteger)))
	{
		channels.reserve(responseInteger.number);

		while (responseInteger.number-- > 0)
		{
			if (!CBasicClient::receive_data((char*)&response, sizeof(responseGetBouquetChannels)))
				return false;

			response.nr++;
			if (!utf_encoded)
			{
				strncpy(response.name, ZapitTools::UTF8_to_Latin1(response.name).c_str(), 30);
				response.name[29] = '\0';
			}
			channels.push_back(response);
		}
	}
	return true;
}


/* gets all channels that are in specified bouquet */
/* bouquets are numbered starting at 0 */
bool CZapitClient::getBouquetChannels(const unsigned int bouquet, BouquetChannelList& channels, channelsMode mode, const bool utf_encoded)
{
	bool                                      return_value;
	CZapitMessages::commandGetBouquetChannels msg;

	msg.bouquet = bouquet;
	msg.mode = mode;

	return_value = (send(CZapitMessages::CMD_GET_BOUQUET_CHANNELS, (char*)&msg, sizeof(msg))) ? receive_channel_list(channels, utf_encoded) : false;

	close_connection();
	return return_value;
}

/* gets all channels */
bool CZapitClient::getChannels( BouquetChannelList& channels, channelsMode mode, channelsOrder order, const bool utf_encoded)
{
	bool                               return_value;
	CZapitMessages::commandGetChannels msg;

	msg.mode = mode;
	msg.order = order;

	return_value = (send(CZapitMessages::CMD_GET_CHANNELS, (char*)&msg, sizeof(msg))) ? receive_channel_list(channels, utf_encoded) : false;

	close_connection();
	return return_value;
}



/* request information about a particular channel_id */

/* channel name */
std::string CZapitClient::getChannelName(const t_channel_id channel_id)
{
	send(CZapitMessages::CMD_GET_CHANNEL_NAME, (char *) & channel_id, sizeof(channel_id));

	CZapitMessages::responseGetChannelName response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return std::string(response.name);
}

/* is channel a TV channel ? */
bool CZapitClient::isChannelTVChannel(const t_channel_id channel_id)
{
	send(CZapitMessages::CMD_IS_TV_CHANNEL, (char *) & channel_id, sizeof(channel_id));

	CZapitMessages::responseGeneralTrueFalse response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return response.status;
}




/* restore bouquets so as if they were just loaded */
void CZapitClient::restoreBouquets()
{
	send(CZapitMessages::CMD_BQ_RESTORE);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
}

/* reloads channels and services*/
void CZapitClient::reinitChannels()
{
	send(CZapitMessages::CMD_REINIT_CHANNELS);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response), true);
	close_connection();
}

//called when sectionsd updates currentservices.xml
void CZapitClient::reloadCurrentServices()
{
	send(CZapitMessages::CMD_RELOAD_CURRENTSERVICES);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response), true);
	close_connection();
}

void CZapitClient::muteAudio(const bool mute)
{
	CZapitMessages::commandBoolean msg;

	msg.truefalse = mute;

	send(CZapitMessages::CMD_MUTE, (char*)&msg, sizeof(msg));

	close_connection();
}

void CZapitClient::setVolume(const unsigned int left, const unsigned int right)
{
	CZapitMessages::commandVolume msg;

	msg.left = left;
	msg.right = right;

	send(CZapitMessages::CMD_SET_VOLUME, (char*)&msg, sizeof(msg));

	close_connection();
}


delivery_system_t CZapitClient::getDeliverySystem(void)
{
	send(CZapitMessages::CMD_GET_DELIVERY_SYSTEM, 0, 0);

	CZapitMessages::responseDeliverySystem response;

	if (!CBasicClient::receive_data((char* )&response, sizeof(response)))
		response.system = DVB_S;  // return DVB_S if communication fails

	close_connection();

	return response.system;
}

bool CZapitClient::get_current_TP(TP_params* TP)
{
	TP_params TP_temp;
	send(CZapitMessages::CMD_GET_CURRENT_TP);
	bool reply = CBasicClient::receive_data((char*)&TP_temp, sizeof(TP_temp));
	memcpy(TP, &TP_temp, sizeof(TP_temp));
	close_connection();
	return reply;
}

/* sends diseqc 1.2 motor command */
void CZapitClient::sendMotorCommand(uint8_t cmdtype, uint8_t address, uint8_t cmd, uint8_t num_parameters, uint8_t param1, uint8_t param2)
{
	CZapitMessages::commandMotor msg;

	msg.cmdtype = cmdtype;
	msg.address = address;
	msg.cmd = cmd;
	msg.num_parameters = num_parameters;
	msg.param1 = param1;
	msg.param2 = param2;

	send(CZapitMessages::CMD_SEND_MOTOR_COMMAND, (char*)&msg, sizeof(msg));

	close_connection();
}


/***********************************************/
/*                                             */
/*  Scanning stuff                             */
/*                                             */
/***********************************************/

/* start TS-Scan */
bool CZapitClient::startScan(const bool  scan_mode, int8_t diseqc)
{
	CZapitMessages::startScan msg;

	msg.scan_mode = scan_mode;
	msg.diseqc = diseqc;

	bool reply = send(CZapitMessages::CMD_SCANSTART, (char*)&msg, sizeof(msg));

	close_connection();
	return reply;
}

bool CZapitClient::stopScan()
{
        bool reply = send(CZapitMessages::CMD_SCANSTOP);
        close_connection();
        return reply;
}

/* start manual scan */
bool CZapitClient::scan_TP(TP_params TP)
{
	bool reply = send(CZapitMessages::CMD_SCAN_TP, (char*)&TP, sizeof(TP));
	close_connection();
	return reply;
}

/* query if ts-scan is ready - response gives status */
bool CZapitClient::isScanReady(unsigned int &satellite,  unsigned int &processed_transponder, unsigned int &transponder, unsigned int &services )
{
	send(CZapitMessages::CMD_SCANREADY);

	CZapitMessages::responseIsScanReady response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	satellite = response.satellite;
	processed_transponder = response.processed_transponder;
	transponder = response.transponder;
	services = response.services;

	close_connection();
	return response.scanReady;
}

/* query possible satellits*/
void CZapitClient::getScanSatelliteList(SatelliteList& satelliteList)
{
	uint32_t satlength;

	send(CZapitMessages::CMD_SCANGETSATLIST);

	responseGetSatelliteList response;
	while (CBasicClient::receive_data((char*)&satlength, sizeof(satlength)))
	{
		if (satlength == SATNAMES_END_MARKER)
			break;

		if (!CBasicClient::receive_data((char*)&(response), satlength))
			break;

		satelliteList.push_back(response);
	}

	close_connection();

}

/* tell zapit which satellites to scan*/
void CZapitClient::setScanSatelliteList( ScanSatelliteList& satelliteList )
{
	CZapitMessages::commandInt num;
	send(CZapitMessages::CMD_SCANSETSCANSATLIST);

	num.val = satelliteList.size();
	send_data((char* )&num, sizeof(num));

	for (int i=0; i<num.val; i++)
	{
		send_data((char*)&satelliteList[i], sizeof(satelliteList[i]));
	}
	close_connection();
}

/* tell zapit stored satellite positions in diseqc 1.2 motor */
void CZapitClient::setScanMotorPosList( ScanMotorPosList& motorPosList )
{
	CZapitMessages::commandInt num;
	send(CZapitMessages::CMD_SCANSETSCANMOTORPOSLIST);

	num.val = motorPosList.size();
	send_data((char* )&num, sizeof(num));

	for (int i=0; i<num.val; i++)
	{
		send_data((char*)&motorPosList[i], sizeof(motorPosList[i]));
	}
	close_connection();
}

/* set diseqcType*/
void CZapitClient::setDiseqcType(const diseqc_t diseqc)
{
	send(CZapitMessages::CMD_SCANSETDISEQCTYPE, (const char *) & diseqc, sizeof(diseqc));
	close_connection();
}

/* set diseqcRepeat*/
void CZapitClient::setDiseqcRepeat(const uint32_t repeat)
{
	send(CZapitMessages::CMD_SCANSETDISEQCREPEAT, (const char *) & repeat, sizeof(repeat));
	close_connection();
}

/* set diseqcRepeat*/
void CZapitClient::setScanBouquetMode(const bouquetMode mode)
{
	send(CZapitMessages::CMD_SCANSETBOUQUETMODE, (const char *) & mode, sizeof(mode));
	close_connection();
}

/* set Scan-TYpe for channelsearch */
void CZapitClient::setScanType(const scanType mode)
{
	send(CZapitMessages::CMD_SCANSETTYPE, (const char *) & mode, sizeof(mode));
	close_connection();
}

//
// -- query Frontend Signal parameters
//
void CZapitClient::getFESignal (struct responseFESignal &f)
{
	struct responseFESignal rsignal;

	send(CZapitMessages::CMD_GET_FE_SIGNAL);
	CBasicClient::receive_data((char *) &rsignal, sizeof(rsignal));

	f.sig = rsignal.sig;
	f.snr = rsignal.snr;
	f.ber = rsignal.ber;

	close_connection();
}


/***********************************************/
/*                                             */
/* Bouquet editing functions                   */
/*                                             */
/***********************************************/

/* adds bouquet at the end of the bouquetlist  */
void CZapitClient::addBouquet(const char * const name)
{
	if (send(CZapitMessages::CMD_BQ_ADD_BOUQUET))
		send_string(name);

	close_connection();
}

/* moves a bouquet from one position to another */
/* bouquets are numbered starting at 0 */
void CZapitClient::moveBouquet(const unsigned int bouquet, const unsigned int newPos)
{
	CZapitMessages::commandMoveBouquet msg;

	msg.bouquet = bouquet;
	msg.newPos = newPos;

	send(CZapitMessages::CMD_BQ_MOVE_BOUQUET, (char*)&msg, sizeof(msg));
	close_connection();
}

/* deletes a bouquet with all its channels*/
/* bouquets are numbered starting at 0 */
void CZapitClient::deleteBouquet(const unsigned int bouquet)
{
	CZapitMessages::commandDeleteBouquet msg;

	msg.bouquet = bouquet;

	send(CZapitMessages::CMD_BQ_DELETE_BOUQUET, (char*)&msg, sizeof(msg));

	close_connection();
}

/* assigns new name to bouquet */
/* bouquets are numbered starting at 0 */
void CZapitClient::renameBouquet(const unsigned int bouquet, const char * const newName)
{
	CZapitMessages::commandRenameBouquet msg;

	msg.bouquet = bouquet;

	if (send(CZapitMessages::CMD_BQ_RENAME_BOUQUET, (char*)&msg, sizeof(msg)))
		send_string(newName);

	close_connection();
}

// -- check if Bouquet-Name exists
// -- Return: Bouquet-ID  or  -1 == no Bouquet found
/* bouquets are numbered starting at 0 */
signed int CZapitClient::existsBouquet(const char * const name)
{
	CZapitMessages::responseGeneralInteger response;

	if (send(CZapitMessages::CMD_BQ_EXISTS_BOUQUET))
		send_string(name);

	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return response.number;
}

// -- check if Channel already is in Bouquet
// -- Return: true/false
/* bouquets are numbered starting at 0 */
bool CZapitClient::existsChannelInBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitMessages::commandExistsChannelInBouquet msg;
	CZapitMessages::responseGeneralTrueFalse response;

	msg.bouquet    = bouquet;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_BQ_EXISTS_CHANNEL_IN_BOUQUET, (char*)&msg, sizeof(msg));

	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return (unsigned int) response.status;
}



/* moves a channel of a bouquet from one position to another, channel lists begin at position=1*/
/* bouquets are numbered starting at 0 */
void CZapitClient::moveChannel( unsigned int bouquet, unsigned int oldPos, unsigned int newPos, channelsMode mode)
{
	CZapitMessages::commandMoveChannel msg;

	msg.bouquet = bouquet;
	msg.oldPos  = oldPos - 1;
	msg.newPos  = newPos - 1;
	msg.mode    = mode;

	send(CZapitMessages::CMD_BQ_MOVE_CHANNEL, (char*)&msg, sizeof(msg));

	close_connection();
}

/* adds a channel at the end of then channel list to specified bouquet */
/* same channels can be in more than one bouquet */
/* bouquets can contain both tv and radio channels */
/* bouquets are numbered starting at 0 */
void CZapitClient::addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitMessages::commandAddChannelToBouquet msg;

	msg.bouquet    = bouquet;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_BQ_ADD_CHANNEL_TO_BOUQUET, (char*)&msg, sizeof(msg));

	close_connection();
}

/* removes a channel from specified bouquet */
/* bouquets are numbered starting at 0 */
void CZapitClient::removeChannelFromBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitMessages::commandRemoveChannelFromBouquet msg;

	msg.bouquet    = bouquet;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_BQ_REMOVE_CHANNEL_FROM_BOUQUET, (char*)&msg, sizeof(msg));

	close_connection();
}

/* set a bouquet's lock-state*/
/* bouquets are numbered starting at 0 */
void CZapitClient::setBouquetLock(const unsigned int bouquet, const bool lock)
{
	CZapitMessages::commandBouquetState msg;

	msg.bouquet = bouquet;
	msg.state   = lock;

	send(CZapitMessages::CMD_BQ_SET_LOCKSTATE, (char*)&msg, sizeof(msg));

	close_connection();
}

/* set a bouquet's hidden-state*/
/* bouquets are numbered starting at 0 */
void CZapitClient::setBouquetHidden(const unsigned int bouquet, const bool hidden)
{
	CZapitMessages::commandBouquetState msg;

	msg.bouquet = bouquet;
	msg.state   = hidden;

	send(CZapitMessages::CMD_BQ_SET_HIDDENSTATE, (char*)&msg, sizeof(msg));
	close_connection();
}

/* renums the channellist, means gives the channels new numbers */
/* based on the bouquet order and their order within bouquets */
/* necessarily after bouquet editing operations*/
void CZapitClient::renumChannellist()
{
	send(CZapitMessages::CMD_BQ_RENUM_CHANNELLIST);
	close_connection();
}


/* saves current bouquet configuration to bouquets.xml*/
void CZapitClient::saveBouquets(const bool includeBouquetOthers)
{
	CZapitMessages::commandBoolean msg;
	msg.truefalse = includeBouquetOthers;
	send(CZapitMessages::CMD_BQ_SAVE_BOUQUETS, (char*)&msg, sizeof(msg));

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
}

void CZapitClient::setFastZap(const bool enable)
{
	CZapitMessages::commandBoolean msg;
	msg.truefalse = enable;
	send(CZapitMessages::CMD_SET_FASTZAP, (char*)&msg, sizeof(msg));
	close_connection();
}

void CZapitClient::setStandby(const bool enable)
{
	CZapitMessages::commandBoolean msg;
	msg.truefalse = enable;
	CZapitMessages::responseCmd response;
	// this can take some time, because zapit saves channel lists etc. on standby...
	send(CZapitMessages::CMD_SET_STANDBY, (char*)&msg, sizeof(msg));
	if (!CBasicClient::receive_data((char* )&response, sizeof(response)))
	{
		/* ...which might actually exceed the 7 seconds timeout of the basicsocket.
		   One alternative would be to use the MAX_TIMEOUT feature, but 300 seconds are
		   probably too much... */
		printf("CZapitClient::setStandby: first try failed, no need to worry, retrying...\n");
		send(CZapitMessages::CMD_SET_STANDBY, (char*)&msg, sizeof(msg));
		CBasicClient::receive_data((char* )&response, sizeof(response));
	}
	close_connection();
}

void CZapitClient::setVideoSystem_a(int video_system)
{
	if (video_system == 0)
		send(CZapitMessages::CMD_SET_PAL);
	else
		send(CZapitMessages::CMD_SET_NTSC);
	close_connection();
}


void CZapitClient::startPlayBack()
{
	send(CZapitMessages::CMD_SB_START_PLAYBACK);
	close_connection();
}

void CZapitClient::stopPlayBack()
{
	send(CZapitMessages::CMD_SB_STOP_PLAYBACK);
	close_connection();
}

bool CZapitClient::isPlayBackActive()
{
	send(CZapitMessages::CMD_SB_GET_PLAYBACK_ACTIVE);

	CZapitMessages::responseGetPlaybackState response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response.activated;
}

void CZapitClient::setDisplayFormat(const video_display_format_t format)
{
	CZapitMessages::commandInt msg;
	msg.val = format;
	send(CZapitMessages::CMD_SET_DISPLAY_FORMAT, (char*)&msg, sizeof(msg));
	close_connection();
}

void CZapitClient::setAudioMode(const int mode)
{
	CZapitMessages::commandInt msg;
	msg.val = mode;
	send(CZapitMessages::CMD_SET_AUDIO_MODE, (char*)&msg, sizeof(msg));
	close_connection();
}

void CZapitClient::setRecordMode(const bool activate)
{
	CZapitMessages::commandSetRecordMode msg;
	msg.activate = activate;
	send(CZapitMessages::CMD_SET_RECORD_MODE, (char*)&msg, sizeof(msg));
	close_connection();
}

bool CZapitClient::isRecordModeActive()
{
	send(CZapitMessages::CMD_GET_RECORD_MODE);

	CZapitMessages::responseGetRecordModeState response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response.activated;
}

#ifdef HAVE_DBOX_HARDWARE
void CZapitClient::IecOn()
{
	send(CZapitMessages::CMD_SET_AE_IEC_ON);
	close_connection();
}

void CZapitClient::IecOff()
{
	send(CZapitMessages::CMD_SET_AE_IEC_OFF);
	close_connection();
}

int CZapitClient::IecState()
{
	send(CZapitMessages::CMD_GET_AE_IEC_STATE);

	CZapitMessages::responseGeneralInteger response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response.number;
}

void CZapitClient::PlaybackSPTS()
{
	send(CZapitMessages::CMD_SET_AE_PLAYBACK_SPTS);
	close_connection();
}

void CZapitClient::PlaybackPES()
{
	send(CZapitMessages::CMD_SET_AE_PLAYBACK_PES);
	close_connection();
}

int CZapitClient::PlaybackState()
{
	send(CZapitMessages::CMD_GET_AE_PLAYBACK_STATE);

	CZapitMessages::responseGeneralInteger response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response.number;
}
#endif

void CZapitClient::registerEvent(const unsigned int eventID, const unsigned int clientID, const char * const udsName)
{
	CEventServer::commandRegisterEvent msg;

	msg.eventID = eventID;
	msg.clientID = clientID;

	strcpy(msg.udsName, udsName);

	send(CZapitMessages::CMD_REGISTEREVENTS, (char*)&msg, sizeof(msg));

	close_connection();
}

void CZapitClient::unRegisterEvent(const unsigned int eventID, const unsigned int clientID)
{
	CEventServer::commandUnRegisterEvent msg;

	msg.eventID = eventID;
	msg.clientID = clientID;

	send(CZapitMessages::CMD_UNREGISTEREVENTS, (char*)&msg, sizeof(msg));

	close_connection();
}
