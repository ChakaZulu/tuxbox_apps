/*
	nhttpd  -  DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: controlapi.cpp,v 1.62 2005/09/06 18:18:30 yjogol Exp $

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// system
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <cctype>
#include <dirent.h>
#include <fstream>
#include <map>

// tuxbox
#include <neutrinoMessages.h>
#include <zapit/client/zapittools.h>
#include <zapit/bouquets.h>
// nhttpd
#include "controlapi.h"
#include "lcdapi.h"
#include "debug.h"

std::map<std::string, std::string> iso639;

bool initialize_iso639_map(void)
{
	std::string s, t, u, v;
	std::ifstream in("/share/iso-codes/iso-639.tab");
	if (in.is_open())
	{
		while (in.peek() == '#')
			getline(in, s);
		while (in >> s >> t >> u >> std::ws)
		{
			getline(in, v);
			iso639[s] = v;
			if (s != t)
				iso639[t] = v;
		}
		in.close();
		return true;
	}
 	else
		return false;
}

const char * getISO639Description(const char * const iso)
{
	std::map<std::string, std::string>::const_iterator it = iso639.find(std::string(iso));
	if (it == iso639.end())
		return iso;
	else
		return it->second.c_str();
}

bool CControlAPI::Execute(CWebserverRequest* request)
{
	unsigned int operation = 0;
	const char *operations[] =
	{
		"timer","setmode","standby","getdate","gettime","settings","getservicesxml",
		"getbouquetsxml","getonidsid","message","info","shutdown","volume",
		"channellist","getbouquet","getbouquets","epg","version","zapto", "startplugin",
		"getmode","exec","system","rc","lcd","yweb",NULL
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
	case 21:
	        return ExecCGI(request);
	case 22:
	        return SystemCGI(request);
	case 23:
	        return RCCGI(request);
	case 24:
	        return LCDAction(request);
	case 25:
	        return YWebCGI(request);
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
		if (!(request->ParameterList.empty()))
		{
			if (request->ParameterList["action"] == "new")
			{
				Parent->WebAPI->doNewTimer(request);
				request->SendOk();
			}
			else if (request->ParameterList["action"] == "modify")
			{
				Parent->WebAPI->doModifyTimer(request);
				request->SendOk();
			}
			else if (request->ParameterList["action"] == "remove")
			{
				unsigned removeId = atoi(request->ParameterList["id"].c_str());
				Parent->Timerd->removeTimerEvent(removeId);
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

	if (!(request->ParameterList.empty()))
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

static const unsigned int PLUGIN_DIR_COUNT = 5;
static std::string PLUGIN_DIRS[PLUGIN_DIR_COUNT] = {
	"/share/tuxbox/neutrino/httpd/scripts", //dummy: override by WebServer->PublicDocumentRoot+/scripts
	"/var/httpd/scripts", //dummy: override by WebServer->PriveDocumentRoot +/scripts
	"/var/tuxbox/plugins",
	PLUGINDIR,
	"/mnt/plugins",
};

bool CControlAPI::ExecCGI(CWebserverRequest *request)
{
	bool res = false;
	std::string script;

	// ToDo: There is no Contructor/init until now
	if(Parent != NULL)
		if(Parent->Parent != NULL)
		{
			PLUGIN_DIRS[0]=Parent->Parent->PublicDocumentRoot;
			PLUGIN_DIRS[0].append("/scripts");
			PLUGIN_DIRS[1]=Parent->Parent->PrivateDocumentRoot;
			PLUGIN_DIRS[1].append("/scripts");
		}
	if (request->ParameterList.size() > 1)
		request->SendPlainHeader("text/html");          // Standard httpd header senden MIME html
	else
		request->SendPlainHeader("text/plain");          // Standard httpd header senden
	if (request->ParameterList.size() > 0)
	{
		script = request->ParameterList["1"];
		std::string::size_type i = script.find_last_of("/");
		if (i != std::string::npos)
			script = script.substr(i+1);
		script += ".sh";
		for (unsigned int i=0;i<PLUGIN_DIR_COUNT;i++) {
			DIR *scriptdir = opendir(PLUGIN_DIRS[i].c_str());
			if (scriptdir != NULL)
			{
				struct dirent *scriptfile = NULL;
				while ((scriptfile = readdir(scriptdir)) != NULL)
				{
					if (strcmp(script.c_str(),scriptfile->d_name) == 0)
						break;
				}
				if (scriptfile != NULL)
				{
					// script was found
					std::string abscmd(PLUGIN_DIRS[i].c_str());
					abscmd += "/";
					abscmd += script;

					for(unsigned int y=2;y<=request->ParameterList.size();y++)
					{
						char number_buf[20];
				 		sprintf(number_buf, "%d", y);
						abscmd += " ";
						abscmd += (request->ParameterList[number_buf]).c_str();
					}
					printf("[CControlAPI] executingY %s\n",abscmd.c_str());
					FILE *f = popen(abscmd.c_str(),"r");
					if (f != NULL)
					{
						char output[1024];
						while (fgets(output,1024,f))
						{
							request->SocketWrite(output);
						}
						pclose(f);
						res = true;
						break;
					}
					else
					{
						printf("[CControlAPI] can't open %s\n",abscmd.c_str());
					}
				}
				closedir(scriptdir);
			}
			else
			{
				printf("[CControlAPI] could not open: %s\n",PLUGIN_DIRS[i].c_str());
			}
		}
		if (!res)
		{
			printf("[CControlAPI] script %s not found in\n",script.c_str());
			for (unsigned int i=0;i<PLUGIN_DIR_COUNT;i++) {
				printf("%s\n",PLUGIN_DIRS[i].c_str());
			}
		}
	}
	else
	{
		printf("[CControlAPI] no script given\n");
	}
	if (!res)
	{
		request->Send404Error();
	}
	return res;
}

//-------------------------------------------------------------------------

bool CControlAPI::SystemCGI(CWebserverRequest *request)
{
	std::string pluginname;
	request->SendPlainHeader("text/plain");          // Standard httpd header senden
	if (request->ParameterList.size() == 1)
	{

		if (request->ParameterList["1"] == "getAViAExtIec")
		{
			request->printf("%d\n", Parent->Zapit->IecState());
			return true;
		}
		if (request->ParameterList["setAViAExtIec"] == "on")
		{
			Parent->Zapit->IecOn();
			request->SendOk();
			return true;
		}
		if (request->ParameterList["setAViAExtIec"] == "off")
		{
			Parent->Zapit->IecOff();
			request->SendOk();
			return true;
		}
		if (request->ParameterList["1"] == "getAViAExtPlayBack")
		{
			request->printf("%d\n", Parent->Zapit->PlaybackState());
			return true;
		}
		if (request->ParameterList["setAViAExtPlayBack"] == "pes")
		{
			Parent->Zapit->PlaybackPES();
			request->SendOk();
			return true;
		}
		if (request->ParameterList["setAViAExtPlayBack"] == "spts")
		{
			Parent->Zapit->PlaybackSPTS();
			request->SendOk();
			return true;
		}
	}

	request->SendError();
	return false;

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
bool CControlAPI::RCCGI(CWebserverRequest *request)
{
	request->SendPlainHeader("text/plain");          // Standard httpd header senden

	if (request->ParameterList.size() == 1)
	{
		if (request->ParameterList["1"] == "lock")	// lock remote control
			Parent->EventServer->sendEvent(NeutrinoMessages::LOCK_RC, CEventServer::INITID_HTTPD);
		else if (request->ParameterList["1"] == "unlock")	// unlock remote control
			Parent->EventServer->sendEvent(NeutrinoMessages::UNLOCK_RC, CEventServer::INITID_HTTPD);

	}

	request->SendOk();
	return true;
}

//-------------------------------------------------------------------------

bool CControlAPI::GetDateCGI(CWebserverRequest *request)
{
	request->SendPlainHeader("text/plain");          // Standard httpd header senden

	if (request->ParameterList.empty())
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

	if (request->ParameterList.empty())
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
	CZapitClient::CCurrentServiceInfo current_pids = Parent->Zapit->getCurrentServiceInfo();
	request->SendPlainHeader("text/plain");
	request->printf("%x%04x%04x\n",current_pids.tsid, current_pids.onid, current_pids.sid);
	return true;
}

//-------------------------------------------------------------------------

bool CControlAPI::MessageCGI(CWebserverRequest *request)
{
	request->SendPlainHeader("text/plain");          // Standard httpd header senden
	std::string message;
	int event = 0;

	if (request->ParameterList.empty())
	{	//paramlos
		request->SendError();
		return false;
	}

	if (!(request->ParameterList["popup"].empty()))
	{
		message = request->ParameterList["popup"];
		event = NeutrinoMessages::EVT_POPUP;
	}
	else if (!(request->ParameterList["nmsg"].empty()))
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
	if (request->ParameterList.empty())
	{
		//paramlos
		request->SocketWrite("Neutrino\n");
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
			request->SocketWrite("3");
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

	if (request->ParameterList.empty())
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

	if (request->ParameterList.empty())
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
	CZapitClient::BouquetList blist;
	request->SendPlainHeader("text/plain");          // Standard httpd header senden

	if (!(request->ParameterList.empty()))
	{
		int mode = CZapitClient::MODE_CURRENT;

		if (!(request->ParameterList["mode"].empty()))
		{
			if (request->ParameterList["mode"].compare("TV") == 0)
				mode = CZapitClient::MODE_TV;
			if (request->ParameterList["mode"].compare("RADIO") == 0)
				mode = CZapitClient::MODE_RADIO;
		}

		// Get Bouquet Number. First matching current channel
		if (request->ParameterList["1"] == "actual")
		{
			int actual=0;
			//easier?
			for (unsigned int i = 0; i < Parent->BouquetList.size() && actual == 0;i++)
			{
				//request->printf("%u %s\n", (Parent->BouquetList[i].bouquet_nr) + 1, Parent->BouquetList[i].name);
				bouquet = Parent->GetBouquet((Parent->BouquetList[i].bouquet_nr) + 1, mode);
				CZapitClient::BouquetChannelList::iterator channel = bouquet->begin();
				for (unsigned int j = 0; channel != bouquet->end() && actual == 0; channel++,j++)
				{
					if(channel->channel_id == Parent->Zapit->getCurrentServiceID())
						actual=i+1;
				}
			}
			request->printf("%d",actual);
			return true;
		}
		else
		{
			bouquet = Parent->GetBouquet(atoi(request->ParameterList["bouquet"].c_str()), mode);
			CZapitClient::BouquetChannelList::iterator channel = bouquet->begin();

			for (unsigned int i = 0; channel != bouquet->end(); channel++,i++)
				request->printf("%u "
					PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
					" %s\n",
					channel->nr,
					channel->channel_id,
					channel->name);
			return true;
		}
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

	if (request->ParameterList.empty())
	{
		CZapitClient::BouquetChannelList *channellist = Parent->GetChannelList(CZapitClient::MODE_CURRENT);

		CZapitClient::BouquetChannelList::iterator channel = channellist->begin();

		for(; channel != channellist->end();channel++)
		{
			event = Parent->ChannelListEvents[channel->channel_id];

			if (event)
				request->printf(PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
						" %llu %s\n",
						channel->channel_id,
						event->eventID,
						event->description.c_str() /*eList[n].eventID,eList[n].description.c_str()*/);
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
					request->printf(PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
							" %ld %u %llu %s\n",
							channel->channel_id,
							event->startTime,
							event->duration,
							event->eventID,
							event->description.c_str() /*eList[n].eventID,eList[n].description.c_str()*/);
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
		else if (request->ParameterList["eventid2fsk"] != "")
		{
			if (request->ParameterList["starttime"] != "")
			{
				unsigned long long epgid;
				time_t starttime;
				sscanf( request->ParameterList["fskid"].c_str(), "%llu", &epgid);
				sscanf( request->ParameterList["starttime"].c_str(), "%lu", &starttime);
				CEPGData longepg;
				if(Parent->Sectionsd->getEPGid(epgid, starttime, &longepg))
				{
					request->printf("%u\n", longepg.fsk);
					return true;
				}
			}
			request->SendError();
			return false;
		}
		else if (!(request->ParameterList["id"].empty()))
		{
			t_channel_id channel_id;
			sscanf(request->ParameterList["id"].c_str(),
			       SCANF_CHANNEL_ID_TYPE,
			       &channel_id);

			Parent->eList = Parent->Sectionsd->getEventsServiceKey(channel_id);
			CChannelEventList::iterator eventIterator;

			for (eventIterator = Parent->eList.begin(); eventIterator != Parent->eList.end(); eventIterator++)
			{
			CShortEPGData epg;

				if (Parent->Sectionsd->getEPGidShort(eventIterator->eventID,&epg))
				{
					request->printf("%llu %ld %d\n", eventIterator->eventID, eventIterator->startTime, eventIterator->duration);
					request->printf("%s\n",epg.title.c_str());
					request->printf("%s\n",epg.info1.c_str());
					request->printf("%s\n\n",epg.info2.c_str());
				}
			}
			return true;

		}
		else
		{
			//eventlist for a chan
			t_channel_id channel_id;
			sscanf(request->ParameterList["1"].c_str(),
			       SCANF_CHANNEL_ID_TYPE,
			       &channel_id);

			SendEventList(request, channel_id);

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

	if (request->ParameterList.empty())
	{
		request->printf(PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
				"\n",
				Parent->Zapit->getCurrentServiceID());
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
			Parent->ZapTo(request->ParameterList["1"].c_str());

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

bool CControlAPI::LCDAction(CWebserverRequest *request)
{
	int tval;
	int error=0;
	int xpos=10, ypos=10, size=12, color=1, font=0;
	int x1,y1,x2,y2,coll,colf;

	request->SendPlainHeader("text/plain");          // Standard httpd header senden

	if (request->ParameterList.empty())
	{	//paramlos
		request->SendError();
		return false;
	}

	if (request->ParameterList["lock"] != "")
	{
		if(sscanf( request->ParameterList["lock"].c_str(), "%d", &tval))
		{
			Parent->LcdAPI->LockDisplay(tval);
		}
		else
		{
			error=1;
		}
	}
	if (request->ParameterList["clear"] != "")
	{
		if(sscanf( request->ParameterList["clear"].c_str(), "%d", &tval))
		{
			if(tval)
			{
				Parent->LcdAPI->Clear();
			}
		}
		else
		{
			error=1;
		}
	}
	if (request->ParameterList["png"] != "")
	{
		if(! Parent->LcdAPI->ShowPng((char*)request->ParameterList["png"].c_str()))
		{
			error=1;
		}
	}
	if (request->ParameterList["raw"] != "")
	{
		char *sptr=strdup((char*)request->ParameterList["raw"].c_str()),*pptr;
		int loop=4;

		pptr=sptr;
		error=1;
		if(sscanf(pptr, "%d,%d,%d,%d",&x1,&y1,&x2,&y2)==4)
		{
			while(loop-- && ((pptr=strchr(pptr,','))!=NULL))
			{
				++pptr;
			}
			if(pptr)
			{
				Parent->LcdAPI->ShowRaw(x1,y1,x2,y2,pptr);
				error=0;
			}
		}
		if(sptr)
		{
			free(sptr);
		}
	}
	if (request->ParameterList["line"] != "")
	{
		if(sscanf( request->ParameterList["line"].c_str(), "%d,%d,%d,%d,%d",&x1,&y1,&x2,&y2,&coll)==5)
		{
			Parent->LcdAPI->DrawLine(x1,y1,x2,y2,coll);
		}
		else
		{
			error=1;
		}
	}
	if (request->ParameterList["rect"] != "")
	{
		if(sscanf( request->ParameterList["rect"].c_str(), "%d,%d,%d,%d,%d,%d",&x1,&y1,&x2,&y2,&coll,&colf)==6)
		{
			Parent->LcdAPI->DrawRect(x1,y1,x2,y2,coll,colf);
		}
		else
		{
			error=1;
		}
	}
	if (request->ParameterList["xpos"] != "")
	{
		if(sscanf( request->ParameterList["xpos"].c_str(), "%d", &tval))
		{
			xpos=tval;
		}
		else
		{
			error=1;
		}
	}
	if (request->ParameterList["ypos"] != "")
	{
		if(sscanf( request->ParameterList["ypos"].c_str(), "%d", &tval))
		{
			ypos=tval;
		}
		else
		{
			error=1;
		}
	}
	if (request->ParameterList["size"] != "")
	{
		if(sscanf( request->ParameterList["size"].c_str(), "%d", &tval))
		{
			size=tval;
		}
		else
		{
			error=1;
		}
	}
	if (request->ParameterList["color"] != "")
	{
		if(sscanf( request->ParameterList["color"].c_str(), "%d", &tval))
		{
			color=tval;
		}
		else
		{
			error=1;
		}
	}
	if (request->ParameterList["font"] != "")
	{
		if(sscanf( request->ParameterList["font"].c_str(), "%d", &tval) && tval>=0 && tval<3)
		{
			font=tval;
		}
		else
		{
			error=1;
		}
	}
	if (request->ParameterList["text"] != "")
	{
		Parent->LcdAPI->DrawText(xpos, ypos, size, color, font, (char*)request->ParameterList["text"].c_str());
	}
	if (request->ParameterList["update"] != "")
	{
		if(sscanf( request->ParameterList["update"].c_str(), "%d", &tval))
		{
			if(tval)
			{
				Parent->LcdAPI->Update();
			}
		}
		else
		{
			error=1;
		}
	}
	if(error)
	{
		request->SendError();
		return false;
	}

	request->SendOk();
	return true;
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
		request->printf(PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
				" %s\n",
				channel->channel_id,
				channel->name);
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
	static bool init_iso=true;
	if(init_iso)
	{
		if(initialize_iso639_map())
			init_iso=false;
	}
	bool eit_not_ok=true;
	CZapitClient::responseGetPIDs pids;

	CSectionsdClient::ComponentTagList tags;
	pids.PIDs.vpid=0;
	Parent->Zapit->getPIDS(pids);

	request->printf("%05u\n", pids.PIDs.vpid);

	t_channel_id current_channel = Parent->Zapit->getCurrentServiceID();
	CSectionsdClient::responseGetCurrentNextInfoChannelID currentNextInfo;
	Parent->Sectionsd->getCurrentNextServiceKey(current_channel, currentNextInfo);
	if (Parent->Sectionsd->getComponentTagsUniqueKey(currentNextInfo.current_uniqueKey,tags))
	{
		for (unsigned int i=0; i< tags.size(); i++)
		{
			for (unsigned short j=0; j< pids.APIDs.size(); j++)
			{
				if ( pids.APIDs[j].component_tag == tags[i].componentTag )
				{
 					if(!tags[i].component.empty())
					{
						if(!(isalnum(tags[i].component[0])))
							tags[i].component=tags[i].component.substr(1,tags[i].component.length()-1);
						request->printf("%05u %s\n",pids.APIDs[j].pid,tags[i].component.c_str());
					}
					else
					{
						if(!(init_iso))
						{
							strcpy( pids.APIDs[j].desc, getISO639Description( pids.APIDs[j].desc ) );
						}
			 			request->printf("%05u %s %s\n",pids.APIDs[j].pid,pids.APIDs[j].desc,pids.APIDs[j].is_ac3 ? " (AC3)": " ");
					}
					eit_not_ok=false;
					break;
				}
			}
		}
	}
	if(eit_not_ok)
	{
		unsigned short i = 0;
		for (CZapitClient::APIDList::iterator it = pids.APIDs.begin(); it!=pids.APIDs.end(); it++)
		{
			if(!(init_iso))
			{
				strcpy( pids.APIDs[i].desc, getISO639Description( pids.APIDs[i].desc ) );
			}
 			request->printf("%05u %s %s\n",it->pid,pids.APIDs[i].desc,pids.APIDs[i].is_ac3 ? " (AC3)": " ");
			i++;
		}
	}

	if(pids.APIDs.empty())
		request->printf("0\n"); // shouldnt happen, but print at least one apid
	if(pids.PIDs.vtxtpid)
		request->printf("%05u vtxt\n",pids.PIDs.vtxtpid);
	if (pids.PIDs.pmtpid)
		request->printf("%05u pmt\n",pids.PIDs.pmtpid);

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

	CTimerd::TimerList::iterator timer = timerlist.begin();

	for(; timer != timerlist.end();timer++)
	{
		// Add Data
		char zAddData[22+1] = { 0 };

		switch(timer->eventType) {
		case CTimerd::TIMER_NEXTPROGRAM:
		case CTimerd::TIMER_ZAPTO:
		case CTimerd::TIMER_RECORD:
			strncpy(zAddData, Parent->Zapit->getChannelName(timer->channel_id).c_str(), 22);
			zAddData[22]=0;

			if (zAddData[0] == 0)
				strcpy(zAddData, Parent->Zapit->isChannelTVChannel(timer->channel_id) ? "Unbekannter TV-Kanal" : "Unbekannter Radiokanal");
			break;

		case CTimerd::TIMER_STANDBY:
			sprintf(zAddData,"Standby: %s",(timer->standby_on ? "ON" : "OFF"));
			break;

		case CTimerd::TIMER_REMIND :
			strncpy(zAddData, timer->message, 22);
			zAddData[22]=0;
			break;

		default:
			break;
		}

		request->printf("%d %d %d %d %d %d %d %s\n",
				timer->eventID,
				(int)timer->eventType,
				(int)timer->eventRepeat,
				(int)timer->repeatCount,
				(int)timer->announceTime,
				(int)timer->alarmTime,
				(int)timer->stopTime,
				zAddData);
	}
}

//-------------------------------------------------------------------------
// yweb : Extentions
//-------------------------------------------------------------------------

// Dispatcher
bool CControlAPI::YWebCGI(CWebserverRequest *request)
{
	bool status=true;
	int para;
	request->SendPlainHeader("text/plain");          // Standard httpd header senden
	if (request->ParameterList["1"] == "nhttpd_version")
	{
		request->printf("%s\n", NHTTPD_VERSION);
	}
	else if (request->ParameterList["video_stream_pids"] != "")
	{
		para=0;
		sscanf( request->ParameterList["video_stream_pids"].c_str(), "%d", &para);
		YWeb_SendVideoStreamingPids(request, para);
	}
	else if (request->ParameterList["1"] == "radio_stream_pid")
	{
		YWeb_SendRadioStreamingPid(request);
	}

	if(!status)
		request->SendError();

	return status;
}

//-------------------------------------------------------------------------
// Get Streaming Pids 0x$pmt,0x$vpid,0x$apid with apid_no is the Number of Audio-Pid
void CControlAPI::YWeb_SendVideoStreamingPids(CWebserverRequest* request, int apid_no)
{
	CZapitClient::responseGetPIDs pids;
	int apid=0,apid_idx=0;
	pids.PIDs.vpid=0;
	Parent->Zapit->getPIDS(pids);

	if( apid_no < (int)pids.APIDs.size())
		apid_idx=apid_no;
	if(!pids.APIDs.empty())
		apid = pids.APIDs[apid_idx].pid;
	request->printf("0x%04x,0x%04x,0x%04x",pids.PIDs.pmtpid,pids.PIDs.vpid,apid);
}

//-------------------------------------------------------------------------
// Get Streaming Pids 0x$pmt,0x$vpid,0x$apid with apid_no is the Number of Audio-Pid
void CControlAPI::YWeb_SendRadioStreamingPid(CWebserverRequest* request)
{
	CZapitClient::responseGetPIDs pids;
	int apid=0;
	Parent->Zapit->getPIDS(pids);

	if(!pids.APIDs.empty())
		apid = pids.APIDs[0].pid;
	request->printf("0x%04x",apid);
}
