/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/lib/zapitclient.cpp,v 1.49 2002/09/24 00:13:26 thegoodguy Exp $ *
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
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <sys/socket.h>


/* libevent */
#include <eventserver.h>


#include "zapitclient.h"



CZapitClient::CZapitClient()
{
	sock_fd = -1;
}

bool CZapitClient::zapit_connect()
{
	zapit_close();

	struct sockaddr_un servaddr;
	int clilen;
	std::string filename = ZAPIT_UDS_NAME;

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, filename.c_str());
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);

	if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("zapitclient: socket");
		sock_fd = -1;
		return false;
	}

	if(connect(sock_fd, (struct sockaddr*) &servaddr, clilen) <0 )
	{
		perror("zapitclient: connect");
		zapit_close();
		return false;
	}
	return true;
}

void CZapitClient::zapit_close()
{
	if(sock_fd != -1)
	{
		close(sock_fd);
		sock_fd = -1;
	}
}

inline void CZapitClient::send(char* data, const unsigned int size)
{
	if (sock_fd != -1)
		write(sock_fd, data, size);
}

bool CZapitClient::receive(char* data, const unsigned int size)
{
	if (sock_fd == -1)
		return false;
	else
		return (read(sock_fd, data, size) > 0);
}

void CZapitClient::send(const commands command, char* data, const unsigned int size)
{
	commandHead msgHead;
	msgHead.version = ACTVERSION;
	msgHead.cmd     = command;
	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	send(data, size);
}

/***********************************************/
/*					     */
/* general functions for zapping	       */
/*					     */
/***********************************************/

/* zaps to channel of specified bouquet */
void CZapitClient::zapTo( unsigned int bouquet, unsigned int channel )
{
	commandZapto msg;

	msg.bouquet = bouquet;
	msg.channel = channel;

	send(CMD_ZAPTO, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* zaps to channel by nr */
void CZapitClient::zapTo( unsigned int channel )
{
	commandZaptoChannelNr msg;

	msg.channel = channel;

	send(CMD_ZAPTO_CHANNELNR, (char*)&msg, sizeof(msg));

	zapit_close();
}

t_channel_id CZapitClient::getCurrentServiceID()
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_GET_CURRENT_SERVICEID;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));

	responseGetCurrentServiceID response;
	receive((char* )&response, sizeof(response));

	zapit_close();

	return response.channel_id;
}

CZapitClient::CCurrentServiceInfo CZapitClient::getCurrentServiceInfo()
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_GET_CURRENT_SERVICEINFO;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));

	responseCurrentServiceInfo response;
	receive((char* )&response, sizeof(response));

	CZapitClient::CCurrentServiceInfo erg;

	erg.onid = response.onid;
	erg.sid  = response.sid;
	erg.tsid = response.tsid;
	erg.vdid = response.vdid;
	erg.apid = response.apid;
	erg.pcrpid = response.pcrpid;
	erg.tsfrequency = response.tsfrequency;
	erg.polarisation = response.polarisation;
	erg.diseqc = response.diseqc;

	zapit_close();
	return erg;
}

void CZapitClient::getLastChannel(unsigned int &channumber, char &mode)
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_GET_LAST_CHANNEL;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));

	responseGetLastChannel response;
	receive((char* )&response, sizeof(response));
	channumber = response.channelNumber;
	mode = response.mode;

	zapit_close();
}

void CZapitClient::setAudioChannel( unsigned channel )
{
	commandSetAudioChannel msg;

	msg.channel = channel;

	send(CMD_SET_AUDIOCHAN, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* zaps to onid_sid, returns the "zap-status" */
unsigned int CZapitClient::zapTo_serviceID(const t_channel_id channel_id)
{
	commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CMD_ZAPTO_SERVICEID, (char*)&msg, sizeof(msg));

	responseZapComplete response;
	receive((char* )&response, sizeof(response));

	zapit_close();

	return response.zapStatus;
}

unsigned int CZapitClient::zapTo_subServiceID(const t_channel_id channel_id)
{
	commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CMD_ZAPTO_SUBSERVICEID, (char*)&msg, sizeof(msg));

	responseZapComplete response;
	receive((char* )&response, sizeof(response));

	zapit_close();

	return response.zapStatus;
}

