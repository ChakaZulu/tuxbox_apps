/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/lib/zapitclient.cpp,v 1.58 2002/10/03 18:07:37 thegoodguy Exp $ *
 *
 * Client-Interface für zapit - DBoxII-Project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de> & the DBoxII-Project
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

#include <stdio.h>

/* libevent */
#include <eventserver.h>


#include "include/zapitclient.h"
#include "msgtypes.h"


inline bool CZapitClient::zapit_connect()
{
	return open_connection(ZAPIT_UDS_NAME);
}

inline void CZapitClient::zapit_close()
{
	close_connection();
}

//void CZapitClient::send(const CZapitMessages::commands command, char* data = NULL, const unsigned int size = 0)
void CZapitClient::send(const unsigned char command, char* data = NULL, const unsigned int size = 0)
{
	CZapitMessages::commandHead msgHead;
	msgHead.version = CZapitMessages::ACTVERSION;
	msgHead.cmd     = command;
	zapit_connect();
	send_data((char*)&msgHead, sizeof(msgHead));
	if (size != 0)
	    send_data(data, size);
}

/***********************************************/
/*					     */
/* general functions for zapping	       */
/*					     */
/***********************************************/

/* zaps to channel of specified bouquet */
/* exception: bouquets are numbered starting at 0 in this routine! */
void CZapitClient::zapTo(const unsigned int bouquet, const unsigned int channel)
{
	CZapitMessages::commandZapto msg;

	msg.bouquet = bouquet;
	msg.channel = channel - 1;

	send(CZapitMessages::CMD_ZAPTO, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* zaps to channel by nr */
void CZapitClient::zapTo(const unsigned int channel)
{
	CZapitMessages::commandZaptoChannelNr msg;

	msg.channel = channel - 1;

	send(CZapitMessages::CMD_ZAPTO_CHANNELNR, (char*)&msg, sizeof(msg));

	zapit_close();
}

t_channel_id CZapitClient::getCurrentServiceID()
{
	send(CZapitMessages::CMD_GET_CURRENT_SERVICEID);

	CZapitMessages::responseGetCurrentServiceID response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	zapit_close();

	return response.channel_id;
}

CZapitClient::CCurrentServiceInfo CZapitClient::getCurrentServiceInfo()
{
	send(CZapitMessages::CMD_GET_CURRENT_SERVICEINFO);

	CZapitClient::CCurrentServiceInfo response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	zapit_close();
	return response;
}

void CZapitClient::getLastChannel(unsigned int &channumber, char &mode)
{
	send(CZapitMessages::CMD_GET_LAST_CHANNEL);

	CZapitClient::responseGetLastChannel response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	channumber = response.channelNumber + 1;
	mode = response.mode;

	zapit_close();
}

void CZapitClient::setAudioChannel(const unsigned int channel)
{
	CZapitMessages::commandSetAudioChannel msg;

	msg.channel = channel;

	send(CZapitMessages::CMD_SET_AUDIOCHAN, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* zaps to onid_sid, returns the "zap-status" */
unsigned int CZapitClient::zapTo_serviceID(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_ZAPTO_SERVICEID, (char*)&msg, sizeof(msg));

	CZapitMessages::responseZapComplete response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	zapit_close();

	return response.zapStatus;
}

unsigned int CZapitClient::zapTo_subServiceID(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_ZAPTO_SUBSERVICEID, (char*)&msg, sizeof(msg));

	CZapitMessages::responseZapComplete response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	zapit_close();

	return response.zapStatus;
}

/* zaps to channel, does NOT wait for completion (uses event) */
void CZapitClient::zapTo_serviceID_NOWAIT(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_ZAPTO_SERVICEID_NOWAIT, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* zaps to subservice, does NOT wait for completion (uses event) */
void CZapitClient::zapTo_subServiceID_NOWAIT(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_ZAPTO_SUBSERVICEID_NOWAIT, (char*)&msg, sizeof(msg));

	zapit_close();
}


void CZapitClient::setMode( channelsMode mode )
{
	CZapitMessages::commandSetMode msg;

	msg.mode = mode;

	send(CZapitMessages::CMD_SET_MODE, (char*)&msg, sizeof(msg));

	zapit_close();
}

void CZapitClient::setSubServices( subServiceList& subServices )
{
	unsigned int i;

	send(CZapitMessages::CMD_SETSUBSERVICES);

	for (i = 0; i< subServices.size(); i++)
		send_data((char*)&subServices[i], sizeof(subServices[i]));

	zapit_close();
}

void CZapitClient::getPIDS( responseGetPIDs& pids )
{
	send(CZapitMessages::CMD_GETPIDS);

	responseGetOtherPIDs response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	memcpy(&pids.PIDs, &response, sizeof(response));

	responseGetAPIDs responseAPID;
	pids.APIDs.clear();
	while ( CBasicClient::receive_data((char*)&responseAPID, sizeof(responseAPID)))
		pids.APIDs.push_back(responseAPID );
	zapit_close();
}

/* gets all bouquets */
void CZapitClient::getBouquets( BouquetList& bouquets, bool emptyBouquetsToo)
{
	CZapitMessages::commandGetBouquets msg;

	msg.emptyBouquetsToo = emptyBouquetsToo;

	send(CZapitMessages::CMD_GET_BOUQUETS, (char*)&msg, sizeof(msg));

	responseGetBouquets response;
	while (CBasicClient::receive_data((char*)&response, sizeof(responseGetBouquets)))
	{
		response.bouquet_nr++;
		bouquets.push_back(response);
	}
	zapit_close();
}

/* gets all channels that are in specified bouquet */
void CZapitClient::getBouquetChannels( unsigned int bouquet, BouquetChannelList& channels, channelsMode mode)
{
	CZapitMessages::commandGetBouquetChannels msg;

	msg.bouquet = bouquet;
	msg.mode = mode;

	send(CZapitMessages::CMD_GET_BOUQUET_CHANNELS, (char*)&msg, sizeof(msg));

	responseGetBouquetChannels response;
	while (CBasicClient::receive_data((char*)&response, sizeof(responseGetBouquetChannels)))
	{
		response.nr++;
		channels.push_back(response);
	}
	zapit_close();
}

/* gets all channels */
void CZapitClient::getChannels( BouquetChannelList& channels, channelsMode mode, channelsOrder order)
{
	CZapitMessages::commandGetChannels msg;

	msg.mode = mode;
	msg.order = order;

	send(CZapitMessages::CMD_GET_CHANNELS, (char*)&msg, sizeof(msg));

	responseGetBouquetChannels response;
	while (CBasicClient::receive_data((char*)&response, sizeof(responseGetBouquetChannels)))
	{
		response.nr++;
		channels.push_back(response);
	}
	zapit_close();
}

/* restore bouquets so as if they where just loaded*/
void CZapitClient::restoreBouquets()
{
	send(CZapitMessages::CMD_RESTORE_BOUQUETS);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	zapit_close();
}

/* reloads channels and services*/
void CZapitClient::reinitChannels()
{
	send(CZapitMessages::CMD_REINIT_CHANNELS);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	zapit_close();
}


/* commit bouquet change */
void CZapitClient::commitBouquetChange()
{
	send(CZapitMessages::CMD_COMMIT_BOUQUET_CHANGE);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	zapit_close();
}

void CZapitClient::muteAudio (bool mute)
{
	CZapitMessages::commandBoolean msg;

	msg.truefalse = mute;

	send(CZapitMessages::CMD_MUTE, (char*)&msg, sizeof(msg));

	zapit_close();
}

void CZapitClient::setVolume (unsigned int left, unsigned int right)
{
	CZapitMessages::commandVolume msg;

	msg.left = left;
	msg.right = right;

	send(CZapitMessages::CMD_SET_VOLUME, (char*)&msg, sizeof(msg));

	zapit_close();
}


/***********************************************/
/*					     */
/*  Scanning stuff			     */
/*					     */
/***********************************************/

/* start TS-Scan */
bool CZapitClient::startScan()
{
	CZapitMessages::commandHead msgHead;
	msgHead.version = CZapitMessages::ACTVERSION;
	msgHead.cmd     = CZapitMessages::CMD_SCANSTART;

	if (!zapit_connect())
		return false;

	send_data((char*)&msgHead, sizeof(msgHead));

	zapit_close();

	return true;
}

/* query if ts-scan is ready - response gives status */
bool CZapitClient::isScanReady(unsigned int &satellite, unsigned int &transponder, unsigned int &services )
{
	send(CZapitMessages::CMD_SCANREADY);

	CZapitMessages::responseIsScanReady response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	satellite = response.satellite;
	transponder = response.transponder;
	services = response.services;

	zapit_close();
	return response.scanReady;
}

/* query possible satellits*/
void CZapitClient::getScanSatelliteList( SatelliteList& satelliteList )
{
	send(CZapitMessages::CMD_SCANGETSATLIST);

	responseGetSatelliteList response;
	while ( CBasicClient::receive_data((char*)&response, sizeof(responseGetSatelliteList)))
		satelliteList.push_back(response);

	zapit_close();

}

/* tell zapit which satellites to scan*/
void CZapitClient::setScanSatelliteList( ScanSatelliteList& satelliteList )
{
	send(CZapitMessages::CMD_SCANSETSCANSATLIST);

	for (uint i=0; i<satelliteList.size(); i++)
	{
		send_data((char*)&satelliteList[i], sizeof(satelliteList[i]));
	}
	zapit_close();
}

/* set diseqcType*/
void CZapitClient::setDiseqcType( diseqc_t diseqc)
{
	send(CZapitMessages::CMD_SCANSETDISEQCTYPE, (char*)&diseqc, sizeof(diseqc));
	zapit_close();
}

/* set diseqcRepeat*/
void CZapitClient::setDiseqcRepeat( uint32_t repeat)
{
	send(CZapitMessages::CMD_SCANSETDISEQCREPEAT, (char*)&repeat, sizeof(repeat));
	zapit_close();
}

/* set diseqcRepeat*/
void CZapitClient::setScanBouquetMode( bouquetMode mode)
{
	send(CZapitMessages::CMD_SCANSETBOUQUETMODE, (char*)&mode, sizeof(mode));
	zapit_close();
}


/***********************************************/
/*					     */
/* Bouquet editing functions		   */
/*					     */
/***********************************************/

/* adds bouquet at the end of the bouquetlist*/
void CZapitClient::addBouquet(std::string name)
{
	CZapitMessages::commandAddBouquet msg;

	strncpy( msg.name, name.c_str(), 30);

	send(CZapitMessages::CMD_BQ_ADD_BOUQUET, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* moves a bouquet from one position to another */
/* exception: bouquets are numbered starting at 0 in this routine! */
void CZapitClient::moveBouquet(const unsigned int bouquet, const unsigned int newPos)
{
	CZapitMessages::commandMoveBouquet msg;

	msg.bouquet = bouquet;
	msg.newPos = newPos;

	send(CZapitMessages::CMD_BQ_MOVE_BOUQUET, (char*)&msg, sizeof(msg));
	zapit_close();
}

/* deletes a bouquet with all its channels*/
/* exception: bouquets are numbered starting at 0 in this routine! */
void CZapitClient::deleteBouquet(const unsigned int bouquet)
{
	CZapitMessages::commandDeleteBouquet msg;

	msg.bouquet = bouquet;

	send(CZapitMessages::CMD_BQ_DELETE_BOUQUET, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* assigns new name to bouquet */
/* exception: bouquets are numbered starting at 0 in this routine! */
void CZapitClient::renameBouquet(const unsigned int bouquet, const std::string newName)
{
	CZapitMessages::commandRenameBouquet msg;

	msg.bouquet = bouquet;
	strncpy( msg.name, newName.c_str(), 30);

	send(CZapitMessages::CMD_BQ_RENAME_BOUQUET, (char*)&msg, sizeof(msg));

	zapit_close();
}

//
// -- check if Bouquet-Name exists (2002-04-02 rasc)
// -- Return: Bouquet-ID  or  0 == no Bouquet found
//
unsigned int CZapitClient::existsBouquet(std::string name)
{
	CZapitMessages::commandExistsBouquet msg;
	CZapitMessages::responseGeneralInteger response;

	strncpy( msg.name, name.c_str(), 30);

	send(CZapitMessages::CMD_BQ_EXISTS_BOUQUET, (char*)&msg, sizeof(msg));

	CBasicClient::receive_data((char* )&response, sizeof(response));
	zapit_close();
	return (unsigned int) response.number + 1;
}


//
// -- check if Channel already is in Bouquet (2002-04-02 rasc)
// -- Return: true/false
//
bool CZapitClient::existsChannelInBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitMessages::commandExistsChannelInBouquet msg;
	CZapitMessages::responseGeneralTrueFalse response;

	msg.bouquet    = bouquet - 1;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_BQ_EXISTS_CHANNEL_IN_BOUQUET, (char*)&msg, sizeof(msg));

	CBasicClient::receive_data((char* )&response, sizeof(response));
	zapit_close();
	return (unsigned int) response.status;
}



/* moves a channel of a bouquet from one position to another, channel lists begin at position=1*/
void CZapitClient::moveChannel( unsigned int bouquet, unsigned int oldPos, unsigned int newPos, channelsMode mode)
{
	CZapitMessages::commandMoveChannel msg;

	msg.bouquet = bouquet - 1;
	msg.oldPos  = oldPos - 1;
	msg.newPos  = newPos - 1;
	msg.mode    = mode;

	send(CZapitMessages::CMD_BQ_MOVE_CHANNEL, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* adds a channel at the end of then channel list to specified bouquet */
/* same channels can be in more than one bouquet */
/* bouquets can contain both tv and radio channels */
void CZapitClient::addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitMessages::commandAddChannelToBouquet msg;

	msg.bouquet    = bouquet - 1;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_BQ_ADD_CHANNEL_TO_BOUQUET, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* removes a channel from specified bouquet */
void CZapitClient::removeChannelFromBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitMessages::commandRemoveChannelFromBouquet msg;

	msg.bouquet    = bouquet - 1;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_BQ_REMOVE_CHANNEL_FROM_BOUQUET, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* set a bouquet's lock-state*/
/* exception: bouquets are numbered starting at 0 in this routine! */
void CZapitClient::setBouquetLock(const unsigned int bouquet, const bool lock)
{
	CZapitMessages::commandBouquetState msg;

	msg.bouquet = bouquet;
	msg.state   = lock;

	send(CZapitMessages::CMD_BQ_SET_LOCKSTATE, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* set a bouquet's hidden-state*/
/* exception: bouquets are numbered starting at 0 in this routine! */
void CZapitClient::setBouquetHidden(const unsigned int bouquet, const bool hidden)
{
	CZapitMessages::commandBouquetState msg;

	msg.bouquet = bouquet;
	msg.state   = hidden;

	send(CZapitMessages::CMD_BQ_SET_HIDDENSTATE, (char*)&msg, sizeof(msg));
	zapit_close();
}

/* renums the channellist, means gives the channels new numbers */
/* based on the bouquet order and their order within bouquets */
/* necessarily after bouquet editing operations*/
void CZapitClient::renumChannellist()
{
	send(CZapitMessages::CMD_BQ_RENUM_CHANNELLIST);
	zapit_close();
}


/* saves current bouquet configuration to bouquets.xml*/
void CZapitClient::saveBouquets()
{
	send(CZapitMessages::CMD_BQ_SAVE_BOUQUETS);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	zapit_close();
}


void CZapitClient::startPlayBack()
{
	send(CZapitMessages::CMD_SB_START_PLAYBACK);
	zapit_close();
}

void CZapitClient::stopPlayBack()
{
	send(CZapitMessages::CMD_SB_STOP_PLAYBACK);
	zapit_close();
}

bool CZapitClient::isPlayBackActive()
{
	send(CZapitMessages::CMD_SB_GET_PLAYBACK_ACTIVE);

	CZapitMessages::responseGetPlaybackState response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	zapit_close();
	return response.activated;
}

void CZapitClient::setDisplayFormat (int format)
{
	CZapitMessages::commandInt msg;
	msg.val = format;
	send(CZapitMessages::CMD_SET_DISPLAY_FORMAT, (char*)&msg, sizeof(msg));
	zapit_close();
}

void CZapitClient::setAudioMode (int mode)
{
	CZapitMessages::commandInt msg;
	msg.val = mode;
	send(CZapitMessages::CMD_SET_AUDIO_MODE, (char*)&msg, sizeof(msg));
	zapit_close();
}

void CZapitClient::setRecordMode( bool activate )
{
	CZapitMessages::commandSetRecordMode msg;
	msg.activate = activate;
	send(CZapitMessages::CMD_SET_RECORD_MODE, (char*)&msg, sizeof(msg));
	zapit_close();
}

bool CZapitClient::isRecordModeActive()
{
	send(CZapitMessages::CMD_GET_RECORD_MODE);

	CZapitMessages::responseGetRecordModeState response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	zapit_close();
	return response.activated;
}

void CZapitClient::registerEvent(unsigned int eventID, unsigned int clientID, std::string udsName)
{
	CEventServer::commandRegisterEvent msg;

	msg.eventID = eventID;
	msg.clientID = clientID;

	strcpy(msg.udsName, udsName.c_str());

	send(CZapitMessages::CMD_REGISTEREVENTS, (char*)&msg, sizeof(msg));

	zapit_close();
}

void CZapitClient::unRegisterEvent(unsigned int eventID, unsigned int clientID)
{
	CEventServer::commandUnRegisterEvent msg;

	msg.eventID = eventID;
	msg.clientID = clientID;

	send(CZapitMessages::CMD_UNREGISTEREVENTS, (char*)&msg, sizeof(msg));

	zapit_close();
}

