/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: webdbox.cpp,v 1.27 2002/05/12 20:29:14 dirch Exp $

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
	if(sidonid == zapit.getCurrentServiceID())
	{
		if(Parent->DEBUG) printf("Kanal ist aktuell\n");
		return;
	}
	zapit.zapTo_serviceID(sidonid);
	sectionsd.setServiceChanged(sidonid,false);

}
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
// Konstruktor und destruktor
//-------------------------------------------------------------------------

TWebDbox::TWebDbox(CWebserver *server)
{
	Parent=server;
	standby_mode=false;
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

}
//-------------------------------------------------------------------------

TWebDbox::~TWebDbox()
{
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
	eList = sectionsd.getChannelEvents();
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
// ExecuteCGI und Execute
//-------------------------------------------------------------------------
/*
bool TWebDbox::ExecuteCGI(CWebserverRequest* request)
{
	if(Parent->DEBUG) printf("Execute CGI : %s\n",request->Filename.c_str());

	if(request->Filename.compare("addtimer") == 0)			// timer hinzufügen
	{
		request->SendPlainHeader();          // Standard httpd header senden
		int min = 0,hour = 0,day = 0,month = 0;
		if(request->ParameterList["min"] != "")
			min = atoi(request->ParameterList["min"].c_str());
		if(request->ParameterList["hour"] != "")
			hour = atoi(request->ParameterList["hour"].c_str());
		if(request->ParameterList["day"] != "")
			day = atoi(request->ParameterList["day"].c_str());
		if(request->ParameterList["month"] != "")
			month = atoi(request->ParameterList["month"].c_str());
		
		if(timerd.isTimerdAvailable())
		{
			int eventid = timerd.addTimerEvent(CTimerdClient::TIMER_SHUTDOWN,NULL,min,hour,day,month);
			printf("Timerdevent: %d\n",eventid);
		}
		else
			printf("[nhttpd] Timerd nicht verfügbar\n");
	}
	if(request->Filename.compare("setmode") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if(request->ParameterList.size() > 0)
		{
			if(request->ParameterList["1"] == "status")
			{
				if(zapit.isRecordModeActive())
					request->SocketWriteLn("on");
				else
					request->SocketWriteLn("off");
				return true;
			}
			
			if (request->ParameterList["1"] == "radio")	// in radio mode schalten
			{
				zapit.setMode(CZapitClient::MODE_RADIO);
				sleep(1);
				UpdateBouquets();
			}
			else if (request->ParameterList["1"] == "tv")	// in tv mode schalten
			{
				zapit.setMode(CZapitClient::MODE_TV);
				sleep(1);
				UpdateBouquets();
			}
			else if (request->ParameterList["record"] == "start")	// record mode starten
			{
				if(request->ParameterList["stopplayback"] == "true")
					zapit.stopPlayBack();
				sectionsd.setPauseScanning(true);
				zapit.setRecordMode(true);
			}
			else if (request->ParameterList["record"] == "stop")	// recordmode beenden
			{
				zapit.setRecordMode(false);
				sectionsd.setPauseScanning(false);
				zapit.startPlayBack();
			}
			request->SocketWriteLn("ok");
			return true;
		}
		else
		{
			request->SocketWriteLn("error");
			return false;
		}

	}
	if(request->Filename.compare("standby") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList.size() == 1)
		{
			if (request->ParameterList["1"] == "on")	// in standby mode schalten
			{
				Parent->EventServer.sendEvent(NeutrinoMessages::STANDBY_ON, CEventServer::INITID_NHTTPD);
				standby_mode = true;
			}
			if (request->ParameterList["1"] == "off")	// standby mode ausschalten
			{
				Parent->EventServer.sendEvent(NeutrinoMessages::STANDBY_OFF, CEventServer::INITID_NHTTPD);
				standby_mode = false;
			}
		}
		request->SocketWrite("ok");
		return true;
	}

	if(request->Filename.compare("getdate") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList.size()==0)
		{	//paramlos
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
			request->SocketWrite("error");
			return false;
		}
	}

	if(request->Filename.compare("gettime") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList.size()==0)
		{	//paramlos
			char *timestr = new char[50];
			time_t jetzt = time(NULL);
			struct tm *tm = localtime(&jetzt);
			strftime(timestr, 50, "%H:%M:%S\n", tm );	// aktuelles datum ausgeben
			request->SocketWrite(timestr);
			delete[] timestr;
			return true;
		}
		else
		{
			request->SocketWrite("error");
			return false;
		}
	}

	if(request->Filename.compare("settings") == 0)		// sendet die settings
	{
		request->SendPlainHeader();
		SendSettings(request);
		return true;
	}

	if(request->Filename.compare("getservicesxml") == 0)		// sendet die datei services.xml
	{
		request->SendPlainHeader();
		request->SendFile("/var/tuxbox/config/zapit","services.xml");
		return true;
	}

	if(request->Filename.compare("getbouquetsxml") == 0)		// sendet die datei bouquets.xml
	{
		request->SendPlainHeader();
		request->SendFile("/var/tuxbox/config/zapit","bouquets.xml");
		return true;
	}

	if(request->Filename.compare("getonidsid") == 0)		// sendet die aktuelle onidsid
	{
		request->SendPlainHeader();
		char buf[10];
		sprintf(buf, "%u\n", zapit.getCurrentServiceID());
		request->SocketWrite(buf);
		return true;
	}


	if(request->Filename.compare("info") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList.size() == 0)
		{	//paramlos
			request->SocketWrite("Neutrino NG\n");
			return true;
		}
		else
		{
			if (request->ParameterList["1"] == "streaminfo")	// streminfo ausgeben
			{
				SendStreamInfo(request);
				return true;
			}
			else if (request->ParameterList["1"] == "settings")	// settings ausgeben
			{
				SendSettings(request);
				return true;
			}
			else if (request->ParameterList["1"] == "version")	// verision ausgeben
			{
				request->SendFile("/",".version");
				return true;
			}
			else
			{
				request->SocketWrite("error");
				return false;
			}
		}
	}

	if(request->Filename.compare("shutdown") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList.size() == 0)
		{	//paramlos
			Parent->EventServer.sendEvent(NeutrinoMessages::SHUTDOWN, CEventServer::INITID_NHTTPD);

			request->SocketWrite("ok");
			return true;
		}
		else
		{
			request->SocketWrite("error");
			return false;
		}
	}

	if(request->Filename.compare("volume") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList.size() == 0)
		{	//paramlos - aktuelles volume anzeigen
			char buf[10];
			sprintf(buf, "%d", controld.getVolume());			// volume ausgeben
			request->SocketWrite(buf);
			return true;
		}
		else
		if (request->ParameterList.size() == 1)
		{
			if(request->ParameterList["mute"] != "")
			{
				controld.setMute(true);
				request->SocketWrite("ok");					// muten
				return true;
			}
			else
			if(request->ParameterList["unmute"] != "")
			{
				controld.setMute(false);
				request->SocketWrite("ok");					// unmuten
				return true;
			}
			else
			if(request->ParameterList["status"] != "")
			{
				request->SocketWrite( (char *) (controld.getMute()?"1":"0") );	//  mute
				return true;
			}
			else
			{	//set volume
				char vol = atol( request->ParameterList[0].c_str() );
				request->SocketWrite((char*) request->ParameterList[0].c_str() );
				controld.setVolume(vol);
				request->SocketWrite("ok");
				return true;
			}
		}
		else
		{
			request->SocketWrite("error");
			return false;
		}
	}

	if(request->Filename.compare("channellist") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		SendChannelList(request);
		return true;
	}

	if(request->Filename.compare("getbouquet") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if(request->ParameterList.size() > 0)
		{
			SendBouquet(request,atoi(request->ParameterList["1"].c_str()));
			return true;		
		}
		request->SocketWriteLn("error");
		return false;
	}

	if(request->Filename.compare("getbouquets") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		SendBouquets(request);
		return true;
	}

	if(request->Filename.compare("epg") == 0)
	{
		GetChannelEvents();
		request->SendPlainHeader();          // Standard httpd header senden
		if(request->ParameterList.size() == 0)
		{
			char buffer[255];
			for(int i = 0; i < ChannelList.size();i++)
			{
				if(ChannelListEvents[ChannelList[i].onid_sid])
				{
					sprintf(buffer,"%u %llu %s\n",ChannelList[i].onid_sid,ChannelListEvents[ChannelList[i].onid_sid]->eventID,ChannelListEvents[ChannelList[i].onid_sid]->description.c_str());
					request->SocketWrite(buffer);
				}
			}
		}
		else if(request->ParameterList.size() == 1)
		{

			if(request->ParameterList["1"] == "ext")
			{
				char buffer[255];
				for(int i = 0; i < ChannelList.size();i++)
				{
					if(ChannelListEvents[ChannelList[i].onid_sid])
					{
						sprintf(buffer,"%u %ld %ld %llu %s\n",ChannelList[i].onid_sid,ChannelListEvents[ChannelList[i].onid_sid]->startTime,ChannelListEvents[ChannelList[i].onid_sid]->duration,ChannelListEvents[ChannelList[i].onid_sid]->eventID,ChannelListEvents[ChannelList[i].onid_sid]->description.c_str());
						request->SocketWrite(buffer);
					}
				}
			}
			else if(request->ParameterList["eventid"] != "")
			{	//special epg query
				unsigned long long epgid;
				sscanf( request->ParameterList["eventid"].c_str(), "%llx", &epgid);
				CShortEPGData epg;
				if(sectionsd.getEPGidShort(epgid,&epg))
				{
					request->SocketWriteLn((char*)epg.title.c_str());
					request->SocketWriteLn((char*)epg.info1.c_str());
					request->SocketWriteLn((char*)epg.info2.c_str());
				}
			}
			else
			{	//eventlist for a chan
				unsigned id = atol( request->ParameterList["1"].c_str());
				SendEventList( request, id);
			}

		}
	}

	if(request->Filename.compare("version") == 0)
	{
		// aktuelle cramfs version ausgeben
		request->SendPlainHeader();          // Standard httpd header senden
		request->SendFile("/",".version");
	}

	if(request->Filename.compare("zapto") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList.size() == 0)
		{	//paramlos - aktuelles programm anzeigen
			if(Parent->DEBUG) printf("zapto ohne params\n");
			char buf[10];
			sprintf(buf, "%u\n", zapit.getCurrentServiceID());
			request->SocketWrite(buf);
		}
		else
		if (request->ParameterList.size() == 1)
		{
			if(request->ParameterList["mode"] != "")			// TV oder RADIO - Mode
			{
				if(request->ParameterList["mode"] == "TV")
				{
					zapit.setMode(CZapitClient::MODE_RADIO);
					sleep(1);
					UpdateBouquets();
				}
				else if(request->ParameterList["mode"] == "RADIO")
				{
					zapit.setMode(CZapitClient::MODE_RADIO);
					sleep(1);
					UpdateBouquets();
				}
				else
					return false;
			}
			if(request->ParameterList["1"] == "getpids")		// getpids !
			{
				SendcurrentVAPid(request);
				return true;
			}
			else if(request->ParameterList["1"] == "stopplayback")
			{
				zapit.stopPlayBack();
				sectionsd.setPauseScanning(true);
				request->SocketWrite("ok");
			}
			else if(request->ParameterList["1"] == "startplayback")
			{
				zapit.startPlayBack();
				sectionsd.setPauseScanning(false);
				if(Parent->VERBOSE) printf("start playback requested..\n");
				request->SocketWrite("ok");
			}
			else if(request->ParameterList["1"] == "stopsectionsd")
			{
				sectionsd.setPauseScanning(true);
				request->SocketWrite("ok");
			}
			else if(request->ParameterList["1"] == "startsectionsd")
			{
				sectionsd.setPauseScanning(false);
				request->SocketWrite("ok");
			}
			else
			{
				ZapTo(request->ParameterList["1"]);
				request->SocketWrite("ok");
			}
		}
		else
		{
			request->SocketWrite("error");
			printf("[nhttpd] zapto fehler\n");
		}
	}
	return true;
}

//-------------------------------------------------------------------------
*/
/*
bool TWebDbox::Execute(CWebserverRequest* request)
{
	if(Parent->DEBUG) printf("Executing %s\n",request->Filename.c_str());
	if(request->Filename.compare("test.dbox2") == 0)
	{
		printf("Teste nun\n");
		return true;
	}
	if(request->Filename.compare("info.dbox2") == 0)
	{
		request->SendPlainHeader("text/html");		
		ShowCurrentStreamInfo(request);
	}

	else if(request->Filename.compare("services.xml") == 0)		// sendet die datei services.xml
	{
		request->SendPlainHeader("text/xml");
		request->SendFile("/var/tuxbox/config/zapit","services.xml");
		request->HttpStatus = 200;
		return true;
	}

	else if(request->Filename.compare("bouquets.xml") == 0)		// sendet die datei bouquets.xml
	{
		request->SendPlainHeader("text/xml");
		request->SendFile("/var/tuxbox/config/zapit","bouquets.xml");
		request->HttpStatus = 200;
		return true;
	}

	else if (request->Filename.compare("bouquetlist.dbox2") == 0)
	{
		request->SendPlainHeader("text/html");
		ShowBouquets(request);
		return true;
	}

	else if (request->Filename.compare("channellist.dbox2") == 0)
	{
		request->SendPlainHeader("text/html");
		if( (request->ParameterList.size() == 1) && ( request->ParameterList["bouquet"] != "") )
		{
			ShowBouquet(request,atoi(request->ParameterList["bouquet"].c_str()));
		}
		else
			ShowBouquet(request);
		return true;
	}
	else if (request->Filename.compare("controlpanel.dbox2") == 0)
	{	
		ShowControlpanel(request);					//funktionen für controlpanel links
		return true;
	}
	else if (request->Filename.compare("epg.dbox2") == 0)
	{
		if(request->ParameterList.size() > 0)
		{											// wenn parameter vorhanden sind
			request->SendHTMLHeader("EPG");

			request->SocketWriteLn("<html>\n<head><title>DBOX2-Neutrino EPG</title><link rel=\"stylesheet\" type=\"text/css\" href=\"../channellist.css\"></head>");
			request->SocketWriteLn("<body>");

			if(request->ParameterList["eventid"] != "")
			{
				unsigned long long epgid;
				sscanf(request->ParameterList["eventid"].c_str(), "%llx", &epgid);
				CShortEPGData epg;
				if(sectionsd.getEPGidShort(epgid,&epg))
				{
					char *buffer2 = new char[1024];
					sprintf(buffer2,"<H1 class=\"epg\">%s</H1><BR>\n<H2 class=\"epg\">%s</H2><BR>\n<B>%s</B><BR>\n\0",epg.title.c_str(),epg.info1.c_str(),epg.info2.c_str());
					request->SocketWrite(buffer2);
					delete[] buffer2;
				}

			}
			else if(request->ParameterList["epgid"] != "")
			{
				unsigned long long epgid;
				time_t startzeit;

				const char * idstr = request->ParameterList["epgid"].c_str();
				sscanf(idstr, "%llx", &epgid);

				const char * timestr = request->ParameterList["startzeit"].c_str();
				sscanf(timestr, "%x", &startzeit);

				CEPGData epg;
				if(sectionsd.getEPGid(epgid,startzeit,&epg))
				{
					char *buffer2 = new char[1024];
					sprintf(buffer2,"<H1 class=\"epg\">%s</H1><BR>\n<H2 class=\"epg\">%s</H2><BR>\n<B>%s</B><BR>\n\0",epg.title.c_str(),epg.info1.c_str(),epg.info2.c_str());
					request->SocketWrite(buffer2);
					delete[] buffer2;
				}

			}
			else
				printf("[nhttpd] Get epgid error\n");
			request->SendHTMLFooter();
			return true;
		}
	}
	else if (request->Filename.compare("switch.dbox2") == 0)
	{
		if(request->ParameterList.size() > 0)
		{

			if(request->ParameterList["zapto"] != "")
			{
				string t;
				if(!Authenticate(request))
				{
					return false;
				}
				ZapTo(request->ParameterList["zapto"]);
				request->SocketWriteLn("HTTP/1.0 302 Moved Temporarily");

				if(request->ParameterList["bouquet"] != "")
					request->SocketWriteLn("Location: channellist.dbox2?bouquet="+request->ParameterList["bouquet"]+"#akt");
				else
					request->SocketWriteLn("Location: channellist.dbox2#akt");
				return true;
			}

			if(request->ParameterList["eventlist"] != "")
			{
				request->SendPlainHeader("text/html");
				unsigned id = atol( request->ParameterList["eventlist"].c_str() );
				ShowEventList( request, id );
				return true;
			}
			if(request->ParameterList["1"] == "eventlist")
			{
				request->SendPlainHeader("text/html");
				printf("service id: %u\n",zapit.getCurrentServiceID());
				ShowEventList( request, zapit.getCurrentServiceID() );
				return true;
			}

			if(request->ParameterList["1"] == "shutdown")
			{
				if(!Authenticate(request))
					return false;
				request->SendPlainHeader("text/html");
				request->SendFile(request->Parent->PrivateDocumentRoot,"/shutdown.include");
				request->EndRequest();
				sleep(1);
				Parent->EventServer.sendEvent(NeutrinoMessages::SHUTDOWN, CEventServer::INITID_NHTTPD);
//				controld.shutdown();
				return true;
			}

			if(request->ParameterList["1"] == "tvmode")
			{
				if(!Authenticate(request))
					return false;
				zapit.setMode(CZapitClient::MODE_TV);
				if(request->Parent->DEBUG) printf("switched to tvmode");
				sleep(1);
				request->SocketWriteLn("HTTP/1.0 302 Moved Temporarily");
				request->SocketWriteLn("Location: channellist.dbox2#akt");
				UpdateBouquets();
				return true;
			}

			if(request->ParameterList["1"] == "radiomode")
			{
				if(!Authenticate(request))
					return false;
				zapit.setMode(CZapitClient::MODE_RADIO);
				if(request->Parent->DEBUG) printf("switched to radiomode");
				sleep(1);
				request->SocketWriteLn("HTTP/1.0 302 Moved Temporarily");
				request->SocketWriteLn("Location: channellist.dbox2#akt");
				UpdateBouquets();
				return true;
			}

			if(request->ParameterList["1"] == "settings")
			{
				if(request->Parent->DEBUG) printf("settings\n");
				ShowSettings(request);
				return true;
			}
		}
		else
		{
			if(request->Parent->DEBUG) printf("Keine Parameter gefunden\n");
			return false;
		}
	}
	else
	{
		request->Send404Error();
		return false;
	}
}
*/

/*
//-------------------------------------------------------------------------
// Send functions (for ExecuteCGI)
//-------------------------------------------------------------------------

void TWebDbox::SendEventList(CWebserverRequest *request,unsigned onidSid)
{
int pos;
char *buf = new char[400];

	eList = sectionsd.getEventsServiceKey(onidSid);
	CChannelEventList::iterator eventIterator;
    for( eventIterator = eList.begin(); eventIterator != eList.end(); eventIterator++, pos++ )
	{
		sprintf(buf, "%llu %ld %d %s\n", eventIterator->eventID, eventIterator->startTime, eventIterator->duration, eventIterator->description.c_str());
		request->SocketWrite(buf);
	}
	delete[] buf;
}
//-------------------------------------------------------------------------

void TWebDbox::SendBouquets(CWebserverRequest *request)
{
char *buffer = new char[500];

	for(int i = 0; i < BouquetList.size();i++)
	{
		sprintf(buffer,"%ld %s\n",BouquetList[i].bouquet_nr,BouquetList[i].name);
		request->SocketWrite(buffer);
	}
	delete[] buffer;
};
//-------------------------------------------------------------------------
void TWebDbox::SendBouquet(CWebserverRequest *request,int BouquetNr)
{
char *buffer = new char[500];

	for(int i = 0; i < BouquetsList[BouquetNr].size();i++)
	{
		sprintf(buffer,"%ld %ld %s\n\0",(BouquetsList[BouquetNr])[i].nr,BouquetsList[BouquetNr][i].onid_sid,BouquetsList[BouquetNr][i].name);
		request->SocketWrite(buffer);
	}
	delete[] buffer;
};
//-------------------------------------------------------------------------
void TWebDbox::SendChannelList(CWebserverRequest *request)
{
char *buffer = new char[500];

	for(int i = 0; i < ChannelList.size();i++)
	{
		sprintf(buffer,"%u %s\n",ChannelList[i].onid_sid,ChannelList[i].name);
		request->SocketWrite(buffer);
	}
	delete[] buffer;
};

//-------------------------------------------------------------------------

void TWebDbox::SendStreamInfo(CWebserverRequest* request)
{

	int bitInfo[10];
	char buf[100];
	GetStreamInfo(bitInfo);
	sprintf((char*) buf, "%d\n%d\n", bitInfo[0], bitInfo[1] );
	request->SocketWrite(buf); //Resolution x y
	sprintf((char*) buf, "%d\n", bitInfo[4]*50);
	request->SocketWrite(buf); //Bitrate bit/sec
	
	switch ( bitInfo[2] ) //format
	{
		case 2: request->SocketWrite("4:3\n"); break;
		case 3: request->SocketWrite("16:9\n"); break;
		case 4: request->SocketWrite("2.21:1\n"); break;
		default: request->SocketWrite("unknown\n"); break;
	}

	switch ( bitInfo[3] ) //fps
	{
		case 3: request->SocketWrite("25\n"); break;
		case 6: request->SocketWrite("50\n"); break;
		default: request->SocketWrite("unknown\n");
	}
	request->SocketWriteLn(audiotype_names[bitInfo[6]]);
//	switch ( bitInfo[6] )
//	{
//		case 1: request->SocketWrite("single channel\n"); break;
//		case 2: request->SocketWrite("dual channel\n"); break;
//		case 3: request->SocketWrite("joint stereo\n"); break;
//		case 4: request->SocketWrite("stereo\n"); break;
//		default: request->SocketWrite("unknown\n");
//	}

}
//-------------------------------------------------------------------------

void TWebDbox::SendcurrentVAPid(CWebserverRequest* request)
{
CZapitClient::responseGetPIDs pids;
	if(Parent->DEBUG) printf("hole jetzt die pids\n");
	zapit.getPIDS(pids);

	char *buf = new char[300];
	sprintf(buf, "%u\n%u\n", pids.PIDs.vpid, pids.APIDs[0].pid);
	request->SocketWrite(buf);
	delete buf;
}

//-------------------------------------------------------------------------
void TWebDbox::SendSettings(CWebserverRequest* request)
{
//	request->SendPlainHeader("text/html");

//	int boxtype = controld.getBoxType();
//	int videooutput = controld.getVideoOutput();
//	int videoformat = controld.getVideoFormat();
//	char *buffer = new char[500];

//	sprintf(buffer,"Boxtype %s\nvideooutput %s\nvideoformat %s\n",dbox_names[boxtype],videooutput_names[videooutput],videoformat_names[videoformat]);
	request->SocketWriteLn("Boxtype "+Dbox_Hersteller[controld.getBoxType()]+"\nvideooutput "+videooutput_names[controld.getVideoOutput()]+"\nvideoformat "+videoformat_names[controld.getVideoFormat()]);
//	request->SocketWrite(buffer);
//	delete[] buffer;
	if(Parent->DEBUG) printf("End SendSettings\n");
}
*/
/*
//-------------------------------------------------------------------------
// Show funtions (Execute)
//-------------------------------------------------------------------------
void TWebDbox::ShowCurrentStreamInfo(CWebserverRequest* request)
{
	CStringList params;
	CZapitClient::CCurrentServiceInfo serviceinfo;
	serviceinfo = zapit.getCurrentServiceInfo();
	params["onid"] = itoh(serviceinfo.onid);
	params["sid"] = itoh(serviceinfo.sid);
	params["tsid"] = itoh(serviceinfo.tsid);
	params["vpid"] = itoh(serviceinfo.vdid);
	params["apid"] = itoh(serviceinfo.apid);
	params["vtxtpid"] = itoh(serviceinfo.vtxtpid);
	params["tsfrequency"] = itoa(serviceinfo.tsfrequency);
	params["polarisation"] = serviceinfo.polarisation==1?"v":"h";
	params["ServiceName"] = GetServiceName(zapit.getCurrentServiceID());
	int bitInfo[10];
	char buf[100];

	GetStreamInfo(bitInfo);
//	params["AspectRatio"] = controld.getAspectRatio();
	
	sprintf((char*) buf, "%d x %d", bitInfo[0], bitInfo[1] );
	params["VideoFormat"] = buf; //Resolution x y
	sprintf((char*) buf, "%d\n", bitInfo[4]*50);
	params["BitRate"] = buf; //Bitrate bit/sec
	
	switch ( bitInfo[2] ) //format
	{
		case 2: params["AspectRatio"] = "4:3"; break;
		case 3: params["AspectRatio"] = "16:9"; break;
		case 4: params["AspectRatio"] = "2.21:1"; break;
		default: params["AspectRatio"] = "unknown"; break;
	}

	switch ( bitInfo[3] ) //fps
	{
		case 3: params["FPS"] = "25"; break;
		case 6: params["FPS"] = "50"; break;
		default: params["FPS"] = "unknown";
	}

	switch ( bitInfo[6] )
	{
		case 1: params["AudioType"] = "single channel"; break;
		case 2: params["AudioType"] = "dual channel"; break;
		case 3: params["AudioType"] = "joint stereo"; break;
		case 4: params["AudioType"] = "stereo"; break;
		default: params["AudioType"] = "unknown";
	}
	request->ParseFile(Parent->PrivateDocumentRoot + "/settings.html",params);


}

//-------------------------------------------------------------------------
void TWebDbox::ShowEventList(CWebserverRequest *request,unsigned onidSid)
{
char *buf = new char[1400];
char classname;
int pos = 0;
	
	eList = sectionsd.getEventsServiceKey(onidSid);
	CChannelEventList::iterator eventIterator;
	request->SendHTMLHeader("DBOX2-Neutrino Channellist");


	request->SocketWrite("<h3>Programmvorschau: ");
	request->SocketWrite(GetServiceName(onidSid)); 	//search channelname...
	request->SocketWriteLn("</h3>");

	request->SocketWrite("<table cellspacing=\"0\">\n");

    for( eventIterator = eList.begin(); eventIterator != eList.end(); eventIterator++, pos++ )
	{
		classname = (pos&1)?'a':'b';
		char zbuffer[25] = {0};
		struct tm *mtime = localtime(&eventIterator->startTime); //(const time_t*)eventIterator->startTime);
		strftime(zbuffer,20,"%d.%m. %H:%M",mtime);
		sprintf(buf, "<tr><td class=\"%c\">%s<small> (%d min)</small></tr></td>\n", classname, zbuffer, eventIterator->duration / 60);
		sprintf(&buf[strlen(buf)], "<tr><td class=\"%c\"><a href=epg.dbox2?eventid=%llx>%s</a></td></tr>\n", classname,eventIterator->eventID, eventIterator->description.c_str());
		request->SocketWrite(buf);
	}
	delete[] buf;
	request->SocketWriteLn("</table>");
	request->SendHTMLFooter();
}
//-------------------------------------------------------------------------

void TWebDbox::ShowSettings(CWebserverRequest *request)
{
	request->SendHTMLHeader("DBOX2-Neutrino Settings");
	SendSettings(request);
}
//-------------------------------------------------------------------------
void TWebDbox::ShowBouquets(CWebserverRequest *request)
{
char *buffer = new char[500];
	request->SendHTMLHeader("DBOX2-Neutrino Bouquetliste");
	request->SocketWriteLn("<div background-color:#000000;>");
	request->SocketWriteLn("<table cellspacing=0 cellpadding=0 border=0>\n");
	sprintf(buffer,"<tr><td><a class=b href=\"channellist.dbox2#akt\" target=\"content\"><h5>Alle Kanäle</h5></a></td></tr>\n");
	request->SocketWrite(buffer);
	request->SocketWrite("<tr><td><HR></td></tr>\n");


	for(int i = 0; i < BouquetList.size();i++)
	{
		sprintf(buffer,"<tr><td><h5><a class=b href=\"channellist.dbox2?bouquet=%d#akt\" target=\"content\">%s</a></h5></td></tr>\n",BouquetList[i].bouquet_nr,BouquetList[i].name);
		request->SocketWrite(buffer);
	}
	sprintf(buffer,"</table>\n");
	request->SocketWrite(buffer);
	request->SocketWrite("</div>");
	request->SendHTMLFooter();
	delete[] buffer;
}

//-------------------------------------------------------------------------
void TWebDbox::ShowBouquet(CWebserverRequest* request,int BouquetNr = -1)
{
	if(Parent->DEBUG) printf("ShowBouquet\n");
	CZapitClient::BouquetChannelList channellist;
	if(BouquetNr > 0)
		channellist = BouquetsList[BouquetNr];
	else
		channellist = ChannelList;

	GetChannelEvents();

	request->SendHTMLHeader("DBOX2-Neutrino Kanalliste");

	request->SocketWriteLn("<table cellspacing=\"0\">");

	int i = 1;
	char classname[] = "a\0";
	char buffer[400];
	int current_channel = zapit.getCurrentServiceID();
	for(int i = 0; i < channellist.size();i++)
	{
		classname[0] = (i&1)?'a':'b';
		if(channellist[i].onid_sid == current_channel)
			classname[0] = 'c';

		char bouquetstr[20]="\0";
		if(BouquetNr >=0)
			sprintf(bouquetstr,"&bouquet=%d",BouquetNr);
		sprintf(buffer,"<tr><td class=\"%c\">%s<a href=\"switch.dbox2?zapto=%d%s\">%d. %s</a>&nbsp;<a href=\"switch.dbox2?eventlist=%u\">%s</a></td></tr>",classname[0],((channellist[i].onid_sid == current_channel)?"<a name=akt></a>":" "),channellist[i].onid_sid,bouquetstr,i + 1,channellist[i].name,channellist[i].onid_sid,((ChannelListEvents[channellist[i].onid_sid])?"<img src=\"../images/elist.gif\" border=\"0\" alt=\"Programmvorschau\">":" "));
		request->SocketWrite(buffer);

		if(ChannelListEvents[channellist[i].onid_sid])
		{
			sprintf(buffer,"<tr><td class=\"%cepg\"><a href=epg.dbox2?epgid=%llx&startzeit=%lx>",classname[0],ChannelListEvents[channellist[i].onid_sid]->eventID,ChannelListEvents[channellist[i].onid_sid]->startTime);
			request->SocketWrite(buffer);
			request->SocketWrite((char *)ChannelListEvents[channellist[i].onid_sid]->description.c_str());
			request->SocketWrite("&nbsp;");
			sprintf(buffer,"<font size=-3>(%d von %d min, %d\%)</font></a>&nbsp;</td></tr>\n",(time(NULL)-ChannelListEvents[channellist[i].onid_sid]->startTime)/60,ChannelListEvents[channellist[i].onid_sid]->duration / 60,100 * (time(NULL)-ChannelListEvents[channellist[i].onid_sid]->startTime) / ChannelListEvents[channellist[i].onid_sid]->duration  );
			request->SocketWrite(buffer);
		}
	}

	request->SocketWriteLn("</table>");

	request->SendHTMLFooter();
}
//-------------------------------------------------------------------------

bool TWebDbox::ShowControlpanel(CWebserverRequest* request)
{
	char volstr_on[10];
	char volstr_off[10];

	if (request->ParameterList.size() > 0)
	{
		if( request->ParameterList["1"] == "volumemute")
		{
			if(!Authenticate(request))
				return false;
			bool mute = controld.getMute();
			controld.setMute( !mute );
			if(request->Parent->DEBUG) printf("mute\n");
		}
		else if( request->ParameterList["1"] == "volumeplus")
		{
			if(!Authenticate(request))
				return false;
			char vol = controld.getVolume();
			vol+=5;
			if (vol>100)
				vol=100;
			controld.setVolume(vol);
			if(request->Parent->DEBUG) printf("Volume plus: %d\n",vol);
		}
		else if( request->ParameterList["1"] == "volumeminus")
		{
			if(!Authenticate(request))
				return false;
			char vol = controld.getVolume();
			if (vol>0)
				vol-=5;
			controld.setVolume(vol);
			if(request->Parent->DEBUG) printf("Volume minus: %d\n",vol);
		}
		else if( request->ParameterList["standby"] != "")
		{
			if(request->ParameterList["standby"] == "on")
			{
				Parent->EventServer.sendEvent(NeutrinoMessages::STANDBY_ON, CEventServer::INITID_NHTTPD);
				standby_mode = true;
			}
			if(request->ParameterList["standby"] == "off")
			{
				Parent->EventServer.sendEvent(NeutrinoMessages::STANDBY_OFF, CEventServer::INITID_NHTTPD);
				standby_mode = false;
			}
		}
	}
	if(Parent->DEBUG)
	{
		CStringList params;
		params["standby"] = standby_mode?"off":"on";
		switch(controld.getBoxType())
		{
			case CControldClient::BOXTYPE_NOKIA :
				params["IMG"] = "<img src=\"/images/nokia.gif\" usemap=\"#nokia\" border=0>";
				break;
			case CControldClient::BOXTYPE_SAGEM :
				params["IMG"] = "<img src=\"/images/sagem.gif\" usemap=\"#sagem\" border=0>";
				break;
			default :
				params["IMG"] = "<img src=\"/images/nokia.gif\" usemap=\"#nokia\" border=0>";
		}
		printf("params[IMG]: '%s'\n",params["IMG"].c_str());
		request->ParseFile(request->Parent->PrivateDocumentRoot + "/controlpanel.html", params);
	}
	else
	{

		request->SendPlainHeader("text/html");

		string mutefile = controld.getMute()?"mute":"muted";
		string mutestring = "<td><a href=\"/fb/controlpanel.dbox2?volumemute\" target=navi onMouseOver=\"mute.src='../images/"+ mutefile+"_on.jpg';\" onMouseOut=\"mute.src='../images/"+ mutefile+"_off.jpg';\"><img src=/images/"+ mutefile+"_off.jpg width=25 height=28 border=0 name=mute></a><br></td>\n";

		char vol = controld.getVolume();
		sprintf((char*) &volstr_on, "%d\0", vol);
		sprintf((char*) &volstr_off, "%d\0", 100-vol);
	 
		request->SendFile(request->Parent->PrivateDocumentRoot,"/controlpanel.include1");
		//muted
		request->SocketWrite(mutestring);
		request->SendFile(request->Parent->PrivateDocumentRoot,"/controlpanel.include2");
		//volume bar...
		request->SocketWrite("<td><img src=../images/vol_flashed.jpg width=");
		request->SocketWrite(volstr_on);
		request->SocketWrite(" height=10 border=0><br></td>\n");
		request->SocketWrite("<td><img src=../images/vol_unflashed.jpg width=");
		request->SocketWrite(volstr_off);
		request->SocketWrite(" height=10 border=0><br></td>\n");
		request->SendFile(request->Parent->PrivateDocumentRoot,"/controlpanel.include3");
	}
	return true;

}


*/