/*
	nhttpd  -  DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: controlapi.cpp,v 1.32 2004/02/21 08:52:27 thegoodguy Exp $

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

// system
#include <unistd.h>
#include <string>
#include <cctype>

// tuxbox
#include <neutrinoMessages.h>

// nhttpd
#include "controlapi.h"
#include "debug.h"

bool CControlAPI::Execute(CWebserverRequest* request)
{
	unsigned int operation = 0;
	const char *operations[] =
	{
		"timer","setmode","standby","getdate","gettime","settings","getservicesxml",
		"getbouquetsxml","getonidsid","message","info","shutdown","volume",
		"channellist","getbouquet","getbouquets","epg","version","zapto", "startplugin",
		"getmode",NULL
	};

	dprintf("Execute CGI : %s\n",request->Filename.c_str());
	if(CDEBUG::getInstance()->Debug)
	{
		for(CStringList::iterator it = request->ParameterList.begin() ;
			 it != request->ParameterList.end() ; it++)
		{
			dprintf("  Parameter %s : %s\n",it->first.c_str(), it->second.c_str());
		}
	}
	// tolower(filename)
	for(unsigned int i = 0; i < request->Filename.length(); i++)
		request->Filename[i] = tolower(request->Filename[i]);

	while (operations[operation])
	{
		if (request->Filename.compare(operations[operation]) == 0)
			break;

		operation++;
	}

	if (operations[operation] == NULL) {
		request->Send404Error();
		return false;
	}

	if (request->Method == M_HEAD)
	{
		if ((operation != 6) && (operation != 7))
			request->SendPlainHeader("text/plain");
		else
			request->SendPlainHeader("text/xml");
		return true;
	}	

	switch (operation) {
	case 0:
		return TimerCGI(request);
	case 1:
		return SetModeCGI(request);
	case 2:
		return StandbyCGI(request);
	case 3:
		return GetDateCGI(request);
	case 4:
		return GetTimeCGI(request);
	case 5:
		return SettingsCGI(request);
	case 6:
		return GetServicesxmlCGI(request);
	case 7:
		return GetBouquetsxmlCGI(request);
	case 8:
		return GetChannel_IDCGI(request);
	case 9:
		return MessageCGI(request);
	case 10:
		return InfoCGI(request);
	case 11:
		return ShutdownCGI(request);
	case 12:
		return VolumeCGI(request);
	case 13:
		return ChannellistCGI(request);
	case 14:
		return GetBouquetCGI(request);
	case 15:
		return GetBouquetsCGI(request);
	case 16:
		return EpgCGI(request);
	case 17:
		return VersionCGI(request);
	case 18:
		return ZaptoCGI(request);
	case 19:
		return StartPluginCGI(request);
	case 20:
		return GetModeCGI(request);
	default:
		request->SendError();
		return false;
	}
}

//-------------------------------------------------------------------------

bool CControlAPI::TimerCGI(CWebserverRequest *request)
{
	request->SendPlainHeader("text/plain");
	
	if (Parent->Timerd->isTimerdAvailable())
	{
		if ((request->ParameterList.size() > 0))
		{
			if (request->ParameterList["action"] == "new")
			{
				Parent->WebAPI->doNewTimer(request);
				request->SendOk();
			}
		}
		else
		{
			SendTimers(request);
		}

		return true;
	}

	request->SendError();
	return false;
}

//-------------------------------------------------------------------------

bool CControlAPI::SetModeCGI(CWebserverRequest *request)
{
	int mode;

	// Standard httpd header senden
	request->SendPlainHeader("text/plain");
	
	if (request->ParameterList.size() > 0)
	{
		if (request->ParameterList["1"] == "status")
		{
			if (Parent->Zapit->isRecordModeActive())
				request->SocketWriteLn("on");
			else
				request->SocketWriteLn("off");
			return true;
		}
		
		if (request->ParameterList["1"] == "radio")	// in radio mode schalten
		{
			mode = NeutrinoMessages::mode_radio;
			Parent->EventServer->sendEvent(NeutrinoMessages::CHANGEMODE, CEventServer::INITID_HTTPD, (void *)&mode,sizeof(int));
			sleep(1);
			Parent->UpdateBouquets();
		}
		else if (request->ParameterList["1"] == "tv")	// in tv mode schalten
		{
			mode = NeutrinoMessages::mode_tv;
			Parent->EventServer->sendEvent(NeutrinoMessages::CHANGEMODE, CEventServer::INITID_HTTPD, (void *)&mode,sizeof(int));
			sleep(1);
			Parent->UpdateBouquets();
		}
		else if (request->ParameterList["record"] == "start")	// record mode starten
		{
			if(request->ParameterList["stopplayback"] == "true")
				Parent->Zapit->stopPlayBack();
			Parent->Sectionsd->setPauseScanning(true);
			Parent->Zapit->setRecordMode(true);
		}
		else if (request->ParameterList["record"] == "stop")	// recordmode beenden
		{
			Parent->Zapit->setRecordMode(false);
			Parent->Sectionsd->setPauseScanning(false);
         if (!Parent->Zapit->isPlayBackActive())
            Parent->Zapit->startPlayBack();
		}

		request->SendOk();
		return true;
	}
	else
	{
		request->SendError();
		return false;
	}
}

//-------------------------------------------------------------------------

bool CControlAPI::GetModeCGI(CWebserverRequest *request)
{
	int mode;

	// Standard httpd header senden
	request->SendPlainHeader("text/plain");
	
	mode = Parent->Zapit->getMode();
	if ( mode == CZapitClient::MODE_TV)
		request->SocketWriteLn("tv");
	else if ( mode == CZapitClient::MODE_RADIO)
		request->SocketWriteLn("radio");
	else
		request->SocketWriteLn("unknown");
	return true;
}

//-------------------------------------------------------------------------

bool CControlAPI::StandbyCGI(CWebserverRequest *request)
{
	request->SendPlainHeader("text/plain");          // Standard httpd header senden

	if (request->ParameterList.size() == 1)
	{
		if (request->ParameterList["1"] == "on")	// in standby mode schalten
			Parent->EventServer->sendEvent(NeutrinoMessages::STANDBY_ON, CEventServer::INITID_HTTPD);
		if (request->ParameterList["1"] == "off")	// standby mode ausschalten
			Parent->EventServer->sendEvent(NeutrinoMessages::STANDBY_OFF, CEventServer::INITID_HTTPD);
	}

	request->SendOk();
	return true;
}

//-------------------------------------------------------------------------

bool CControlAPI::GetDateCGI(CWebserverRequest *request)
{
	request->SendPlainHeader("text/plain");          // Standard httpd header senden

	if (request->ParameterList.size() == 0)
	{
		//paramlos
		char *timestr = new char[50];
		struct timeb tm;
		ftime(&tm);
		strftime(timestr, 20, "%d.%m.%Y\n", localtime(&tm.time) );	//aktuelle zeit ausgeben
		request->SocketWrite(timestr);
		delete[] timestr;
		return true;
	}
	else
	{
		request->SendError();
		return false;
	}
}

//-------------------------------------------------------------------------

bool CControlAPI::GetTimeCGI(CWebserverRequest *request)
{
	time_t now = time(NULL);

	request->SendPlainHeader("text/plain");          // Standard httpd header senden
	
	if (request->ParameterList.size() == 0)
	{
		//paramlos
		char *timestr = new char[50];
		struct tm *tm = localtime(&now);
		strftime(timestr, 50, "%H:%M:%S\n", tm );	// aktuelles datum ausgeben
		request->SocketWrite(timestr);
		delete[] timestr;
		return true;
	}

	if (request->ParameterList["1"].compare("rawtime") == 0)
	{
		request->printf("%ld\n",now);
		return true;
	}

	// if nothing matches
	request->SendError();
	return false;
}

//-------------------------------------------------------------------------

bool CControlAPI::SettingsCGI(CWebserverRequest *request)		// sendet die settings
{
	request->SendPlainHeader("text/plain");
	SendSettings(request);
	return true;
}

//-------------------------------------------------------------------------

bool CControlAPI::GetServicesxmlCGI(CWebserverRequest *request)   // sendet die datei services.xml
{
	request->SendPlainHeader("text/xml");
	request->SendFile("/var/tuxbox/config/zapit","services.xml");
	return true;
}

//-------------------------------------------------------------------------

bool CControlAPI::GetBouquetsxmlCGI(CWebserverRequest *request)		// sendet die datei bouquets.xml
{
	request->SendPlainHeader("text/xml");
	request->SendFile("/var/tuxbox/config/zapit","bouquets.xml");
	return true;
}

//-------------------------------------------------------------------------

bool CControlAPI::GetChannel_IDCGI(CWebserverRequest *request) // sendet die aktuelle channel_id
{
	request->SendPlainHeader("text/plain");
	request->printf("%u\n", Parent->Zapit->getCurrentServiceID());
	return true;
}

//-------------------------------------------------------------------------

bool CControlAPI::MessageCGI(CWebserverRequest *request)
{
	request->SendPlainHeader("text/plain");          // Standard httpd header senden
	std::string message;
	int event = 0;

	if (request->ParameterList.size() == 0)
	{	//paramlos
		request->SendError();
		return false;
	}
	
	if (request->ParameterList["popup"] != "")
	{
		message = request->ParameterList["popup"];
		event = NeutrinoMessages::EVT_POPUP;
	}
	else if (request->ParameterList["nmsg"] != "")
	{
		message = request->ParameterList["nmsg"];
		event = NeutrinoMessages::EVT_EXTMSG;
	}
	
	if (event != 0)
	{
		request->URLDecode(message);
		Parent->EventServer->sendEvent(event, CEventServer::INITID_HTTPD, (void *) message.c_str(), message.length() + 1);
		request->SendOk();
		return true;
	}

	request->SendError();
	return false;
}

//-------------------------------------------------------------------------

bool CControlAPI::InfoCGI(CWebserverRequest *request)
{
	if (request->ParameterList.size() == 0)
	{
		//paramlos
		request->SocketWrite("Neutrino NG\n");
		return true;
	}
	else
	{
		if (request->ParameterList["1"] == "streaminfo")	// streaminfo ausgeben
		{
			request->SendPlainHeader("text/plain");			// Standard httpd header senden
			SendStreamInfo(request);
			return true;
		}
		else if (request->ParameterList["1"] == "settings")	// settings ausgeben
		{
			request->SendPlainHeader("text/plain");			// Standard httpd header senden
			SendSettings(request);
			return true;
		}
		else if (request->ParameterList["1"] == "version")	// version file ausgeben
		{
			request->SendFile("/",".version");
			return true;
		}
		else if (request->ParameterList["1"] == "httpdversion")	// httpd version ausgeben
		{
			request->SendPlainHeader("text/plain");			// Standard httpd header senden
			request->SocketWrite("2");
			return true;
		}
		else
		{
			request->SendError();
			return false;
		}
	}
}

//-------------------------------------------------------------------------

bool CControlAPI::ShutdownCGI(CWebserverRequest *request)
{
	request->SendPlainHeader("text/plain");          // Standard httpd header senden
	
	if (request->ParameterList.size() == 0)
	{
		//paramlos
		Parent->EventServer->sendEvent(NeutrinoMessages::SHUTDOWN, CEventServer::INITID_HTTPD);
		request->SendOk();
		return true;
	}
	else
	{
		request->SendError();
		return false;
	}
}

//-------------------------------------------------------------------------

bool CControlAPI::VolumeCGI(CWebserverRequest *request)
{
	request->SendPlainHeader("text/plain");          // Standard httpd header senden
	
	if (request->ParameterList.size() == 0)
	{	//paramlos - aktuelle volume anzeigen
		request->printf("%d", Parent->Controld->getVolume());			// volume ausgeben
		return true;
	}
	else if (request->ParameterList.size() == 1)
	{
		if (request->ParameterList["1"].compare("mute") == 0)
		{
			Parent->Controld->setMute(true);
			request->SendOk();					// muten
			return true;
		}
		else if (request->ParameterList["1"].compare("unmute") == 0)
		{
			Parent->Controld->setMute(false);
			request->SendOk();					// unmuten
			return true;
		}
		else if (request->ParameterList["1"].compare("status") == 0)
		{
			request->SocketWrite((char *) (Parent->Controld->getMute() ? "1" : "0"));	//  mute
			return true;
		}
		else
		{	//set volume
			char vol = atol( request->ParameterList["1"].c_str() );
			Parent->Controld->setVolume(vol);
			request->SendOk();
			return true;
		}
	}
	else
	{
		request->SendError();
		return false;
	}
}

//-------------------------------------------------------------------------

bool CControlAPI::ChannellistCGI(CWebserverRequest *request)
{
	request->SendPlainHeader("text/plain");          // Standard httpd header senden
	SendChannelList(request);
	return true;
}

//-------------------------------------------------------------------------

bool CControlAPI::GetBouquetCGI(CWebserverRequest *request)
{
	CZapitClient::BouquetChannelList *bouquet;

	request->SendPlainHeader("text/plain");          // Standard httpd header senden

	if (request->ParameterList.size() > 0)
	{
		int mode = CZapitClient::MODE_CURRENT;
		
		if (request->ParameterList["mode"] != "")
		{
			if (request->ParameterList["mode"].compare("TV") == 0)
				mode = CZapitClient::MODE_TV;
			if (request->ParameterList["mode"].compare("RADIO") == 0)
				mode = CZapitClient::MODE_RADIO;
		}
		
		bouquet = Parent->GetBouquet(atoi(request->ParameterList["bouquet"].c_str()), mode);
		CZapitClient::BouquetChannelList::iterator channel = bouquet->begin();

		for (unsigned int i = 0; channel != bouquet->end(); channel++,i++)
			request->printf("%u %u %s\n",channel->nr, channel->channel_id, channel->name);

		return true;
	}
	else
	{
		request->SocketWriteLn("error");
		return false;
	}
}

//-------------------------------------------------------------------------

bool CControlAPI::GetBouquetsCGI(CWebserverRequest *request)
{
	request->SendPlainHeader("text/plain");          // Standard httpd header senden

	for (unsigned int i = 0; i < Parent->BouquetList.size();i++)
		request->printf("%u %s\n", (Parent->BouquetList[i].bouquet_nr) + 1, Parent->BouquetList[i].name);

	return true;
}

//-------------------------------------------------------------------------

bool CControlAPI::EpgCGI(CWebserverRequest *request)
{
	CChannelEvent *event;
	
	Parent->GetChannelEvents();

	request->SendPlainHeader("text/plain");          // Standard httpd header senden

	if (request->ParameterList.size() == 0)
	{
		CZapitClient::BouquetChannelList *channellist = Parent->GetChannelList(CZapitClient::MODE_CURRENT);
		
		CZapitClient::BouquetChannelList::iterator channel = channellist->begin();

		for(; channel != channellist->end();channel++)
		{
			event = Parent->ChannelListEvents[channel->channel_id];

			if (event)
				request->printf("%u %llu %s\n",channel->channel_id,event->eventID,event->description.c_str() /*eList[n].eventID,eList[n].description.c_str()*/);
		}

		return true;
	}
	else if (request->ParameterList.size() == 1)
	{

		if (request->ParameterList["1"] == "ext")
		{
			CZapitClient::BouquetChannelList *channellist = Parent->GetChannelList(CZapitClient::MODE_CURRENT);
			CZapitClient::BouquetChannelList::iterator channel = channellist->begin();

			for(; channel != channellist->end();channel++)
			{
				event = Parent->ChannelListEvents[channel->channel_id];
				if(event)
				{
					request->printf("%u %ld %u %llu %s\n",channel->channel_id,event->startTime,event->duration,event->eventID,event->description.c_str() /*eList[n].eventID,eList[n].description.c_str()*/);
				}
			}

			return true;
		}
		else if (request->ParameterList["eventid"] != "")
		{
			//special epg query
			unsigned long long epgid;
			sscanf( request->ParameterList["eventid"].c_str(), "%llu", &epgid);
			CShortEPGData epg;
			
			if (Parent->Sectionsd->getEPGidShort(epgid,&epg))
			{
				request->SocketWriteLn(epg.title);
				request->SocketWriteLn(epg.info1);
				request->SocketWriteLn(epg.info2);
				return true;
			}
		}
		else if (request->ParameterList["onidsid"] != "")
		{
			t_channel_id channel_id = atol(request->ParameterList["onidsid"].c_str()); // FIXME
			Parent->eList = Parent->Sectionsd->getEventsServiceKey(channel_id);
			CChannelEventList::iterator eventIterator;

			for (eventIterator = Parent->eList.begin(); eventIterator != Parent->eList.end(); eventIterator++)
			{
			CShortEPGData epg;
			
				if (Parent->Sectionsd->getEPGidShort(eventIterator->eventID,&epg))
				{
					request->printf("%llu %ld %d\n", eventIterator->eventID, eventIterator->startTime, eventIterator->duration);
					if(epg.title.length() > 0)
					request->printf("%s\n",epg.title.c_str());
					if(epg.info1.length() > 0)
					request->printf("%s\n",epg.info1.c_str());
					if(epg.info2.length() > 0)
					request->printf("%s\n\n",epg.info2.c_str());
				}
			}
			return true;
		
		}
		else
		{
			//eventlist for a chan
			unsigned id = atol( request->ParameterList["1"].c_str());
			SendEventList( request, id);
			return true;
		}
	}
	
	request->SendError();
	return false;
}