/* zaps to channel, does NOT wait for completion (uses event) */
void CZapitClient::zapTo_serviceID_NOWAIT(const t_channel_id channel_id)
{
	commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CMD_ZAPTO_SERVICEID_NOWAIT, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* zaps to subservice, does NOT wait for completion (uses event) */
void CZapitClient::zapTo_subServiceID_NOWAIT(const t_channel_id channel_id)
{
	commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CMD_ZAPTO_SUBSERVICEID_NOWAIT, (char*)&msg, sizeof(msg));

	zapit_close();
}


void CZapitClient::setMode( channelsMode mode )
{
	commandSetMode msg;

	msg.mode = mode;

	send(CMD_SET_MODE, (char*)&msg, sizeof(msg));

	zapit_close();
}

void CZapitClient::setSubServices( subServiceList& subServices )
{
	unsigned int i;
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_SETSUBSERVICES;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));

	for (i = 0; i< subServices.size(); i++)
		send((char*)&subServices[i], sizeof(subServices[i]));

	zapit_close();
}

void CZapitClient::getPIDS( responseGetPIDs& pids )
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_GETPIDS;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	responseGetOtherPIDs response;
    receive((char* )&response, sizeof(response));
    memcpy(&pids.PIDs, &response, sizeof(response));

	responseGetAPIDs responseAPID;
	pids.APIDs.clear();
	while ( receive((char*)&responseAPID, sizeof(responseAPID)))
		pids.APIDs.insert( pids.APIDs.end(), responseAPID );
	zapit_close();
}

/* gets all bouquets */
void CZapitClient::getBouquets( BouquetList& bouquets, bool emptyBouquetsToo)
{
	commandHead msgHead;
	commandGetBouquets msg;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_GET_BOUQUETS;
	msg.emptyBouquetsToo = emptyBouquetsToo;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	send((char*)&msg, sizeof(msg));

	responseGetBouquets response;
	while ( receive((char*)&response, sizeof(responseGetBouquets)))
		bouquets.insert( bouquets.end(), response);
	zapit_close();
}

/* gets all channels that are in specified bouquet */
void CZapitClient::getBouquetChannels( unsigned int bouquet, BouquetChannelList& channels, channelsMode mode)
{
	commandGetBouquetChannels msg;

	msg.bouquet = bouquet;
	msg.mode = mode;

	send(CMD_GET_BOUQUET_CHANNELS, (char*)&msg, sizeof(msg));

	responseGetBouquetChannels response;
	while ( receive((char*)&response, sizeof(responseGetBouquetChannels)))
		channels.insert( channels.end(), response);
	zapit_close();
}

/* gets all channels */
void CZapitClient::getChannels( BouquetChannelList& channels, channelsMode mode, channelsOrder order)
{
	commandGetChannels msg;

	msg.mode = mode;
	msg.order = order;

	send(CMD_GET_CHANNELS, (char*)&msg, sizeof(msg));

	responseGetBouquetChannels response;
	while ( receive((char*)&response, sizeof(responseGetBouquetChannels)))
		channels.insert( channels.end(), response);
	zapit_close();
}

/* restore bouquets so as if they where just loaded*/
void CZapitClient::restoreBouquets()
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_RESTORE_BOUQUETS;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));

	responseCmd response;
	receive((char* )&response, sizeof(response));
	zapit_close();
}

/* reloads channels and services*/
void CZapitClient::reinitChannels()
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_REINIT_CHANNELS;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));

	responseCmd response;
	receive((char* )&response, sizeof(response));
	zapit_close();
}


/* commit bouquet change */
void CZapitClient::commitBouquetChange()
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_COMMIT_BOUQUET_CHANGE;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));

	responseCmd response;
	receive((char* )&response, sizeof(response));
	zapit_close();
}

void CZapitClient::muteAudio (bool mute)
{
	commandBoolean msg;

	msg.truefalse = mute;

	send(CMD_MUTE, (char*)&msg, sizeof(msg));

	zapit_close();
}

void CZapitClient::setVolume (unsigned int left, unsigned int right)
{
	commandVolume msg;

	msg.left = left;
	msg.right = right;

	send(CMD_SET_VOLUME, (char*)&msg, sizeof(msg));

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
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_SCANSTART;

	if (!zapit_connect())
		return false;

	send((char*)&msgHead, sizeof(msgHead));

	zapit_close();

	return true;
}

