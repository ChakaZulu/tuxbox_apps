/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: webdbox.cpp,v 1.32 2002/05/31 20:29:01 dirch Exp $

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

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

#include "webdbox.h"
#include "webserver.h"
#include "request.h"
#include "helper.h"

#define SA struct sockaddr
#define SAI struct sockaddr_in


//-------------------------------------------------------------------------
void TWebDbox::UpdateBouquets(void)
{
	GetBouquets();
	for(int i = 1; i <= BouquetList.size();i++)
		GetBouquet(i);
	GetChannelList();
}
//-------------------------------------------------------------------------

void TWebDbox::ZapTo(string target)
{
	int sidonid = atoi(target.c_str());
	if(sidonid == zapit->getCurrentServiceID())
	{
		if(Parent->DEBUG) printf("Kanal ist aktuell\n");
		return;
	}
	zapit->zapTo_serviceID(sidonid);
	sectionsd->setServiceChanged(sidonid,false);

}
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
// Konstruktor und destruktor
//-------------------------------------------------------------------------

TWebDbox::TWebDbox(CWebserver *server)
{
	Parent=server;
	standby_mode=false;

	controld = NULL;
	sectionsd = NULL;
	zapit = NULL;
	timerd = NULL;

	controld = new CControldClient();
	sectionsd = new CSectionsdClient();
	zapit = new CZapitClient();
	timerd = new CTimerdClient();

	UpdateBouquets();

	Dbox_Hersteller[1] = "Nokia";
	Dbox_Hersteller[2] = "Sagem";
	Dbox_Hersteller[3] = "Philips";
	videooutput_names[0] = "Composite";
	videooutput_names[1] = "RGB";
	videooutput_names[2] = "S-Video";
	videoformat_names[0] = "automatic";
	videoformat_names[1] = "16:9";
	videoformat_names[2] = "4:3";
	audiotype_names[1] =  "single channel";
	audiotype_names[2] = "dual channel";
	audiotype_names[3] = "joint stereo";
	audiotype_names[4] = "stereo";

	TimerEventNames[CTimerEvent::TIMER_NEXTPROGRAM] = "Umschalten";
	TimerEventNames[CTimerEvent::TIMER_SHUTDOWN] = "Shutdown";
	TimerEventNames[CTimerEvent::TIMER_STANDBY] = "Standby";
	TimerEventNames[CTimerEvent::TIMER_RECORD] = "Record";
	TimerEventNames[CTimerEvent::TIMER_ZAPTO] = "Zapto";
	TimerEventNames[CTimerEvent::TIMER_SLEEPTIMER] = "Sleeptimer";

	TimerEventStateNames[CTimerEvent::TIMERSTATE_SCHEDULED]="wartet";
	TimerEventStateNames[CTimerEvent::TIMERSTATE_PREANNOUNCE]="announced";
	TimerEventStateNames[CTimerEvent::TIMERSTATE_ISRUNNING]="aktiv";
	TimerEventStateNames[CTimerEvent::TIMERSTATE_HASFINISHED]= "beendet";
	TimerEventStateNames[CTimerEvent::TIMERSTATE_TERMINATED]= "beendet";

}
//-------------------------------------------------------------------------

TWebDbox::~TWebDbox()
{
	if(controld)
		delete controld;
	controld = NULL;
	if(sectionsd)
		delete sectionsd;
	sectionsd = NULL;
	if(zapit)
		delete zapit;
	zapit = NULL;
	if(timerd)
		delete timerd;
	timerd = NULL;
}

//-------------------------------------------------------------------------
// Get functions
//-------------------------------------------------------------------------

bool TWebDbox::GetStreamInfo(int bitInfo[10])
{
	char *key,*tmpptr,buf[100];
	int value, pos=0;

	FILE* fd = fopen("/proc/bus/bitstream", "rt");
	if (fd==NULL)
	{
		printf("error while opening proc-bitstream\n" );
		return false;
	}

	fgets(buf,29,fd);//dummy
	while(!feof(fd))
	{
		if(fgets(buf,29,fd)!=NULL)
		{
			buf[strlen(buf)-1]=0;
			tmpptr=buf;
			key=strsep(&tmpptr,":");
			for(;tmpptr[0]==' ';tmpptr++);
			value=atoi(tmpptr);
			bitInfo[pos]= value;
			pos++;
		}
	}
	fclose(fd);

	return true;
}

//-------------------------------------------------------------------------

void TWebDbox::GetChannelEvents()
{
	eList = sectionsd->getChannelEvents();
	CChannelEventList::iterator eventIterator;

    for( eventIterator = eList.begin(); eventIterator != eList.end(); eventIterator++ )
		ChannelListEvents[(*eventIterator).serviceID()] = &(*eventIterator);
}
//-------------------------------------------------------------------------
string TWebDbox::GetServiceName(int onid_sid)
{
	for(int i = 0; i < ChannelList.size();i++)
		if( ChannelList[i].onid_sid == onid_sid)
			return ChannelList[i].name;
	return "";
}

//-------------------------------------------------------------------------
bool TWebDbox::GetBouquets(void)
{
	BouquetList.clear();
	zapit->getBouquets(BouquetList); 
	return true;
}

//-------------------------------------------------------------------------
bool TWebDbox::GetBouquet(unsigned int BouquetNr)
{
	BouquetsList[BouquetNr].clear();
	zapit->getBouquetChannels(BouquetNr,BouquetsList[BouquetNr]);
	return true;
}
//-------------------------------------------------------------------------

bool TWebDbox::GetChannelList(void)
{
	ChannelList.clear();
	zapit->getChannels(ChannelList);
	return true;
}