//-------------------------------------------------------------------------

bool CControlAPI::VersionCGI(CWebserverRequest *request)
{
	// aktuelle cramfs version ausgeben
	request->SendPlainHeader("text/plain");          // Standard httpd header senden
	request->SendFile("/",".version");
	return true;
}

//-------------------------------------------------------------------------

bool CControlAPI::ZaptoCGI(CWebserverRequest *request)
{
	request->SendPlainHeader("text/plain");          // Standard httpd header senden

	if (request->ParameterList.size() == 0)
	{
		//paramlos - aktuelles programm anzeigen
		request->printf("%u\n", Parent->Zapit->getCurrentServiceID());
		return true;
	}
	else if (request->ParameterList.size() == 1)
	{
		if (request->ParameterList["1"] == "getpids")		// getpids !
		{
			SendcurrentVAPid(request);
			return true;
		}
		else if (request->ParameterList["1"] == "getallpids")		// getpids !
		{
			SendAllCurrentVAPid(request);
			return true;
		}
		else if (request->ParameterList["1"] == "stopplayback")
		{
			Parent->Zapit->stopPlayBack();
			Parent->Sectionsd->setPauseScanning(true);
			request->SendOk();
		}
		else if (request->ParameterList["1"] == "startplayback")
		{
			Parent->Zapit->startPlayBack();
			Parent->Sectionsd->setPauseScanning(false);
			dprintf("start playback requested..\n");
			request->SendOk();
		}
		else if (request->ParameterList["1"] == "statusplayback")
		{
			request->SocketWrite((char *) (Parent->Zapit->isPlayBackActive() ? "1" : "0")); 
			return true;
		}
		else if (request->ParameterList["1"] == "stopsectionsd")
		{
			Parent->Sectionsd->setPauseScanning(true);
			request->SendOk();
		}
		else if (request->ParameterList["1"] == "startsectionsd")
		{
			Parent->Sectionsd->setPauseScanning(false);
			request->SendOk();
		}
		else if (request->ParameterList["1"] == "statussectionsd")
		{
			request->SocketWrite((char *) (Parent->Sectionsd->getIsScanningActive() ? "1" : "0")); 
			return true;
		}
		else
		{
			const char * argument = request->ParameterList["1"].c_str();

			if ((argument[0] == '0') && (argument[1] == 'x'))
				Parent->ZapTo(&(argument[2]));
			else
				Parent->ZapTo_decimal(argument);

			request->SendOk();
		}
		
		return true;
	}
	
	request->SendError();
	return false;
}
//-------------------------------------------------------------------------

