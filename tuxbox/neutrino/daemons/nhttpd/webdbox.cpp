/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

	$Id: webdbox.cpp,v 1.19 2002/04/17 20:15:48 dirch Exp $

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
#include "avcontrol.h"

#define SA struct sockaddr
#define SAI struct sockaddr_in

//-------------------------------------------------------------------------
// Konstruktor und destruktor
//-------------------------------------------------------------------------

TWebDbox::TWebDbox(TWebserver *server)
{
	Parent=server;
	UpdateBouquets();
}
//-------------------------------------------------------------------------

TWebDbox::~TWebDbox()
{
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// ExecuteCGI und Execute
//-------------------------------------------------------------------------
bool TWebDbox::ExecuteCGI(CWebserverRequest* request)
{
	if(Parent->DEBUG) printf("Execute CGI : %s\n",request->Filename.c_str());

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

	if(request->Filename.compare("getservices") == 0)		// sendet die datei services.xml
	{
		request->SendPlainHeader();
		request->SendFile("/var/tuxbox/config/zapit","services.xml");
		return true;
	}

	if(request->Filename.compare("getbouquets") == 0)		// sendet die datei bouquets.xml
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
				SendStreaminfo(request);
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
			controld.shutdown();			// dbox runterfahren
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

	if(request->Filename.compare("bouquets") == 0)
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
					sprintf(buffer,"%u %llu %s\n",ChannelList[i].onid_sid,ChannelListEvents[ChannelList[i].onid_sid]->eventID,ChannelListEvents[ChannelList[i].onid_sid]->description.c_str() /*eList[n].eventID,eList[n].description.c_str()*/);
					request->SocketWrite(buffer);
				}
			}
		}
		else if(request->ParameterList.size() == 1)
		{

//			request->ParameterList->PrintParameterList();
			if(request->ParameterList["1"] == "ext")
			{
				char buffer[255];
				for(int i = 0; i < ChannelList.size();i++)
				{
					if(ChannelListEvents[ChannelList[i].onid_sid])
					{
						sprintf(buffer,"%u %ld %ld %llu %s\n",ChannelList[i].onid_sid,ChannelListEvents[ChannelList[i].onid_sid]->startTime,ChannelListEvents[ChannelList[i].onid_sid]->duration,ChannelListEvents[ChannelList[i].onid_sid]->eventID,ChannelListEvents[ChannelList[i].onid_sid]->description.c_str() /*eList[n].eventID,eList[n].description.c_str()*/);
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
				GetEventList( request, id, true);
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
					if(request->Parent->DEBUG) printf("switched to tvmode");
					sleep(1);
					UpdateBouquets();
				}
				else if(request->ParameterList["mode"] == "RADIO")
				{				
					zapit.setMode(CZapitClient::MODE_RADIO);
					if(request->Parent->DEBUG) printf("switched to radiomode");
					sleep(1);
					UpdateBouquets();
				}
				else
					return false;
			}
			if(request->ParameterList["1"] == "getpids")		// getpids !
			{
				SendcurrentVAPid(request);
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
				ZapTo( (char *)request->ParameterList["1"].c_str() );
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

bool TWebDbox::Execute(CWebserverRequest* request)
{
	if(Parent->DEBUG) printf("Executing %s\n",request->Filename.c_str());
	if(request->Filename.compare("test.dbox2") == 0)
	{

		printf("Teste nun\n");
		request->SendPlainHeader("text/html");
		request->SocketWrite("alles wird gut\n");
		return true;
	}
/*
	if(strcmp(request->Filename.c_str(),"avcontrol.dbox2") == 0)
	{
		request->SendPlainHeader("text/html");
		if (request->ParameterList.size() == 4)
		{
			int vcrvideo,vcraudio,tvvideo,tvaudio,auxaudio;
			if(request->ParameterList->GetIndex("TV_AUDIO") != -1)
			{
				tvaudio = atoi(request->ParameterList->GetValue(request->ParameterList->GetIndex("TV_AUDIO")));
				if(request->ParameterList->GetIndex("TV_VIDEO") != -1)
				{
					tvvideo = atoi(request->ParameterList->GetValue(request->ParameterList->GetIndex("TV_VIDEO")));
					if(request->ParameterList->GetIndex("VCR_AUDIO") != -1)
					{
						vcraudio = atoi(request->ParameterList->GetValue(request->ParameterList->GetIndex("VCR_AUDIO")));
						if(request->ParameterList->GetIndex("VCR_VIDEO") != -1)
						{
							vcraudio = atoi(request->ParameterList->GetValue(request->ParameterList->GetIndex("VCR_AUDIO")));
							if(request->ParameterList->GetIndex("AUX_AUDIO") != -1)
							{
								auxaudio = atoi(request->ParameterList->GetValue(request->ParameterList->GetIndex("AUX_AUDIO")));
								TAvControl *av = new TAvControl();
								av->Set_TV_Sources(tvvideo,tvaudio);
								av->Set_VCR_Sources(vcrvideo,vcraudio);
								av->Set_AUX_Sources(auxaudio);
								delete av;
								return true;
							}
						}
					}
				}
			}
		}
		return false;
	}
*/
	if(request->Filename.compare("upload.dbox2") == 0)
	{
		if(request->Upload != NULL)
		{
			request->Upload->HandleUpload();
		}
		else
			printf("Kein Dateianhang gefunden\n");
		return true;
	}
	if (request->Filename.compare("bouquetlist.dbox2") == 0)
	{
		request->SendPlainHeader("text/html");
		ShowBouquets(request);
		return true;
	}
	if (request->Filename.compare("channellist.dbox2") == 0)
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
	else
	{
		if (request->Filename.compare("controlpanel.dbox2") == 0)
		{	//funktionen für controlpanel links
			request->SendPlainHeader("text/html");
			ShowControlpanel(request);
			return true;

		}
		else if (request->Filename.compare("epg.dbox2") == 0)
		{
			if(request->ParameterList.size() > 0)
			{											// wenn parameter vorhanden sind
				request->SendHTMLHeader("EPG");

				request->SocketWriteLn("<html>\n<head><title>DBOX2-Neutrino EPG</title><link rel=\"stylesheet\" type=\"text/css\" href=\"../channellist.css\"></head>");
				//request->println("<script> function openWindow(theURL) { window.open(theURL,'Neutrino-EPG','scrollbars=no,width=100,height=100');}</script>");
				request->SocketWriteLn("<body>");

				if(request->ParameterList["eventid"] != "")
				{
					unsigned long long epgid;
					sscanf(request->ParameterList["eventid"].c_str(), "%llx", &epgid);
					CShortEPGData epg;
					if(sectionsd.getEPGidShort(epgid,&epg))
					{
						char *buffer2 = new char[1024];
						sprintf(buffer2,"<H1>%s</H1><BR>\n<H2>%s</H2><BR>\n<B>%s</B><BR>\n\0",epg.title.c_str(),epg.info1.c_str(),epg.info2.c_str());
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
						sprintf(buffer2,"<H1>%s</H1><BR>\n<H2>%s</H2><BR>\n<B>%s</B><BR>\n\0",epg.title.c_str(),epg.info1.c_str(),epg.info2.c_str());
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
					ZapTo((char *) request->ParameterList["zapto"].c_str() );
					request->SocketWriteLn("HTTP/1.0 302 Moved Temporarily");
					request->SocketWriteLn("Location: channellist.dbox2#akt");
					return true;
				}

				if(request->ParameterList["eventlist"] != "")
				{
					request->SendPlainHeader("text/html");
					unsigned id = atol( request->ParameterList["eventlist"].c_str() );
					GetEventList( request, id );
					return true;
				}

				if(request->ParameterList["1"] == "shutdown")
				{
					request->SendPlainHeader("text/html");
					request->SendFile(request->Parent->PrivateDocumentRoot,"/shutdown.include");
					request->EndRequest();
					sleep(1);
					controld.shutdown();
					return true;
				}

				if(request->ParameterList["1"] == "tvmode")
				{
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
	}
}

//-------------------------------------------------------------------------
// Get functions 
//-------------------------------------------------------------------------


void TWebDbox::GetChannelEvents()
{
	eList = sectionsd.getChannelEvents();
	CChannelEventList::iterator eventIterator;

    for( eventIterator = eList.begin(); eventIterator != eList.end(); eventIterator++ )
		ChannelListEvents[(*eventIterator).serviceID] = &(*eventIterator);
}
//-------------------------------------------------------------------------

void TWebDbox::GetEventList(CWebserverRequest *request,unsigned onidSid, bool cgi = false)
{

char *epgID,*edate,*etime,*eduration,*ename;

char *actPos=NULL;
int pos;
char classname;
int sock_fd;
Tmconnect con;

	if((sock_fd = Parent->SocketConnect(&con,sectionsd::portNumber)) != -1)
	{
		sectionsd::msgRequestHeader req;
		req.version = 2;
		req.command = sectionsd::allEventsChannelID;
		req.dataLength = 4;
		write(sock_fd, &req, sizeof(req));
		write(sock_fd, &onidSid, req.dataLength);

		sectionsd::msgResponseHeader resp;
		memset(&resp, 0, sizeof(resp));
		if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0)
		{
			close(sock_fd);
			return;
		}
		if ( resp.dataLength>0 )
		{
			char* buffer = new char[resp.dataLength] ;

			if( read(sock_fd, buffer, resp.dataLength) <= 0)
			{
				delete[] buffer;
				close(sock_fd);
				printf("EventList::readEvents - read from socket failed!\n,");
				return;
			}
			if(!cgi)
			{
				request->SendHTMLHeader("DBOX2-Neutrino Channellist");

				//search channame...
				request->SocketWrite("<h3>Programmvorschau: ");
				request->SocketWrite(GetServiceName(onidSid));
				request->SocketWriteLn("</h3>");

				request->SocketWrite("<table cellspacing=\"0\">\n");
			}
			pos=1;
			actPos=buffer;

			while(*actPos && actPos<buffer+resp.dataLength)
			{
				epgID = actPos;
				for(;actPos<buffer+resp.dataLength && (*actPos != ' ');actPos++)
				;
				*actPos=0;
				edate = ++actPos;
				for(;actPos<buffer+resp.dataLength && (*actPos != ' ');actPos++)
				;
				*actPos=0;
				etime = ++actPos;
				for(;actPos<buffer+resp.dataLength && (*actPos != ' ');actPos++)
				;
				*actPos=0;
				eduration = ++actPos;
				for(;actPos<buffer+resp.dataLength && (*actPos != ' ');actPos++)
				;
				*actPos=0;
				ename = ++actPos;
				for(;actPos<buffer+resp.dataLength && (*actPos != '\n');actPos++)
				;
				*actPos=0;
				actPos++;

				char *buf = new char[1400];
				struct  tm *tmzeit;
				time_t  tZeit  = time(NULL),zeit;

				tmzeit = localtime(&tZeit);
				int tag = 0,monat=0;
				int stunde=0,minute=0;

				sscanf(edate,"%i.%i",&tag,&monat);
				sscanf(etime,"%i:%i",&stunde,&minute);

				tmzeit->tm_min = minute;
				tmzeit->tm_hour = stunde;
				tmzeit->tm_mday = tag;
				tmzeit->tm_mon = monat -1;
				tmzeit->tm_sec = 0;
				zeit = mktime(tmzeit);
				if((zeit + (atoi(eduration) * 60)) >= time(NULL))
				{
					if(cgi)
					{
						sprintf(buf, "%s %s %s %s %s\n", epgID, edate, etime, eduration, ename);
					}
					else
					{
						classname = (pos&1)?'a':'b';

						sprintf(buf, "<tr><td class=\"%c\">%s, %s <small>(%smin)</small></tr></td>\n", classname, edate, etime, eduration,zeit,onidSid);
						sprintf(&buf[strlen(buf)], "<tr><td class=\"%c\"><a href=epg.dbox2?eventid=%s>%s</a></td></tr>\n", classname,epgID, ename);
					}
					request->SocketWrite(buf);
					pos++;
				}
				delete[] buf;
			}

			delete[] buffer;
		}
		close(sock_fd);
		if(!cgi)
		{
			request->SocketWriteLn("</table>");
			request->SendHTMLFooter();
		}
	}

}

char* TWebDbox::GetServiceName(int onid_sid)
{
	for(int i = 0; i < ChannelList.size();i++)
		if( ChannelList[i].onid_sid == onid_sid)
			return ChannelList[i].name;
	return "";
}
//-------------------------------------------------------------------------
// Send functions (for ExecuteCGI)
//-------------------------------------------------------------------------

void TWebDbox::SendBouquets(CWebserverRequest *request)
{
char *buffer = new char[500];

	for(int i = 0; i < BouquetList.size();i++)
	{
		sprintf(buffer,"BouquetList[i]. bouquet_nr: %ld name: '%s'\n",BouquetList[i].bouquet_nr,BouquetList[i].name);
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
		sprintf(buffer,"BouquetList[i]. nr: %ld onsid: %ld name: '%s'\n\0",(BouquetsList[BouquetNr])[i].nr,BouquetsList[BouquetNr][i].onid_sid,BouquetsList[BouquetNr][i].name);
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
void TWebDbox::SendStreaminfo(CWebserverRequest* request)
{
	int bitInfo[10];
	char *key,*tmpptr,buf[100];
	int value, pos=0;

	FILE* fd = fopen("/proc/bus/bitstream", "rt");
	if (fd==NULL)
	{
		printf("error while opening proc-bitstream\n" );
		return;
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

	switch ( bitInfo[6] )
	{
		case 1: request->SocketWrite("single channel\n"); break;
		case 2: request->SocketWrite("dual channel\n"); break;
		case 3: request->SocketWrite("joint stereo\n"); break;
		case 4: request->SocketWrite("stereo\n"); break;
		default: request->SocketWrite("unknown\n");
	}

}
//-------------------------------------------------------------------------

void TWebDbox::SendcurrentVAPid(CWebserverRequest* request)
{
CZapitClient::responseGetPIDs pids;
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
	char dbox_names[4][10]={"","Nokia","Sagem","Philips"};
	char videooutput_names[3][13]={"Composite","RGB","S-Video"};
	char videoformat_names[3][13]={"automatic","16:9","4:3"};

	int boxtype = controld.getBoxType();
	int videooutput = controld.getVideoOutput();
	int videoformat = controld.getVideoFormat();
	char *buffer = new char[500];

	sprintf(buffer,"Boxtype %s\nvideooutput %s\nvideoformat %s\n",dbox_names[boxtype],videooutput_names[videooutput],videoformat_names[videoformat]);
	request->SocketWrite(buffer);
	delete[] buffer;
	if(Parent->DEBUG) printf("End SendSettings\n");
}

//-------------------------------------------------------------------------
// Show funtions (Execute)
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
	if(BouquetNr >= 0)
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
//		else
//			sprintf(bouquetstr,"\0");
//		printf("bouquetstr: '%s'\n",bouquetstr);
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

void TWebDbox::ShowControlpanel(CWebserverRequest* request)
{
	if (request->ParameterList.size() > 0)
	{
		if( request->ParameterList["1"] == "volumemute")
		{
			bool mute = controld.getMute();
			controld.setMute( !mute );
			if(request->Parent->DEBUG) printf("mute\n");
		}
		else if( request->ParameterList["1"] == "volumeplus")
		{
			char vol = controld.getVolume();
			vol+=5;
			if (vol>100)
				vol=100;
			controld.setVolume(vol);
			if(request->Parent->DEBUG) printf("Volume plus: %d\n",vol);
		}
		else if( request->ParameterList["1"] == "volumeminus")
		{
			char vol = controld.getVolume();
			if (vol>0)
				vol-=5;
			controld.setVolume(vol);
			if(request->Parent->DEBUG) printf("Volume minus: %d\n",vol);
		}
	}
	bool mute = controld.getMute();
	char mutefile[8] = {0};
	char vol = controld.getVolume();
	if (!mute)
		strcpy(mutefile,"mute");
	else
		strcpy(mutefile,"muted");

	char *mutestr = new char[500];
	sprintf(mutestr,"<td><a href=\"/fb/controlpanel.dbox2?volumemute\" target=navi onMouseOver=\"mute.src='../images/%s_on.jpg';\" onMouseOut=\"mute.src='../images/%s_off.jpg';\"><img src=/images/%s_off.jpg width=25 height=28 border=0 name=mute></a><br></td>\n\0",mutefile,mutefile,mutefile);
	if(request->Parent->DEBUG) printf("mutestr='%s'\n",mutestr);

	char volstr_on[10]={0};
	char volstr_off[10]={0};
	sprintf((char*) &volstr_on, "%d", vol);
	sprintf((char*) &volstr_off, "%d", 100-vol);
	request->SendFile(request->Parent->PrivateDocumentRoot.c_str(),"/controlpanel.include1");
	//muted
	request->SocketWrite(mutestr);
	request->SendFile(request->Parent->PrivateDocumentRoot.c_str(),"/controlpanel.include2");
	//volume bar...
	request->SocketWrite("<td><img src=../images/vol_flashed.jpg width=");
	request->SocketWrite(volstr_on);
	request->SocketWrite(" height=10 border=0><br></td>\n");
	request->SocketWrite("<td><img src=../images/vol_unflashed.jpg width=");
	request->SocketWrite(volstr_off);
	request->SocketWrite(" height=10 border=0><br></td>\n");
	request->SendFile(request->Parent->PrivateDocumentRoot.c_str(),"/controlpanel.include3");
	delete[] mutestr;
}


//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
void TWebDbox::UpdateBouquets(void)
{
	GetBouquets();
	for(int i = 1; i <= BouquetList.size();i++)
		GetBouquet(i);
	GetChannelList();
}
//-------------------------------------------------------------------------

void TWebDbox::ZapTo(char *target)
{
int sock_fd;
Tmconnect con;

	int sidonid = atol(target);
	if(sidonid == zapit.getCurrentServiceID())
	{
		if(Parent->DEBUG) printf("Kanal ist aktuell\n");
		return;
	}
	zapit.zapTo_serviceID(sidonid);
}
