/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski


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

	// Revision 1.1  11.02.2002 20:20  dirch
	// Revision 1.2  22.03.2002 20:20  dirch

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

TWebDbox::TWebDbox(TWebserver *server)
{
	Parent=server;
	old_ChannelList = NULL;
	UpdateBouquets();
}
//-------------------------------------------------------------------------

TWebDbox::~TWebDbox()
{
	if(old_ChannelList)
		delete old_ChannelList;
}

//-------------------------------------------------------------------------
static char* copyStringto(const char* from, char* to, int len, char delim)
{
	const char *fromend=from+len;
	while(*from!=delim && from<fromend && *from)
	{
		*(to++)=*(from++);
	}
	*to=0;
	return (char *)++from;
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
void TWebDbox::ParseString(TWebserverRequest * request,char *str,TParameterList * Parameter)
{
char *panfang,*pende;
char *obuffer;
int len = strlen(str);
	obuffer = (char *) malloc(len + 200);
	memset(obuffer,0,(len + 200));
	if(obuffer)
	{
		int i = 0;
		int o = 0;
		do
		{
			while( ((str[i] != '%') || (str[i+1] != '%')) && (i < len) )
				obuffer[o++] = str[i++];
			if(i < len)
			{
				panfang = &str[i+2];
				if( (pende = strstr(panfang,"%%")) != NULL)
				{
					TString *parameter = new TString(panfang,pende - panfang);
					if(Parameter->GetIndex(parameter->c_str()) != -1)
					{
						char * value = Parameter->GetValue(Parameter->GetIndex(parameter->c_str()));
						strncpy(&obuffer[o],value,strlen(value));
						o += strlen(value);

					}
					else
					{
						char nicht_gefunden[] = "(nicht gefunden)\0";
						strcpy(&obuffer[o],nicht_gefunden);
						o += strlen(nicht_gefunden);
					}
					delete parameter;
					i = pende - str + 2;
				}
			}
		}while(i < len);
		request->SocketWrite(obuffer);
	}
	free(obuffer);
}

//-------------------------------------------------------------------------


//-------------------------------------------------------------------------

bool TWebDbox::ExecuteCGI(TWebserverRequest* request)
{
	if(Parent->DEBUG) printf("Execute CGI : %s\n",request->Filename->c_str());
	if(strcmp(request->Filename->c_str(),"getdate") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList->Count==0)
		{	//paramlos
			char *timestr = new char[50];
			struct timeb tm;
			ftime(&tm);
			strftime(timestr, 20, "%d.%m.%Y\n", localtime(&tm.time) );	//aktuelle zeit ausgeben
			request->SocketWrite(timestr);
			delete[] timestr;
		}
		else
			request->SocketWrite("error");
	}

	if(strcmp(request->Filename->c_str(),"getservices") == 0)		// sendet die datei services.xml
	{
		request->SendPlainHeader();
		request->SendFile("/var/tuxbox/config/zapit","services.xml");
	}

	if(strcmp(request->Filename->c_str(),"getbouquets") == 0)		// sendet die datei bouquets.xml
	{
		request->SendPlainHeader();
		request->SendFile("/var/tuxbox/config/zapit","bouquets.xml");
	}
	
	if(strcmp(request->Filename->c_str(),"gettime") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList->Count==0)
		{	//paramlos
			char *timestr = new char[50];
			struct timeb tm;
			ftime(&tm);
			strftime(timestr, 20, "%H:%M:%S\n", localtime(&tm.time) - timezone );	// aktuelles datum ausgeben
			request->SocketWrite(timestr);
			delete[] timestr;
		}
		else
			request->SocketWrite("error");
	}

	if(strcmp(request->Filename->c_str(),"info") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList->Count == 0)
		{	//paramlos
			request->SocketWrite("Neutrino NG\n");
		}
		else if (request->ParameterList->GetIndex("streaminfo") != -1)	// streminfo ausgeben
		{
			SendStreaminfo(request);
		}
		else
			request->SocketWrite("error");	
	}

	if(strcmp(request->Filename->c_str(),"shutdown") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList->Count == 0)
		{	//paramlos
			request->SocketWrite("shutdown");
			controld.shutdown();			// dbox runterfahren
			request->SocketWrite("ok");
		}
		else
		{
			request->SocketWrite("error");
		}
	}

	if(strcmp(request->Filename->c_str(),"volume") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList->Count == 0)
		{	//paramlos - aktuelles volume anzeigen
			char buf[10];
			sprintf(buf, "%d", controld.getVolume());			// volume ausgeben
			request->SocketWrite(buf);
		}
		else
		if (request->ParameterList->Count == 1)
		{
			if(request->ParameterList->GetIndex("mute") != -1)
			{
				controld.setMute(true);
				request->SocketWrite("mute");					// muten
			}
			else
			if(request->ParameterList->GetIndex("unmute") != -1)
			{
				controld.setMute(false);
				request->SocketWrite("unmute");					// unmuten
			}
			else
			if(request->ParameterList->GetIndex("status") != -1)
			{
				request->SocketWrite( (char *) (controld.getMute()?"1":"0") );	//  mute 
			}
			else
			{	//set volume
				char vol = atol( request->ParameterList->GetValue(0) );
				request->SocketWrite( request->ParameterList->GetValue(0) );
				controld.setVolume(vol);
				request->SocketWrite("ok");
			}
		}
		else
		{
			request->SocketWrite("error");
		}
	}

	if(strcmp(request->Filename->c_str(),"channellist") == 0)
	{
	char buf[250];
		request->SendPlainHeader();          // Standard httpd header senden
		SendChannelList(request);
	}

	if(strcmp(request->Filename->c_str(),"bouquets") == 0)
	{
	char buf[250];
		request->SendPlainHeader();          // Standard httpd header senden
		SendBouquets(request);
	}

	if(strcmp(request->Filename->c_str(), "epg") == 0)
	{
		GetChannelEvents();
		request->SendPlainHeader();          // Standard httpd header senden
		if(Parent->DEBUG) printf("EPG, Parameter: %d\n",request->ParameterList->Count);
		if(Parent->DEBUG && request->ParameterList->Count > 0)
			request->ParameterList->PrintParameterList();
		if(request->ParameterList->Count == 0)
		{
			
			if(Parent->DEBUG) printf("Jetzt channelList\n");
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
		else if(request->ParameterList->Count == 1)
		{
//			request->ParameterList->PrintParameterList();

			if(request->ParameterList->GetIndex("eventid") != -1)
			{	//special epg query
				unsigned long long epgid;
				sscanf( request->ParameterList->GetValue(request->ParameterList->GetIndex("eventid")), "%llx", &epgid);
				CShortEPGData epg;
				if(sectionsd.getEPGidShort(epgid,&epg))
				{
					request->SocketWriteLn("test\n");
					request->SocketWriteLn((char*)epg.title.c_str());
					request->SocketWriteLn((char*)epg.info1.c_str());
					request->SocketWriteLn((char*)epg.info2.c_str());					
				}
			}
			else
			{	//eventlist for a chan
				unsigned id = atol( request->ParameterList->Head->Name->c_str() );
				GetEventList( request, id, true);
			}

		}
/*
		else if (request->ParameterList->Count == 3)
		{
			long long epgid;
			sscanf(request->ParameterList->GetValue(0), "%llx", &epgid);

			time_t startzeit;
			sscanf
//			GetEPG( request, id, NULL, true);
			CEPGData * epg = new CEPGData;
			if(sectionsd.getEPGid(epgid,*startzeit,epg))
			{
				request->SocketWriteLn((char*)epg.title.c_str());
				request->SocketWriteLn((char*)epg.info1.c_str());
				request->SocketWriteLn((char*)epg.info2.c_str());
			}
			delete epg;

		}
*/
	}
	if(strcmp(request->Filename->c_str(),"version") == 0)
	{
		// aktuelle cramfs version ausgeben
		request->SendPlainHeader();          // Standard httpd header senden
		request->SendFile("/",".version");
	}

	if(strcmp(request->Filename->c_str(),"zapto") == 0)
	{
		request->SendPlainHeader();          // Standard httpd header senden
		if (request->ParameterList->Count == 0)
		{	//paramlos - aktuelles programm anzeigen
			if(Parent->DEBUG) printf("zapto ohne params\n");
			char buf[30];
			sprintf(buf, "%u\n", zapit.getCurrentServiceID());
			request->SocketWrite(buf);
		}
		else
		if (request->ParameterList->Count == 1)
		{
			if(Parent->DEBUG) printf("Zapto mit einem Parameter: '%s'\n",request->ParameterList->Head->Name->c_str());
			if(request->ParameterList->GetIndex("getpids") != -1)
			{
				SendcurrentVAPid(request);
			}
			else if(request->ParameterList->GetIndex("stopplayback") != -1)
			{
				zapit.stopPlayBack();
				sectionsd.setPauseScanning(true);
				request->SocketWrite("ok");
				if(Parent->VERBOSE) printf("stop playback requested..\n");
			}
			else if(request->ParameterList->GetIndex("startplayback") != -1)
			{
				zapit.startPlayBack();
				sectionsd.setPauseScanning(false);
				if(Parent->VERBOSE) printf("start playback requested..\n");
				request->SocketWrite("ok");
			}
			else if(request->ParameterList->GetIndex("stopsectionsd") != -1)
			{
				sectionsd.setPauseScanning(true);
				if(Parent->VERBOSE) printf("stop sectionsd requested..\n");
				request->SocketWrite("ok");
			}
			else if(request->ParameterList->GetIndex("startsectionsd") != -1)
			{
				sectionsd.setPauseScanning(false);
				if(Parent->VERBOSE) printf("stop sectionsd requested..\n");
				request->SocketWrite("ok");
			}
			else
			{
				ZapTo( request->ParameterList->Head->Name->c_str() );
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

bool TWebDbox::Execute(TWebserverRequest* request)
{
	if(Parent->DEBUG) printf("executing\n");
	if(strcmp(request->Filename->c_str(),"test.dbox2") == 0)
	{

		printf("Teste nun\n");
		request->SendPlainHeader("text/html");
		printf("vorher\n");
//		GetChannelEvents();
		printf("fertig\n");
		request->SocketWrite("alles wird gut\n");
		return true;
	}
/*
	if(strcmp(request->Filename->c_str(),"avcontrol.dbox2") == 0)
	{
		request->SendPlainHeader("text/html");
		if (request->ParameterList->Count == 4)
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
	if(strcmp(request->Filename->c_str(),"upload.dbox2") == 0)
	{
		printf("uploade :)\n");
		if(request->Upload != NULL)
		{
			request->Upload->HandleUpload();
		}
		else
			printf("Kein Dateianhang gefunden\n");
		return true;
	}
	if (strcmp(request->Filename->c_str(),"bouquetlist.dbox2") == 0)
	{
		request->SendPlainHeader("text/html");
		ShowBouquets(request);
		return true;
	}
	if (strcmp(request->Filename->c_str(),"channellist.dbox2") == 0)
	{
		request->SendPlainHeader("text/html");
		if(request->ParameterList->Count == 0)
			ShowChannelList(request,ChannelList);
		else if( (request->ParameterList->Count == 1) && ( request->ParameterList->GetIndex("bouquet") != -1) )
		{
			ShowBouquet(request,atoi(request->ParameterList->GetValue(request->ParameterList->GetIndex("bouquet"))));
		}
		return true;
	}
	else
	{
		if (strcmp(request->Filename->c_str(),"controlpanel.dbox2") == 0)
		{	//funktionen für controlpanel links
			request->SendPlainHeader("text/html");
			if (request->ParameterList->Count > 0)
			{
				if( request->ParameterList->GetIndex("volumemute") != -1)
				{
					bool mute = controld.getMute();
					controld.setMute( !mute );
					if(request->Parent->DEBUG) printf("mute\n");
				}
				else if( request->ParameterList->GetIndex("volumeplus") != -1)
				{
					char vol = controld.getVolume();
					vol+=5;
					if (vol>100)
						vol=100;
					controld.setVolume(vol);
					if(request->Parent->DEBUG) printf("Volume plus: %d\n",vol);
				}
				else if( request->ParameterList->GetIndex("volumeminus") != -1)
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
			request->SendFile(request->Parent->PrivateDocumentRoot->c_str(),"/controlpanel.include1");
			//muted
			request->SocketWrite(mutestr);
			request->SendFile(request->Parent->PrivateDocumentRoot->c_str(),"/controlpanel.include2");
			//volume bar...
			request->SocketWrite("<td><img src=../images/vol_flashed.jpg width=");
			request->SocketWrite(volstr_on);
			request->SocketWrite(" height=10 border=0><br></td>\n");
			request->SocketWrite("<td><img src=../images/vol_unflashed.jpg width=");
			request->SocketWrite(volstr_off);
			request->SocketWrite(" height=10 border=0><br></td>\n");
			request->SendFile(request->Parent->PrivateDocumentRoot->c_str(),"/controlpanel.include3");
			delete[] mutestr;
			return true;

		}
		else if (strcmp(request->Filename->c_str(),"epg.dbox2") == 0)
		{
			if(request->ParameterList->Count > 0)
			{											// wenn parameter vorhanden sind
				request->SendHTMLHeader("EPG");

				request->SocketWriteLn("<html>\n<head><title>DBOX2-Neutrino EPG</title><link rel=\"stylesheet\" type=\"text/css\" href=\"../channellist.css\"></head>");
				//request->println("<script> function openWindow(theURL) { window.open(theURL,'Neutrino-EPG','scrollbars=no,width=100,height=100');}</script>");
				request->SocketWriteLn("<body>");

				if(request->ParameterList->GetIndex("channel") != -1)
				{
					unsigned channel_id = atol( request->ParameterList->GetValue(request->ParameterList->GetIndex("channel")) );
//					GetCurrentEPG(request,channel_id);
					return true;
				}
				else if(request->ParameterList->GetIndex("epgid") != -1)
				{
					unsigned long long epgid;
					time_t startzeit;

					char * idstr = request->ParameterList->GetValue(request->ParameterList->GetIndex("epgid"));
					sscanf(idstr, "%llx", &epgid);

					char * timestr = request->ParameterList->GetValue(request->ParameterList->GetIndex("startzeit"));
					sscanf(timestr, "%x", &startzeit);
	
					char *buffer2 = new char[1024];
					CEPGData epg;
					if(sectionsd.getEPGid(epgid,startzeit,&epg))
					{
						sprintf(buffer2,"<H1>%s</H1><BR>\n<H2>%s</H2><BR>\n<B>%s</B><BR>\n\0",epg.title.c_str(),epg.info1.c_str(),epg.info2.c_str());
						request->SocketWrite(buffer2);
					}

				}
				else
					printf("Fehler\n");
				request->SendHTMLFooter();
				return true;
			}
		}
/*
		else if (strcmp(request->Filename->c_str(),"timer.dbox2") == 0)
		{
			printf("Timer request\n");

			if(request->ParameterList->Count > 0)
			{
				if( (request->ParameterList->GetIndex("set") != -1) && (request->ParameterList->GetIndex("time") != -1) && (request->ParameterList->GetIndex("channel") != -1) )
				{
					int zeit = atoi(request->ParameterList->GetValue(request->ParameterList->GetIndex("time")));
					char *channel = request->ParameterList->GetValue(request->ParameterList->GetIndex("channel"));
					printf("Channel ist '%s'\n",channel);
					request->Parent->TimerList->Add(zeit,tChannel,atol(channel));

					printf("Zur Liste hinzugefügt\n");

					request->SocketWriteLn("HTTP/1.0 302 Moved Temporarily");
					request->SocketWrite("Location: switch.dbox2?eventlist=");
					request->SocketWriteLn(request->ParameterList->GetValue(request->ParameterList->GetIndex("channel")));
					request->SocketWriteLn("Content-Type: text/html\n");
					return true;
				}
				else
				{
					request->SocketWriteLn("Parameter Fehler");
					return false;
				}
			}
			else
			{
				request->SendPlainHeader("text/html");
				ShowTimerList(request);
				return true;
			}
		}
*/
		else if (strcmp(request->Filename->c_str(),"switch.dbox2") == 0)
		{
			if(request->ParameterList->Count > 0)
			{
				if(request->ParameterList->GetIndex("zapto") != - 1)
				{
					ZapTo(request->ParameterList->GetValue(request->ParameterList->GetIndex("zapto")) );
					request->SocketWriteLn("HTTP/1.0 302 Moved Temporarily");
					request->SocketWriteLn("Location: channellist.dbox2#akt");
					return true;
				}

				if(request->ParameterList->GetIndex("eventlist") != -1)
				{
					request->SendPlainHeader("text/html");
					unsigned id = atol( request->ParameterList->GetValue(request->ParameterList->GetIndex("eventlist")) );
					GetEventList( request, id );
					return true;
				}

				if(request->ParameterList->GetIndex("shutdown") != -1)
				{
					request->SendPlainHeader("text/html");
					request->SendFile(request->Parent->PrivateDocumentRoot->c_str(),"/shutdown.include");
					request->EndRequest();
					sleep(1);
					controld.shutdown();
					return true;
				}

				if(request->ParameterList->GetIndex("tvmode") != -1)
				{
					zapit.setMode(CZapitClient::MODE_CURRENT);
					if(request->Parent->DEBUG) printf("ok");
					return true;
				}

				if(request->ParameterList->GetIndex("radiomode") != -1)
				{
					zapit.setMode(CZapitClient::MODE_RADIO);
					if(request->Parent->DEBUG) printf("ok");
					return true;
				}

				if(request->ParameterList->GetIndex("settings") != -1)
				{
					if(request->Parent->DEBUG) printf("settings\n");
					SendSettings(request);
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
void TWebDbox::SendBouquets(TWebserverRequest *request)
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
void TWebDbox::SendBouquet(TWebserverRequest *request,int BouquetNr)
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
void TWebDbox::SendChannelList(TWebserverRequest *request)
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
void TWebDbox::SendStreaminfo(TWebserverRequest* request)
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
			//printf("%s: %d\n",key,value);
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

void TWebDbox::SendcurrentVAPid(TWebserverRequest* request)
{
CZapitClient::responseGetPIDs pids;
	zapit.getPIDS(pids);

	char *buf = new char[300];
	printf("%u\n%u\n", pids.PIDs.vpid, pids.APIDs[0].pid);
	sprintf(buf, "%u\n%u\n", pids.PIDs.vpid, pids.APIDs[0].pid);
	request->SocketWrite(buf);
	delete buf;
}

//-------------------------------------------------------------------------
void TWebDbox::SendSettings(TWebserverRequest* request)
{
	request->SendPlainHeader("text/html");
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
	if(Parent->DEBUG) printf("Ende SendSettings\n");
}

//-------------------------------------------------------------------------
void TWebDbox::UpdateBouquets(void)
{
	GetBouquets();
	for(int i = 1; i <= BouquetList.size();i++)
		GetBouquet(i);
	GetChannelList();
}


//-------------------------------------------------------------------------
void TWebDbox::ShowBouquets(TWebserverRequest *request)
{
char *buffer = new char[500];
//	request->SocketWriteLn("<html>\n<head><title>DBOX2-Neutrino Bouquetliste</title><link rel=\"stylesheet\" type=\"text/css\" href=\"../channellist.css\"></head>");
	request->SendHTMLHeader("DBOX2-Neutrino Bouquetliste");
	request->SocketWriteLn("<table cellspacing=0 cellpadding=0 border=0>\n");
	sprintf(buffer,"<tr><td><a class=b href=\"channellist.dbox2\" target=\"content\"><h5>Alle Kanäle</h5></a></td></tr>\n");
	request->SocketWrite(buffer);

	for(int i = 0; i < BouquetList.size();i++)
	{
		sprintf(buffer,"<tr><td><h5><a class=b href=\"channellist.dbox2?bouquet=%d\" target=\"content\">%s</a></h5></td></tr>\n",BouquetList[i].bouquet_nr,BouquetList[i].name);
		request->SocketWrite(buffer);
	}
	sprintf(buffer,"</table>\n");
	request->SocketWrite(buffer);
	request->SendHTMLFooter();
	delete[] buffer;
}

//-------------------------------------------------------------------------
void TWebDbox::ShowBouquet(TWebserverRequest *request,int BouquetNr)
{
	if(BouquetNr < 10)		// hmm, das muss geändert werden
	{
		ShowChannelList(request,BouquetsList[BouquetNr]);
	}
	else
		printf("BouquetNr out of range\n");
};

//-------------------------------------------------------------------------
void TWebDbox::ShowChannelList(TWebserverRequest* request,CZapitClient::BouquetChannelList channellist)
{
	GetChannelEvents();

	request->SendHTMLHeader("DBOX2-Neutrino Kanalliste");

	request->SocketWriteLn("<table cellspacing=\"0\">");

	int i = 1;
	char classname[] = "a\0";
	char buffer[400];
	int current_channel = zapit.getCurrentServiceID();
	for(int i = 0; i < channellist.size();i++)
	{
		TParameterList *params = new TParameterList;
		classname[0] = (i&1)?'a':'b';
		if(channellist[i].onid_sid == current_channel)
		{
			classname[0] = 'c';
			params->Add("AKT","<a name=akt></a>");
		}
		else
			params->Add("AKT"," ");

		params->Add("CLASS",classname);
		char id[10] ={0};
		sprintf(id,"%d",channellist[i].onid_sid);
		params->Add("CHANNEL_ID",id);
		params->Add("CHANNEL_NAME",channellist[i].name);
		char nr[3] ={0};
		sprintf(nr,"%d",i + 1);
		params->Add("CHANNEL_NR",nr);


		char teststr[] = "<tr><td class=\"%%CLASS%%\">%%AKT%%<a href=\"switch.dbox2?zapto=%%CHANNEL_ID%%\">%%CHANNEL_NR%%. %%CHANNEL_NAME%%</a> </td></tr>\n\0";
		ParseString(request,teststr,params);
		delete params;
		if(ChannelListEvents[channellist[i].onid_sid])
		{	
			sprintf(buffer,"<tr><td class=\"%cepg\"><a href=epg.dbox2?epgid=%llx&startzeit=%lx>",classname[0],ChannelListEvents[channellist[i].onid_sid]->eventID,ChannelListEvents[channellist[i].onid_sid]->startTime);
			request->SocketWrite(buffer);
			request->SocketWrite((char *)ChannelListEvents[channellist[i].onid_sid]->description.c_str());
			request->SocketWrite("</a>&nbsp;");
			sprintf(buffer,"<font size=-3>(%d von %d min, %d\%)</font>&nbsp;<a href=\"switch.dbox2?eventlist=%u\"><img src=\"../images/elist.gif\" border=\"0\"></a></td></tr>\n",(time(NULL)-ChannelListEvents[channellist[i].onid_sid]->startTime)/60,ChannelListEvents[channellist[i].onid_sid]->duration / 60,100 * (time(NULL)-ChannelListEvents[channellist[i].onid_sid]->startTime) / ChannelListEvents[channellist[i].onid_sid]->duration, channellist[i].onid_sid  );
			request->SocketWrite(buffer);
		}
	}

	request->SocketWriteLn("</table>");

	request->SendHTMLFooter();
}
//-------------------------------------------------------------------------
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
//-------------------------------------------------------------------------
void TWebDbox::GetChannelEvents()
{
	
	eList = sectionsd.getChannelEvents();
	CChannelEventList::iterator eventIterator;

    for( eventIterator = eList.begin();
		 eventIterator != eList.end();
		 eventIterator++ )
	{
//		(*eventIterator).serviceID;
		ChannelListEvents[(*eventIterator).serviceID] = &(*eventIterator);
	}

}
//-------------------------------------------------------------------------

void TWebDbox::GetEventList(TWebserverRequest *request,unsigned onidSid, bool cgi = false)
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
				request->SocketWrite("<h3>Eventlist: ");
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
//				printf("epgID: '%s' edate: '%s' etime: '%s' ename: '%s'\n",epgID,edate,etime,ename);

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

						sprintf(buf, "<tr><td class=\"%c\">%s, %s <small>(%smin)</small></tr></td>", classname, edate, etime, eduration,zeit,onidSid);
						sprintf(&buf[strlen(buf)], "<tr><td class=\"%c\"><a href=epg.dbox2?epgid=%s>%s</a></td></tr>\n", classname,epgID, ename);
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
//			request->SocketWriteLn("</body></html>");
		}
	}

}

//-------------------------------------------------------------------------
/*
void TWebDbox::GetEPG(TWebserverRequest *request,unsigned long long epgid,time_t *startzeit,bool cgi = false)
{
	char *buffer2 = new char[1024];
	CEPGData * epg = new CEPGData;
	if(sectionsd.getEPGid(epgid,*startzeit,epg))
	{
		if(cgi)
			sprintf(buffer2, "%s\n%s\n%s\n", epg->title.c_str(), epg->info1.c_str(), epg->info2.c_str());
		else
			sprintf(buffer2,"<H1>%s</H1><BR>\n<H2>%s</H2><BR>\n<B>%s</B><BR>\n\0",epg->title.c_str(),epg->info1.c_str(),epg->info2.c_str());
		request->SocketWrite(buffer2);
	}
	delete epg;
}
*/
//-------------------------------------------------------------------------

char* TWebDbox::GetServiceName(int onid_sid)
{
	for(int i = 0; i < ChannelList.size();i++)
		if( ChannelList[i].onid_sid == onid_sid)
			return ChannelList[i].name;
	return "";
}

//-------------------------------------------------------------------------
/*
void TWebDbox::ShowTimerList(TWebserverRequest *request)
{
	request->SocketWrite("<HTML><HEAD></HEAD><BODY><TABLE>\n");
	char buffer[500];
	struct tm *zeit;
	if(Parent->TimerList)
	{
		sprintf(buffer,"<TR><TH>Datum</TH><TH>Startzeit</TH><TH>Kanal</TH><TH>EPGID</TH><TH>erledigt</TH><TR>\n");
		request->SocketWrite(buffer);
		for(int i = 0; i < Parent->TimerList->Count;i++)
		{
			TTimerEvent * event = Parent->TimerList->GetEvent(i);
			if(event)
			{
				zeit = localtime(&event->StartTime);

				sprintf(buffer,"<TR><TD>%02d.%02d.</TD><TD>%02d:%02d</TD><TD>%ld</TD><TD>%ld</TD><TD>%s</TD><TR>\n",zeit->tm_mday,zeit->tm_mon+1,zeit->tm_hour,zeit->tm_min,event->Channel,event->EPGID,event->Processed?"ja":"nein");
				request->SocketWrite(buffer);
			}
		}
	}
	request->SocketWrite("</TABLE></BODY></HTML>\n");

}
*/

//-------------------------------------------------------------------------