bool CControlAPI::StartPluginCGI(CWebserverRequest *request)
{
	std::string pluginname;
	if (request->ParameterList.size() == 1)
	{

		if (request->ParameterList["name"] != "")
		{
			pluginname = request->ParameterList["name"];
			request->URLDecode(pluginname);
			Parent->EventServer->sendEvent(NeutrinoMessages::EVT_START_PLUGIN, 
				CEventServer::INITID_HTTPD, 
				(void *) pluginname.c_str(), 
				pluginname.length() + 1);

			request->SendOk();
			return true;
		}
	}
	
	request->SendError();
	return false;

	
}



//-------------------------------------------------------------------------
// Send functions (for ExecuteCGI)
//-------------------------------------------------------------------------

void CControlAPI::SendEventList(CWebserverRequest *request, t_channel_id channel_id)
{
	int pos;

	Parent->eList = Parent->Sectionsd->getEventsServiceKey(channel_id);
	CChannelEventList::iterator eventIterator;

	for (eventIterator = Parent->eList.begin(); eventIterator != Parent->eList.end(); eventIterator++, pos++)
		request->printf("%llu %ld %d %s\n", eventIterator->eventID, eventIterator->startTime, eventIterator->duration, eventIterator->description.c_str());
}

//-------------------------------------------------------------------------