/* query if ts-scan is ready - response gives status */
bool CZapitClient::isScanReady(unsigned int &satellite, unsigned int &transponder, unsigned int &services )
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_SCANREADY;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));

	responseIsScanReady response;
	receive((char* )&response, sizeof(response));

	satellite = response.satellite;
	transponder = response.transponder;
	services = response.services;

	zapit_close();
	return response.scanReady;
}

/* query possible satellits*/
void CZapitClient::getScanSatelliteList( SatelliteList& satelliteList )
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_SCANGETSATLIST;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));

	responseGetSatelliteList response;
	while ( receive((char*)&response, sizeof(responseGetSatelliteList)))
		satelliteList.insert( satelliteList.end(), response);

	zapit_close();

}

/* tell zapit which satellites to scan*/
void CZapitClient::setScanSatelliteList( ScanSatelliteList& satelliteList )
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_SCANSETSCANSATLIST;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	for (uint i=0; i<satelliteList.size(); i++)
	{
		send((char*)&satelliteList[i], sizeof(satelliteList[i]));
	}
	zapit_close();
}

/* set diseqcType*/
void CZapitClient::setDiseqcType( diseqc_t diseqc)
{
	send(CMD_SCANSETDISEQCTYPE, (char*)&diseqc, sizeof(diseqc));
	zapit_close();
}

/* set diseqcRepeat*/
void CZapitClient::setDiseqcRepeat( uint32_t repeat)
{
	send(CMD_SCANSETDISEQCREPEAT, (char*)&repeat, sizeof(repeat));
	zapit_close();
}

/* set diseqcRepeat*/
void CZapitClient::setScanBouquetMode( bouquetMode mode)
{
	send(CMD_SCANSETBOUQUETMODE, (char*)&mode, sizeof(mode));
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
	commandAddBouquet msg;

	strncpy( msg.name, name.c_str(), 30);

	send(CMD_BQ_ADD_BOUQUET, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* moves a bouquet from one position to another, bouquet list begins at position=1*/
void CZapitClient::moveBouquet( unsigned int bouquet, unsigned int newPos)
{
	commandMoveBouquet msg;

	msg.bouquet = bouquet;
	msg.newPos = newPos;

	send(CMD_BQ_MOVE_BOUQUET, (char*)&msg, sizeof(msg));
	zapit_close();
}

//
// -- check if Bouquet-Name exists (2002-04-02 rasc)
// -- Return: Bouquet-ID  or  0 == no Bouquet found
//
unsigned int CZapitClient::existsBouquet(std::string name)
{
	commandExistsBouquet msg;
	responseGeneralInteger response;

	strncpy( msg.name, name.c_str(), 30);

	send(CMD_BQ_EXISTS_BOUQUET, (char*)&msg, sizeof(msg));

	receive((char* )&response, sizeof(response));
	zapit_close();
	return (unsigned int) response.number;
}


//
// -- check if Channel already is in Bouquet (2002-04-02 rasc)
// -- Return: true/false
//
bool CZapitClient::existsChannelInBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	commandExistsChannelInBouquet msg;
	responseGeneralTrueFalse response;

	msg.bouquet    = bouquet;
	msg.channel_id = channel_id;

	send(CMD_BQ_EXISTS_CHANNEL_IN_BOUQUET, (char*)&msg, sizeof(msg));

	receive((char* )&response, sizeof(response));
	zapit_close();
	return (unsigned int) response.status;
}



/* moves a channel of a bouquet from one position to another, channel lists begin at position=1*/
void CZapitClient::moveChannel( unsigned int bouquet, unsigned int oldPos, unsigned int newPos, channelsMode mode)
{
	commandMoveChannel msg;

	msg.bouquet = bouquet;
	msg.oldPos = oldPos;
	msg.newPos = newPos;
	msg.mode = mode;

	send(CMD_BQ_MOVE_CHANNEL, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* deletes a bouquet with all its channels*/
void CZapitClient::deleteBouquet( unsigned int bouquet)
{
	commandDeleteBouquet msg;

	msg.bouquet = bouquet;

	send(CMD_BQ_DELETE_BOUQUET, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* assigns new name to bouquet*/
void CZapitClient::renameBouquet( unsigned int bouquet, std::string newName)
{
	commandRenameBouquet msg;

	msg.bouquet = bouquet;
	strncpy( msg.name, newName.c_str(), 30);

	send(CMD_BQ_RENAME_BOUQUET, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* adds a channel at the end of then channel list to specified bouquet */
/* same channels can be in more than one bouquet */
/* bouquets can contain both tv and radio channels */
void CZapitClient::addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	commandAddChannelToBouquet msg;

	msg.bouquet    = bouquet;
	msg.channel_id = channel_id;

	send(CMD_BQ_ADD_CHANNEL_TO_BOUQUET, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* removes a channel from specified bouquet */
void CZapitClient::removeChannelFromBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	commandRemoveChannelFromBouquet msg;

	msg.bouquet    = bouquet;
	msg.channel_id = channel_id;

	send(CMD_BQ_REMOVE_CHANNEL_FROM_BOUQUET, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* set a bouquet's lock-state*/
void CZapitClient::setBouquetLock( unsigned int bouquet, bool lock)
{
	commandBouquetState msg;

	msg.bouquet = bouquet;
	msg.state   = lock;

	send(CMD_BQ_SET_LOCKSTATE, (char*)&msg, sizeof(msg));

	zapit_close();
}

/* set a bouquet's hidden-state*/
void CZapitClient::setBouquetHidden( unsigned int bouquet, bool hidden)
{
	commandBouquetState msg;

	msg.bouquet = bouquet;
	msg.state   = hidden;

	send(CMD_BQ_SET_HIDDENSTATE, (char*)&msg, sizeof(msg));
	zapit_close();
}

/* renums the channellist, means gives the channels new numbers */
/* based on the bouquet order and their order within bouquets */
/* necessarily after bouquet editing operations*/
void CZapitClient::renumChannellist()
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_BQ_RENUM_CHANNELLIST;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	zapit_close();
}


/* saves current bouquet configuration to bouquets.xml*/
void CZapitClient::saveBouquets()
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_BQ_SAVE_BOUQUETS;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));

	responseCmd response;
	receive((char* )&response, sizeof(response));

	zapit_close();
}


void CZapitClient::startPlayBack()
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_SB_START_PLAYBACK;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	zapit_close();
}

void CZapitClient::stopPlayBack()
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_SB_STOP_PLAYBACK;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	zapit_close();
}

bool CZapitClient::isPlayBackActive()
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_SB_GET_PLAYBACK_ACTIVE;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));

	responseGetPlaybackState response;
	receive((char* )&response, sizeof(response));

	zapit_close();
	return response.activated;
}

