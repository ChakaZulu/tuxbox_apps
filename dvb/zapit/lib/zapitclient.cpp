/*
  Client-Interface für zapit  -   DBoxII-Project

  $Id: zapitclient.cpp,v 1.4 2002/01/29 23:17:54 Simplex Exp $

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

  $Log: zapitclient.cpp,v $
  Revision 1.4  2002/01/29 23:17:54  Simplex
  bugfix

  Revision 1.3  2002/01/12 22:07:01  Simplex
  method for zapping with bouquet and channel

  Revision 1.2  2002/01/07 21:14:24  Simplex
  functions for start and stop videoplayback

  Revision 1.1  2002/01/06 19:10:06  Simplex
  made clientlib for zapit
  implemented bouquet-editor functions in lib


*/

#include "zapitclient.h"

CZapitClient::CZapitClient()
{
	sock_fd = 0;
}

bool CZapitClient::zapit_connect()
{
	zapit_close();

	sockaddr_in servaddr;
	char rip[]="127.0.0.1";
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
#ifdef HAS_SIN_LEN
	servaddr.sin_len = sizeof(servaddr); // needed ???
#endif
	inet_pton(AF_INET, rip, &servaddr.sin_addr);
	sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	servaddr.sin_port=htons(1505);
	if(connect(sock_fd, (sockaddr *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("[zapitclient] couldn't connect to  zapit!");
		return( false);
	}

}

bool CZapitClient::zapit_close()
{
	if(sock_fd!=0)
	{
		close(sock_fd);
		sock_fd=0;
	}
}

bool CZapitClient::send(char* data, int size)
{
	write(sock_fd, data, size);
}

bool CZapitClient::receive(char* data, int size)
{
	return (read(sock_fd, data, size) > 0);
}

/***********************************************/
/*                                             */
/* general functions for zapping               */
/*                                             */
/***********************************************/

/* zaps to channel of specifeid bouquet */
void CZapitClient::zapTo( unsigned int bouquet, unsigned int channel )
{
	commandHead msgHead;
	commandZapto msg;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_ZAPTO;

	msg.bouquet = bouquet;
	msg.channel = channel;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	send((char*)&msg, sizeof(msg));

	zapit_close();
}

/* gets all bouquets */
void CZapitClient::getBouquets( BouquetList& bouquets, bool emptyBouquetsToo = false)
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
void CZapitClient::getBouquetChannels( unsigned int bouquet, BouquetChannelList& channels)
{
	commandHead msgHead;
	commandGetBouquetChannels msg;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_GET_BOUQUET_CHANNELS;

	msg.bouquet = bouquet;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	send((char*)&msg, sizeof(msg));

	responseGetBouquetChannels response;
	while ( receive((char*)&response, sizeof(responseGetBouquetChannels)))
		channels.insert( channels.end(), response);
	zapit_close();

}

/***********************************************/
/*                                             */
/* Bouquet editing functions                   */
/*                                             */
/***********************************************/

/* adds bouquet at the end of the bouquetlist*/
void CZapitClient::addBouquet( string name)
{
	commandHead msgHead;
	commandAddBouquet msg;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_BQ_ADD_BOUQUET;

	strncpy( msg.name, name.c_str(), 30);

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	send((char*)&msg, sizeof(msg));
	zapit_close();
}

/* moves a bouquet from one position to another, bouquet list begins at position=1*/
void CZapitClient::moveBouquet( unsigned int bouquet, unsigned int newPos)
{
	commandHead msgHead;
	commandMoveBouquet msg;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_BQ_MOVE_BOUQUET;

	msg.bouquet = bouquet;
	msg.newPos = newPos;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	send((char*)&msg, sizeof(msg));
	zapit_close();
}

/* moves a channel of a bouquet from one position to another, channel lists begin at position=1*/
void CZapitClient::moveChannel( unsigned int bouquet, unsigned int oldPos, unsigned int newPos)
{
	commandHead msgHead;
	commandMoveChannel msg;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_BQ_MOVE_CHANNEL;

	msg.bouquet = bouquet;
	msg.oldPos = oldPos;
	msg.newPos = newPos;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	send((char*)&msg, sizeof(msg));
	zapit_close();
}

/* deletes a bouquet with all its channels*/
void CZapitClient::deleteBouquet( unsigned int bouquet)
{
	commandHead msgHead;
	commandDeleteBouquet msg;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_BQ_DELETE_BOUQUET;

	msg.bouquet = bouquet;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	send((char*)&msg, sizeof(msg));
	zapit_close();
}

/* assigns new name to bouquet*/
void CZapitClient::renameBouquet( unsigned int bouquet, string newName)
{
	commandHead msgHead;
	commandRenameBouquet msg;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_BQ_RENAME_BOUQUET;

	msg.bouquet = bouquet;
	strncpy( msg.name, newName.c_str(), 30);

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	send((char*)&msg, sizeof(msg));
	zapit_close();
}

/* adds a channel at the end of then channel list to specified bouquet */
/* same channels can be in more than one bouquet */
/* bouquets can contain both tv and radio channels */
void CZapitClient::addChannelToBouquet( unsigned int bouquet, unsigned int onid_sid)
{
	commandHead msgHead;
	commandAddChannelToBouquet msg;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_BQ_ADD_CHANNEL_TO_BOUQUET;

	msg.bouquet = bouquet;
	msg.onid_sid = onid_sid;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	send((char*)&msg, sizeof(msg));
	zapit_close();
}

/* removes a channel from specified bouquet */
void CZapitClient::removeChannelFromBouquet( unsigned int bouquet, unsigned int onid_sid)
{
	commandHead msgHead;
	commandRemoveChannelFromBouquet msg;
	msgHead.version=ACTVERSION;
	msgHead.cmd=CMD_BQ_REMOVE_CHANNEL_FROM_BOUQUET;

	msg.bouquet = bouquet;
	msg.onid_sid = onid_sid;

	zapit_connect();
	send((char*)&msgHead, sizeof(msgHead));
	send((char*)&msg, sizeof(msg));
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