void CControlAPI::SendChannelList(CWebserverRequest *request)
{
	CZapitClient::BouquetChannelList *channellist = Parent->GetChannelList(CZapitClient::MODE_CURRENT);
	CZapitClient::BouquetChannelList::iterator channel = channellist->begin();
	
	for(; channel != channellist->end();channel++)
		request->printf("%u %s\n",channel->channel_id, channel->name);
};

//-------------------------------------------------------------------------

void CControlAPI::SendStreamInfo(CWebserverRequest* request)
{

	int bitInfo[10];
	Parent->GetStreamInfo(bitInfo);
	request->printf("%d\n%d\n", bitInfo[0], bitInfo[1] );	//Resolution x y
	request->printf("%d\n", bitInfo[4]*50);					//Bitrate bit/sec
	
	switch (bitInfo[2]) //format
	{
		case 2: request->SocketWrite("4:3\n"); break;
		case 3: request->SocketWrite("16:9\n"); break;
		case 4: request->SocketWrite("2.21:1\n"); break;
		default: request->SocketWrite("unknown\n"); break;
	}

	switch (bitInfo[3]) //fps
	{
		case 3: request->SocketWrite("25\n"); break;
		case 6: request->SocketWrite("50\n"); break;
		default: request->SocketWrite("unknown\n");
	}

	request->SocketWriteLn(Parent->audiotype_names[bitInfo[6]]);

}
//-------------------------------------------------------------------------

