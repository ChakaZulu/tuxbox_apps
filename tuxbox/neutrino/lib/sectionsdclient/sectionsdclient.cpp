/*
  Client-Interface für zapit  -   DBoxII-Project

  $Id: sectionsdclient.cpp,v 1.12 2002/04/12 15:47:28 field Exp $

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

  $Log: sectionsdclient.cpp,v $
  Revision 1.12  2002/04/12 15:47:28  field
  laestigen Bug in der glibc2.2.5 umschifft

  Revision 1.11  2002/03/30 03:54:31  dirch
  sectionsd_close vergessen ;)

  Revision 1.10  2002/03/30 03:45:37  dirch
  getChannelEvents() gefixt, getEPGid() and getEPGidShort() added

  Revision 1.9  2002/03/28 14:58:30  dirch
  getChannelEvents() gefixt

  Revision 1.8  2002/03/22 17:12:06  field
  Weitere Updates, compiliert wieder

  Revision 1.6  2002/03/20 21:42:30  McClean
  add channel-event functionality

  Revision 1.5  2002/03/18 15:08:50  field
  Updates...

  Revision 1.4  2002/03/18 09:32:51  field
  nix bestimmtes...

  Revision 1.2  2002/03/07 18:33:43  field
  ClientLib angegangen, Events angefangen

  Revision 1.1  2002/01/07 21:28:22  McClean
  initial

  Revision 1.1  2002/01/06 19:10:06  Simplex
  made clientlib for zapit
  implemented bouquet-editor functions in lib


*/

#include "sectionsdclient.h"

CSectionsdClient::CSectionsdClient()
{
	sock_fd = 0;
}

