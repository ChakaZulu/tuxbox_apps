/*
  Client-Interface für zapit  -   DBoxII-Project

  $Id: sectionsdclient.cpp,v 1.25 2002/10/14 20:03:56 thegoodguy Exp $

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
  Revision 1.25  2002/10/14 20:03:56  thegoodguy
  Use CBasicClient in sectionsdclient, too ; always close_connection (even when the sectionsd did not respond)

  Revision 1.24  2002/10/13 21:21:49  thegoodguy
  Cleanup includes

  Revision 1.23  2002/10/13 11:35:03  woglinde


  yeah, its done neutrino compiles now again,
  you can go on and find bugs

  Revision 1.22  2002/10/13 05:42:51  woglinde


  2nd round of moving headers in lib/include

  Revision 1.21  2002/10/07 10:46:09  thegoodguy
  Enhancement in Clientlib (setEventsAreOldInMinutes) & code cleanup

  Revision 1.20  2002/09/25 22:15:09  thegoodguy
  Small bugfix (thx to gcc: "comparison is always false due to limited range of data type")

  Revision 1.19  2002/09/24 22:29:06  thegoodguy
  Code cleanup (kick out onid_sid)

  Revision 1.18  2002/07/27 17:14:51  obi
  no more warnings

  Revision 1.17  2002/04/18 13:09:53  field
  Sectionsd auf clientlib umgestellt :)

  Revision 1.14  2002/04/17 15:58:24  field
  Anpassungen

  Revision 1.13  2002/04/15 12:33:44  field
  Wesentlich verbessertes Paket-Handling (CPU-Last sollte viel besser sein
  *g*)

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
#include <stdio.h>

//#include <netinet/in.h>
//#include <netinet/in_systm.h>
//#include <netinet/ip.h>
//#include <netdb.h>
//#include <arpa/inet.h>


#include <eventserver.h>
#include <sectionsdclient/sectionsdclient.h>

int CSectionsdClient::readResponse(char* data, int size)
{
	struct sectionsd::msgResponseHeader responseHeader;
    receive_data((char*)&responseHeader, sizeof(responseHeader));

	if ( data != NULL )
	{
		if ( responseHeader.dataLength != size )
			return -1;
		else
			return receive_data(data, size);
	}
	else
		return responseHeader.dataLength;
}


bool CSectionsdClient::send(const unsigned char command, char* data = NULL, const unsigned int size = 0, const unsigned char version = 2)
{
	sectionsd::msgRequestHeader msgHead;

	msgHead.version    = version;
	msgHead.command    = command;
	msgHead.dataLength = size;

	open_connection(SECTIONSD_UDS_NAME); // if the return value is false, the next send_data call will return false, too

        if (!send_data((char*)&msgHead, sizeof(msgHead)))
            return false;

        if (size != 0)
            return send_data(data, size);

        return true;
}

void CSectionsdClient::registerEvent(unsigned int eventID, unsigned int clientID, string udsName)
{
	CEventServer::commandRegisterEvent msg2;

	msg2.eventID = eventID;
	msg2.clientID = clientID;

	strcpy(msg2.udsName, udsName.c_str());
	
	send(sectionsd::CMD_registerEvents, (char*)&msg2, sizeof(msg2), 3);

	close_connection();
}

void CSectionsdClient::unRegisterEvent(unsigned int eventID, unsigned int clientID)
{
	CEventServer::commandUnRegisterEvent msg2;

	msg2.eventID = eventID;
	msg2.clientID = clientID;

	send(sectionsd::CMD_unregisterEvents, (char*)&msg2, sizeof(msg2), 3);

	close_connection();
}

bool CSectionsdClient::getIsTimeSet()
{
	sectionsd::responseIsTimeSet rmsg;

	if (send(sectionsd::getIsTimeSet))
	{
		readResponse((char*)&rmsg, sizeof(rmsg));
		close_connection();

		return rmsg.IsTimeSet;
	}
	else
	{
		close_connection();
		return false;
	}
}


void CSectionsdClient::setEventsAreOldInMinutes(const unsigned short minutes)
{
	send(sectionsd::setEventsAreOldInMinutes, (char*)&minutes, sizeof(minutes));

	readResponse();
	close_connection();
}

void CSectionsdClient::setPauseSorting(const bool doPause)
{
	int PauseIt = (doPause) ? 1 : 0;

	send(sectionsd::pauseSorting, (char*)&PauseIt, sizeof(PauseIt));

	readResponse();
	close_connection();
}

void CSectionsdClient::setPauseScanning(const bool doPause)
{
	int PauseIt = (doPause) ? 1 : 0;

	send(sectionsd::pauseScanning, (char*)&PauseIt, sizeof(PauseIt));

	readResponse();
	close_connection();
}

void CSectionsdClient::setServiceChanged(const unsigned ServiceKey, const bool requestEvent)
{
	char pData[8];

	*((unsigned *)pData) = ServiceKey;
	*((bool *)(pData + 4)) = requestEvent;

	send(sectionsd::serviceChanged, pData, 8);

	readResponse();
	close_connection();
}


bool CSectionsdClient::getComponentTagsUniqueKey( unsigned long long uniqueKey, sectionsd::ComponentTagList& tags )
{
	if (send(sectionsd::ComponentTagsUniqueKey, (char*)&uniqueKey, sizeof(uniqueKey)))
	{
		tags.clear();

		int nBufSize = readResponse();

		char* pData = new char[nBufSize];
		receive_data(pData, nBufSize);
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
		close_connection();

		return true;
	}
	else
	{
		close_connection();
		return false;
	}
}

bool CSectionsdClient::getLinkageDescriptorsUniqueKey( unsigned long long uniqueKey, sectionsd::LinkageDescriptorList& descriptors )
{
	if (send(sectionsd::LinkageDescriptorsUniqueKey, (char*)&uniqueKey, sizeof(uniqueKey)))
	{
		descriptors.clear();

		int nBufSize = readResponse();

		char* pData = new char[nBufSize];
		receive_data(pData, nBufSize);
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
		close_connection();
		return true;
	}
	else
	{
		close_connection();
		return false;
	}
}

bool CSectionsdClient::getNVODTimesServiceKey( unsigned serviceKey, sectionsd::NVODTimesList& nvod_list )
{
	if (send(sectionsd::timesNVODservice, (char*)&serviceKey, sizeof(serviceKey)))
	{
		nvod_list.clear();

		int nBufSize = readResponse();

		char* pData = new char[nBufSize];
		receive_data(pData, nBufSize);
		char* dp = pData;

		sectionsd::responseGetNVODTimes response;

		while( dp< pData+ nBufSize )
		{
			response.service_id = *(t_service_id *) dp;			dp += sizeof(t_service_id);
			response.original_network_id = *(t_original_network_id *) dp;	dp += sizeof(t_original_network_id);
			response.transport_stream_id = *(t_transport_stream_id *) dp;	dp += sizeof(t_transport_stream_id);
			response.zeit = *(sectionsd::sectionsdTime*) dp;		dp += sizeof(sectionsd::sectionsdTime);

			nvod_list.insert( nvod_list.end(), response);
		}
		close_connection();
		return true;
	}
	else
	{
		close_connection();
		return false;
	}
}


bool CSectionsdClient::getCurrentNextServiceKey( unsigned serviceKey, sectionsd::responseGetCurrentNextInfoChannelID& current_next )
{
	if (send(sectionsd::currentNextInformationID, (char*)&serviceKey, sizeof(serviceKey)))
	{
		int nBufSize = readResponse();

		char* pData = new char[nBufSize];
		receive_data(pData, nBufSize);
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

		current_next.current_fsk = *(char*) dp;

		close_connection();
		return true;
	}
	else
	{
		close_connection();
		return false;
	}
}



CChannelEventList CSectionsdClient::getChannelEvents()
{
	CChannelEventList eList;

	if (send(sectionsd::actualEventListTVshortIDs))
	{
		int nBufSize = readResponse();

		if( nBufSize > 0)
		{
			char* pData = new char[nBufSize];
			receive_data(pData, nBufSize);

			char* dp = pData;

			while(dp < pData + nBufSize)
			{
				CChannelEvent aEvent;

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

				eList.push_back(aEvent);
			}
			delete[] pData;
		}
	}
	close_connection();
	return eList;
}

CChannelEventList CSectionsdClient::getEventsServiceKey( unsigned serviceKey )
{
	CChannelEventList eList;

	if (send(sectionsd::allEventsChannelID_, (char*)&serviceKey, sizeof(serviceKey)))
	{
		int nBufSize = readResponse();

		if( nBufSize > 0)
		{
			char* pData = new char[nBufSize];
			receive_data(pData, nBufSize);

			char* dp = pData;

			while(dp < pData + nBufSize)
			{
				CChannelEvent aEvent;

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

				eList.push_back(aEvent);
			}
			delete[] pData;
		}
	}

	close_connection();
	return eList;
}

bool CSectionsdClient::getActualEPGServiceKey( unsigned serviceKey, CEPGData * epgdata)
{
	epgdata->title = "";

	if (send(sectionsd::actualEPGchannelID, (char*)&serviceKey, sizeof(serviceKey)))
	{
		int nBufSize = readResponse();
		if( nBufSize > 0)
		{
			char* pData = new char[nBufSize];
			receive_data(pData, nBufSize);

			close_connection();

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

			epgdata->epg_times.startzeit = ((sectionsd::sectionsdTime *) dp)->startzeit;
			epgdata->epg_times.dauer = ((sectionsd::sectionsdTime *) dp)->dauer;
			dp+= sizeof(sectionsd::sectionsdTime);

			delete[] pData;
			return true;
		}
		else
			printf("no response from sectionsd\n");
	}

	close_connection();

	return false;
}


bool CSectionsdClient::getEPGid( unsigned long long eventid,time_t starttime,CEPGData * epgdata)
{
	sectionsd::msgRequestHeader req;
	req.version = 2;

	req.command = sectionsd::epgEPGid;
	req.dataLength = sizeof(eventid)+ sizeof(starttime);
	if (open_connection(SECTIONSD_UDS_NAME))
	{
		send_data((char*)&req, sizeof(req));
		send_data((char*)&eventid, sizeof(eventid));
		send_data((char*)&starttime, sizeof(starttime));

		int nBufSize = readResponse();
		if( nBufSize > 0)
		{
			char* pData = new char[nBufSize];
			receive_data(pData, nBufSize);
			close_connection();

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

			epgdata->epg_times.startzeit = ((sectionsd::sectionsdTime *) dp)->startzeit;
			epgdata->epg_times.dauer = ((sectionsd::sectionsdTime *) dp)->dauer;
			dp+= sizeof(sectionsd::sectionsdTime);

			delete[] pData;
			return true;
		}
		else
			printf("no response from sectionsd\n");
	}

	close_connection();

	return false;
}


bool CSectionsdClient::getEPGidShort( unsigned long long eventid,CShortEPGData * epgdata)
{
	if (send(sectionsd::epgEPGidShort, (char*)&eventid, sizeof(eventid)))
	{
		int nBufSize = readResponse();
		if( nBufSize > 0)
		{
			char* pData = new char[nBufSize];
			receive_data(pData, nBufSize);

			close_connection();

			char* dp = pData;

			for(int i = 0; i < nBufSize;i++)
				if(((unsigned char)pData[i]) == 0xff)
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

	close_connection();
	
	return false;
}