void CControlAPI::SendcurrentVAPid(CWebserverRequest* request)
{
	CZapitClient::responseGetPIDs pids;
	pids.PIDs.vpid=0;
	Parent->Zapit->getPIDS(pids);

	request->printf("%u\n", pids.PIDs.vpid);
	if(!pids.APIDs.empty())
		request->printf("%u\n", pids.APIDs[0].pid);
	else
		request->printf("0\n");
}

//-------------------------------------------------------------------------

void CControlAPI::SendAllCurrentVAPid(CWebserverRequest* request)
{
	CZapitClient::responseGetPIDs pids;
	pids.PIDs.vpid=0;
	Parent->Zapit->getPIDS(pids);

	request->printf("%u\n", pids.PIDs.vpid);
        
	for (CZapitClient::APIDList::iterator it = pids.APIDs.begin() ; 
		  it!=pids.APIDs.end(); it++)
		request->printf("%u\n", it->pid);
	if(pids.APIDs.empty())
		request->printf("0\n"); // shouldnt happen, but print at least one apid

}

//-------------------------------------------------------------------------

void CControlAPI::SendSettings(CWebserverRequest* request)
{
	request->SocketWriteLn(
		"Boxtype " +
		Parent->Dbox_Hersteller[Parent->Controld->getBoxType()] +
		"\n" 
		"videooutput " +
		Parent->videooutput_names[(unsigned char)Parent->Controld->getVideoOutput()] +
		"\n"
		"videoformat " +
		Parent->videoformat_names[(unsigned char)Parent->Controld->getVideoFormat()]
	);
}