bool CSectionsdClient::sectionsd_connect()
{
	sockaddr_in servaddr;
	char rip[]="127.0.0.1";
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;

	inet_pton(AF_INET, rip, &servaddr.sin_addr);
	sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	servaddr.sin_port=htons(sectionsd::portNumber);
	if(connect(sock_fd, (sockaddr *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("[sectionsdclient] couldn't connect to sectionsd!");
		return false;
	}
	return true;
}

int CSectionsdClient::readResponse(char* data, int size)
{
	struct sectionsd::msgResponseHeader responseHeader;
    receive((char*)&responseHeader, sizeof(responseHeader));

	if ( data != NULL )
	{
		if ( responseHeader.dataLength != size )
			return -1;
		else
			return receive(data, size);
	}
	else
		return responseHeader.dataLength;
}


bool CSectionsdClient::sectionsd_close()
{
	if(sock_fd!=0)
	{
		close(sock_fd);
		sock_fd=0;
	}
}

bool CSectionsdClient::send(char* data, int size)
{
	if(sock_fd)
	{
		write(sock_fd, data, size);
	}
}

bool CSectionsdClient::receive(char* data, int size)
{
	if(sock_fd)
	{
		read(sock_fd, data, size);
	}
}

void CSectionsdClient::registerEvent(unsigned int eventID, unsigned int clientID, string udsName)
{
	sectionsd::msgRequestHeader msg;
	CEventServer::commandRegisterEvent msg2;

	msg.version = 3;
	msg.command = sectionsd::CMD_registerEvents;
	msg.dataLength = sizeof( msg2 );

	msg2.eventID = eventID;
	msg2.clientID = clientID;

	strcpy(msg2.udsName, udsName.c_str());
	sectionsd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	sectionsd_close();
}

void CSectionsdClient::unRegisterEvent(unsigned int eventID, unsigned int clientID)
{
	sectionsd::msgRequestHeader msg;
	CEventServer::commandUnRegisterEvent msg2;

	msg.version = 3;
	msg.command = sectionsd::CMD_unregisterEvents;
	msg.dataLength = sizeof( msg2 );

	msg2.eventID = eventID;
	msg2.clientID = clientID;

	sectionsd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	sectionsd_close();
}

bool CSectionsdClient::getIsTimeSet()
{
	sectionsd::msgRequestHeader msg;
	sectionsd::responseIsTimeSet rmsg;

	msg.version = 2;
	msg.command = sectionsd::getIsTimeSet;
	msg.dataLength = 0;

	if ( sectionsd_connect() )
	{
		send((char*)&msg, sizeof(msg));
		readResponse((char*)&rmsg, sizeof(rmsg));
		sectionsd_close();

		return rmsg.IsTimeSet;
	}
	else
		return false;
}

void CSectionsdClient::setPauseScanning( bool doPause )
{
	sectionsd::msgRequestHeader msg;

	msg.version = 2;
	msg.command = sectionsd::pauseScanning;
	int PauseIt = ( doPause ) ? 1 : 0;
	msg.dataLength = sizeof( PauseIt );

	sectionsd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&PauseIt, msg.dataLength);
	readResponse();
	sectionsd_close();
}

void CSectionsdClient::setServiceChanged( unsigned ServiceKey, bool requestEvent )
{
	sectionsd::msgRequestHeader msg;

	msg.version = 2;
	msg.command = sectionsd::serviceChanged;
	msg.dataLength = 8;

	char* pData = new char[msg.dataLength];
    *((unsigned *)pData) = ServiceKey;
    *((bool *)(pData + 4)) = requestEvent;

	sectionsd_connect();
	send((char*)&msg, sizeof(msg));
	send(pData, msg.dataLength);
	delete[] pData;
	readResponse();
	sectionsd_close();
}


bool CSectionsdClient::getComponentTagsUniqueKey( unsigned long long uniqueKey, sectionsd::ComponentTagList& tags )
{
	sectionsd::msgRequestHeader msg;

	msg.version = 2;
	msg.command = sectionsd::ComponentTagsUniqueKey;
	msg.dataLength = sizeof(uniqueKey);

	if ( sectionsd_connect() )
	{
        tags.clear();

		send((char*)&msg, sizeof(msg));
		send((char*)&uniqueKey, sizeof(uniqueKey));

		int nBufSize = readResponse();

		char* pData = new char[nBufSize];
		receive(pData, nBufSize);
        char* dp = pData;

        int	count= *(int *) pData;
        dp+= sizeof(int);

		sectionsd::responseGetComponentTags response;
		for (int i= 0; i<count; i++)
		{
			response.component = dp;
			dp+= strlen(dp)+1;
			response.componentType = *(unsigned char *) dp;
			dp+=sizeof(unsigned char);
			response.componentTag = *(unsigned char *) dp;
			dp+=sizeof(unsigned char);
			response.streamContent = *(unsigned char *) dp;
			dp+=sizeof(unsigned char);

			tags.insert( tags.end(), response);
		}
		sectionsd_close();

		return true;
	}
	else
		return false;
}

bool CSectionsdClient::getLinkageDescriptorsUniqueKey( unsigned long long uniqueKey, sectionsd::LinkageDescriptorList& descriptors )
{
	sectionsd::msgRequestHeader msg;

	msg.version = 2;
	msg.command = sectionsd::LinkageDescriptorsUniqueKey;
	msg.dataLength = sizeof(uniqueKey);

	if ( sectionsd_connect() )
	{
        descriptors.clear();

		send((char*)&msg, sizeof(msg));
		send((char*)&uniqueKey, sizeof(uniqueKey));

		int nBufSize = readResponse();

		char* pData = new char[nBufSize];
		receive(pData, nBufSize);
        char* dp = pData;

        int	count= *(int *) pData;
        dp+= sizeof(int);

		sectionsd::responseGetLinkageDescriptors response;
		for (int i= 0; i<count; i++)
		{
			response.name = dp;
			dp+= strlen(dp)+1;
			response.transportStreamId = *(unsigned short *) dp;
			dp+=sizeof(unsigned short);
			response.originalNetworkId = *(unsigned short *) dp;
			dp+=sizeof(unsigned short);
			response.serviceId = *(unsigned short *) dp;
			dp+=sizeof(unsigned short);

			descriptors.insert( descriptors.end(), response);
		}
		sectionsd_close();
		return true;
	}
	else
		return false;
}

bool CSectionsdClient::getNVODTimesServiceKey( unsigned serviceKey, sectionsd::NVODTimesList& nvod_list )
{
	sectionsd::msgRequestHeader msg;

	msg.version = 2;
	msg.command = sectionsd::timesNVODservice;
	msg.dataLength =  sizeof(serviceKey);

	if ( sectionsd_connect() )
	{
        nvod_list.clear();

		send((char*)&msg, sizeof(msg));
		send((char*)&serviceKey, sizeof(serviceKey));

		int nBufSize = readResponse();

		char* pData = new char[nBufSize];
		receive(pData, nBufSize);
        char* dp = pData;

        sectionsd::responseGetNVODTimes response;

		while( dp< pData+ nBufSize )
		{
			response.onid_sid = *(unsigned *) dp;
			dp+=sizeof(unsigned);
			response.tsid = *(unsigned short *) dp;
			dp+=sizeof(unsigned short);
            response.zeit = *(sectionsd::sectionsdTime*) dp;
			dp+= sizeof(sectionsd::sectionsdTime);

			nvod_list.insert( nvod_list.end(), response);
		}
		sectionsd_close();
		return true;
	}
	else
		return false;
}


bool CSectionsdClient::getCurrentNextServiceKey( unsigned serviceKey, sectionsd::responseGetCurrentNextInfoChannelID& current_next )
{
	sectionsd::msgRequestHeader msg;

	msg.version = 2;
	msg.command = sectionsd::currentNextInformationID;
	msg.dataLength = sizeof(serviceKey);

	if ( sectionsd_connect() )
	{
		send((char*)&msg, sizeof(msg));
		send((char*)&serviceKey, sizeof(serviceKey));

		int nBufSize = readResponse();

		char* pData = new char[nBufSize];
		receive(pData, nBufSize);
        char* dp = pData;

		// current
		current_next.current_uniqueKey = *((unsigned long long *)dp);
		dp+= sizeof(unsigned long long);
		current_next.current_zeit = *(sectionsd::sectionsdTime*) dp;
		dp+= sizeof(sectionsd::sectionsdTime);
		current_next.current_name = dp;
		dp+=strlen(dp)+1;

		// next
		current_next.next_uniqueKey = *((unsigned long long *)dp);
		dp+= sizeof(unsigned long long);
		current_next.next_zeit = *(sectionsd::sectionsdTime*) dp;
		dp+= sizeof(sectionsd::sectionsdTime);
		current_next.next_name = dp;
		dp+=strlen(dp)+1;

		current_next.flags = *(unsigned*) dp;
		dp+= sizeof(unsigned);

		sectionsd_close();
		return true;
	}
	else
		return false;
}



CChannelEventList CSectionsdClient::getChannelEvents()
{
	CChannelEventList eList;

	sectionsd::msgRequestHeader req;
	req.version = 2;

	req.command = sectionsd::actualEventListTVshortIDs;
	req.dataLength = 0;
	if ( sectionsd_connect() )
	{
		send((char*)&req, sizeof(req));

		int nBufSize = readResponse();

		if( nBufSize > 0)
		{
			char* pData = new char[nBufSize];
			receive(pData, nBufSize);
			sectionsd_close();

			char* dp = pData;

			while(dp < pData + nBufSize)
			{
				CChannelEvent aEvent;

				aEvent.serviceID = *((unsigned *) dp);
				dp+=sizeof(aEvent.serviceID);

				aEvent.eventID = *((unsigned long long *) dp);
				dp+=sizeof(aEvent.eventID);

				aEvent.startTime = *((time_t *) dp);
				dp+=sizeof(aEvent.startTime);

				aEvent.duration = *((unsigned *) dp);
				dp+=sizeof(aEvent.duration);

				aEvent.description= dp;
				dp+=strlen(dp)+1;

				aEvent.text= dp;
				dp+=strlen(dp)+1;

				eList.insert(eList.end(), aEvent);
			}
			delete[] pData;
		}
	}
	else
		printf("no connection to sectionsd\n");
	return eList;
}



bool CSectionsdClient::getEPGid( unsigned long long eventid,time_t starttime,CEPGData * epgdata)
{
	sectionsd::msgRequestHeader req;
	req.version = 2;

	req.command = sectionsd::epgEPGid;
	req.dataLength = 12;
	if ( sectionsd_connect() )
	{
		send((char*)&req, sizeof(req));
		send((char*)&eventid, sizeof(eventid));
		send((char*)&starttime,sizeof(starttime));

		int nBufSize = readResponse();
		if( nBufSize > 0)
		{
			char* pData = new char[nBufSize];
			receive(pData, nBufSize);
			sectionsd_close();

			char* dp = pData;


			epgdata->eventID = *((unsigned long long *)dp);
			dp+= sizeof(epgdata->eventID);

			epgdata->title = dp;
			dp+=strlen(dp)+1;
			epgdata->info1 = dp;
			dp+=strlen(dp)+1;
			epgdata->info2 = dp;
			dp+=strlen(dp)+1;
			epgdata->contentClassification = dp;
			dp+=strlen(dp)+1;
			epgdata->userClassification = dp;
			dp+=strlen(dp)+1;
			epgdata->fsk = *dp++;
//			printf("titel: %s\n",epgdata->title.c_str());

			epgdata->epg_times.startzeit = ((sectionsd::sectionsdTime *) dp)->startzeit;
			epgdata->epg_times.dauer = ((sectionsd::sectionsdTime *) dp)->dauer;
			dp+= sizeof(sectionsd::sectionsdTime);

			delete[] pData;
			return true;
		}
		else
			printf("no response from sectionsd\n");
	}
	else
		printf("no connection to sectionsd\n");
	return false;
}


bool CSectionsdClient::getEPGidShort( unsigned long long eventid,CShortEPGData * epgdata)
{
	sectionsd::msgRequestHeader req;
	req.version = 2;

	req.command = sectionsd::epgEPGidShort;
	req.dataLength = 8;
	if ( sectionsd_connect() )
	{
		send((char*)&req, sizeof(req));
		send((char*)&eventid, sizeof(eventid));

		int nBufSize = readResponse();
		if( nBufSize > 0)
		{
			char* pData = new char[nBufSize];
			receive(pData, nBufSize);
			sectionsd_close();

			char* dp = pData;

			for(int i = 0; i < nBufSize;i++)
				if(pData[i] == 0xff)
					pData[i] = 0;

			epgdata->title = dp;
			dp+=strlen(dp)+1;
			epgdata->info1 = dp;
			dp+=strlen(dp)+1;
			epgdata->info2 = dp;
			dp+=strlen(dp)+1;
//			printf("titel: %s\n",epgdata->title.c_str());


			delete[] pData;
			return true;
		}
		else
			printf("no response from sectionsd\n");
	}
	else
		printf("no connection to sectionsd\n");
	return false;
}