void CZapitClient::setDisplayFormat (int format)
{
	commandInt msg;
	msg.val = format;
	send(CMD_SET_DISPLAY_FORMAT, (char*)&msg, sizeof(msg));
	zapit_close();
}

void CZapitClient::setAudioMode (int mode)
{
	commandInt msg;
	msg.val = mode;
	send(CMD_SET_AUDIO_MODE, (char*)&msg, sizeof(msg));
	zapit_close();
}

void CZapitClient::setRecordMode( bool activate )
{
	commandSetRecordMode msg;
	msg.activate = activate;
	send(CMD_SET_RECORD_MODE, (char*)&msg, sizeof(msg));
	zapit_close();
}

bool CZapitClient::isRecordModeActive()
{
	commandHead msgHead;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_GET_RECORD_MODE;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));

	responseGetRecordModeState response;
	receive((char* )&response, sizeof(response));

	zapit_close();
	return response.activated;
}

void CZapitClient::registerEvent(unsigned int eventID, unsigned int clientID, std::string udsName)
{
	CEventServer::commandRegisterEvent msg;

	msg.eventID = eventID;
	msg.clientID = clientID;

	strcpy(msg.udsName, udsName.c_str());

	send(CMD_REGISTEREVENTS, (char*)&msg, sizeof(msg));

	zapit_close();
}

void CZapitClient::unRegisterEvent(unsigned int eventID, unsigned int clientID)
{
	CEventServer::commandUnRegisterEvent msg;

	msg.eventID = eventID;
	msg.clientID = clientID;

	send(CMD_UNREGISTEREVENTS, (char*)&msg, sizeof(msg));

	zapit_close();
}