//-------------------------------------------------------------------------

void CControlAPI::SendTimers(CWebserverRequest* request)
{
	CTimerd::TimerList timerlist;			// List of bouquets

	timerlist.clear();
	Parent->Timerd->getTimerList(timerlist);

	CZapitClient::BouquetChannelList channellist_tv;
	CZapitClient::BouquetChannelList channellist_radio;
	channellist_tv.clear();
	channellist_radio.clear();

	CTimerd::TimerList::iterator timer = timerlist.begin();
	
	for(; timer != timerlist.end();timer++)
	{
		// Add Data
		char zAddData[20+1] = { 0 };
		
		switch(timer->eventType) {
		case CTimerd::TIMER_NEXTPROGRAM:
		case CTimerd::TIMER_ZAPTO:
		case CTimerd::TIMER_RECORD:
			if (timer->mode == CTimerd::MODE_RADIO)
			{
				// Radiokanal
				if (channellist_radio.size() == 0)
					Parent->Zapit->getChannels(channellist_radio,CZapitClient::MODE_RADIO);

				CZapitClient::BouquetChannelList::iterator channel = channellist_radio.begin();
				
				for(; channel != channellist_radio.end();channel++)
				{
					if (channel->channel_id == timer->channel_id)
					{
						strncpy(zAddData, channel->name, 20);
						zAddData[20]=0;
						break;
					}
				}

				if (channel == channellist_radio.end())
					strcpy(zAddData,"Unbekannter Radiokanal");

			}
			else
			{
				//TV Kanal
				if (channellist_tv.size() == 0)
					Parent->Zapit->getChannels(channellist_tv, CZapitClient::MODE_TV);

				CZapitClient::BouquetChannelList::iterator channel = channellist_tv.begin();

				for(; channel != channellist_tv.end();channel++)
				{
					if (channel->channel_id == timer->channel_id)	
					{				
						strncpy(zAddData, channel->name, 20);						
						zAddData[20]=0;						
						break;						
					}
				}

				if (channel == channellist_tv.end())
					strcpy(zAddData,"Unbekannter TV-Kanal");

			}
			break;
			
		case CTimerd::TIMER_STANDBY:
			sprintf(zAddData,"Standby: %s",(timer->standby_on ? "ON" : "OFF"));
			break;

		case CTimerd::TIMER_REMIND :
			strncpy(zAddData, timer->message, 20);
			zAddData[20]=0;
			break;

		default:
			break;
		}

		request->printf("%d %d %d %d %d %d %s\n",
			timer->eventID,
			(int)timer->eventType,
			(int)timer->eventRepeat,
			(int)timer->announceTime,
			(int)timer->alarmTime,
			(int)timer->stopTime,
			zAddData);
	}
}

