/*
	webserver  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/



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
void TWebDbox::Streaminfo(TWebserverRequest* request)
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
//-------------------------------------------------------------------------

void TWebDbox::StopEPGScanning(TWebserverRequest* request, bool off)
{
int sock_fd;
Tmconnect con;
	if((sock_fd = Parent->SocketConnect(&con,sectionsd::portNumber)) != -1)
	{

		sectionsd::msgRequestHeader req;
		req.version = 2;
		req.command = sectionsd::pauseScanning;
		req.dataLength = 2;
		write(sock_fd, &req, sizeof(req));
		int stopit = off?1:0;
		write(sock_fd, &stopit, req.dataLength);

		close(sock_fd);
	}
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
void TWebDbox::GetcurrentVAPid(TWebserverRequest* request)
{
char return_buf[4] = {0}; 
st_rmsg		sendmessage;
int sock_fd;
Tmconnect con;

	if((sock_fd = Parent->SocketConnect(&con,EZAP_PORT)) != -1)
	{
		sendmessage.version=1;
		sendmessage.cmd = 'b';

		write(sock_fd, &sendmessage, sizeof(sendmessage));

		if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
			perror("recv(zapit)");
			close(sock_fd);
			return;
		}

		if(strcmp(return_buf,"00b")==0)
		{
			short apid = 0;
			short vpid = 0;
			read(sock_fd, &vpid, sizeof(vpid));
			read(sock_fd, &apid, sizeof(apid));
			char buf[30];
			sprintf(buf, "%u\n%u\n", vpid, apid);
			request->SocketWrite(buf);
		}
		else
			request->SocketWrite(return_buf);

		close(sock_fd);
	}
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
void TWebDbox::StopPlayback(TWebserverRequest* request)
{
char return_buf[4]={0};
st_rmsg		sendmessage;
int sock_fd;
Tmconnect con;

	if((sock_fd = Parent->SocketConnect(&con,EZAP_PORT)) != -1)
	{
		sendmessage.version=1;
		sendmessage.cmd = 2;

		write(sock_fd, &sendmessage, sizeof(sendmessage));
		if (recv(sock_fd, return_buf, 3,0) <= 0 ) 
		{
			perror("recv(zapit)");
			close(sock_fd);
			return;
		}
	
		request->SocketWrite(return_buf);

		close(sock_fd);
	}
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

bool TWebDbox::ExecuteCGI(TWebserverRequest* request)
{
	printf("Execute CGI : %s\n",request->Filename->c_str());
	if(strcmp(request->Filename->c_str(),"getdate") == 0)
	{
		request->SendPlainHeader();

		if (request->ParameterList->Count==0)
		{	//paramlos
			char *timestr = new char[50];
			struct timeb tm;
			ftime(&tm);
			strftime(timestr, 20, "%d.%m.%Y\n", localtime(&tm.time) );
			request->SocketWrite(timestr);
			delete[] timestr;
		}
		else
			request->SocketWrite("error");
	}

	if(strcmp(request->Filename->c_str(),"gettime") == 0)
	{
		request->SendPlainHeader();

		if (request->ParameterList->Count==0)
		{	//paramlos
			char *timestr = new char[50];
			struct timeb tm;
			ftime(&tm);
			strftime(timestr, 20, "%H:%M:%S\n", localtime(&tm.time) );
			request->SocketWrite(timestr);
		}
		else
			request->SocketWrite("error");
	}

	if(strcmp(request->Filename->c_str(),"info") == 0)
	{
		request->SendPlainHeader();

		if (request->ParameterList->Count == 0)
		{	//paramlos
			request->SocketWrite("Neutrino NG\n");
		}
		else if (request->ParameterList->GetIndex("streaminfo") != -1)
			Streaminfo(request);
	}
	
	if(strcmp(request->Filename->c_str(),"shutdown") == 0)
	{
		request->SendPlainHeader();

		if (request->ParameterList->Count == 0)
		{	//paramlos
			request->SocketWrite("shutdown");
			controld.shutdown();
		}
		else
		{
			request->SocketWrite("error");
		}
	}

	if(strcmp(request->Filename->c_str(),"volume") == 0)
	{
		request->SendPlainHeader();

		if (request->ParameterList->Count == 0)
		{	//paramlos - aktuelles volume anzeigen
			char buf[10];
			sprintf(buf, "%d", controld.getVolume());
			request->SocketWrite(buf);
		}
		else
		if (request->ParameterList->Count == 1)
		{
			request->SendPlainHeader();
			if(request->ParameterList->GetIndex("mute") != -1)
			{
				request->SocketWrite("mute");
				controld.setMute(true);
			}
			else 
			if(request->ParameterList->GetIndex("unmute") != -1)
			{
				request->SocketWrite("unmute");
				controld.setMute(false);
			}
			else 
			if(request->ParameterList->GetIndex("status") != -1)
			{
				request->SocketWrite( (char *) (controld.getMute()?"1":"0") );
			}
			else
			{	//set volume
				char vol = atol( request->ParameterList->GetValue(0) );
				request->SocketWrite( request->ParameterList->GetValue(0) );
				controld.setVolume(vol);
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
		printf("Gebe channellist aus\n");
		request->SendPlainHeader();
		if(!ChannelList)
			GetChannelList();
		for(int i = 0; i < ChannelList->Count;i++)
		{
			printf("count: %dchannel: %d\n",ChannelList->Count,i);
			TChannel *channel = ChannelList->GetValue(i);
			if(channel)
			{
				sprintf(buf, "%u %s\n", channel->onid_tsid, channel->Name);		
				request->SocketWrite(buf);
			}
		}
	}

	if(strcmp(request->Filename->c_str(), "epg") == 0)
	{
		request->SendPlainHeader();
		if(Parent->DEBUG) printf("EPG, Parameter: %d\n",request->ParameterList->Count);
		if(request->ParameterList->Count == 1)
		{
			request->ParameterList->PrintParameterList();
			printf("Nach PArameter listen\n");
			if(request->ParameterList->GetIndex("eventid") != -1)
			{	//special epg query
				if(Parent->DEBUG) printf("event_id: %s\n",request->ParameterList->GetValue(request->ParameterList->GetIndex("eventid")));
				long long id;
				sscanf( request->ParameterList->GetValue(request->ParameterList->GetIndex("eventid")), "%llx", &id);
				if(Parent->DEBUG) printf("query spezial epg: %llx\n", id);
				GetEPG( request, id, true );
			}
			else
			{	//eventlist for a chan
				unsigned id = atol( request->ParameterList->Head->Name->c_str() );
				GetEventList( request, NULL, id, true);
			}
		}
		else if (request->ParameterList->Count == 3)
		{
			long long id;
			sscanf(request->ParameterList->GetValue(0), "%llx", &id);
			GetEPG( request, id, true);
		}
	}

	if(strcmp(request->Filename->c_str(),"zapto") == 0)
	{
		request->SendPlainHeader();
		if (request->ParameterList->Count == 0)
		{	//paramlos - aktuelles programm anzeigen
			printf("zapto ohne params\n");
			char buf[30];
			sprintf(buf, "%u\n", GetcurrentONIDSID());
			request->SocketWrite(buf);
		}
		else
		if (request->ParameterList->Count == 1)
		{
			printf("Zapto mit einem Parameter: '%s'\n",request->ParameterList->Head->Name->c_str());
			if(request->ParameterList->GetIndex("getpids") != -1)
			{
				GetcurrentVAPid(request);
			}
			else if(request->ParameterList->GetIndex("stopplayback") != -1)
			{
				StopPlayback(request);
				StopEPGScanning(request, true);
				printf("stop playback requested..\n");
			}
			else if(request->ParameterList->GetIndex("startplayback") != -1)
			{
				char buf[30];
				sprintf(buf, "%u\n", GetcurrentONIDSID());
				ZapTo(&buf[0]);
				StopEPGScanning(request, false);
				printf("start playback requested..\n");
			}
			else if(request->ParameterList->GetIndex("stopsectionsd") != -1)
			{
				StopEPGScanning(request, true);
				printf("stop sectionsd requested..\n");
			}
			else if(request->ParameterList->GetIndex("startsectionsd") != -1)
			{
				StopEPGScanning(request, false);
				printf("stop sectionsd requested..\n");
			}
			else
			{
				ZapTo( request->ParameterList->Head->Name->c_str() );
			}
		}
		else
		{
			request->SocketWrite("error");
			printf("[nhttpd] zapto fehler\n");
		}
	}
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

bool TWebDbox::Execute(TWebserverRequest* request)
{
	if(strcmp(request->Filename->c_str(),"test.dbox2") == 0)
	{
		printf("Teste nun\n");
		request->SendPlainHeader("text/html");
		return true;
	}
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

	if(!ChannelList)
		GetChannelList();
	GetEPGList();

	if (strcmp(request->Filename->c_str(),"channellist.dbox2") == 0)
	{
		ShowChannelList(request);
		return true;
	}
	else 
	{
		if (strcmp(request->Filename->c_str(),"controlpanel.dbox2") == 0)
		{	//funktionen für controlpanel links
			if (request->ParameterList->Count > 0)
			{	
//				request->ParameterList->PrintParameterList();
				if( request->ParameterList->GetIndex("volumemute") != -1)
				{
//					printf("Muting . . \n");
					bool mute = controld.getMute();
					controld.setMute( !mute );
					if(request->Parent->DEBUG) printf("mute\n");
				}
				else if( request->ParameterList->GetIndex("volumeplus") != -1)
				{
					char vol = controld.getVolume();
					vol+=5;
					if (vol>100)
					{
						vol=100;
					}
					controld.setVolume(vol);
					 if(request->Parent->DEBUG) printf("Volume plus: %d\n",vol);
				}
				else if( request->ParameterList->GetIndex("volumeminus") != -1)
				{
					char vol = controld.getVolume();
					if (vol>0)
					{
						vol-=5;
					}
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

			char mutestr[500];
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
			return true;

		}
		else if (strcmp(request->Filename->c_str(),"epg.dbox2") == 0)
		{
			if(request->ParameterList->Count > 0)
			{											// wenn parameter vorhanden sind
				request->SendPlainHeader("text/html");
				request->SocketWriteLn("<html>\n<head><title>DBOX2-Neutrino EPG</title><link rel=\"stylesheet\" type=\"text/css\" href=\"../channellist.css\"></head>");
				//request->println("<script> function openWindow(theURL) { window.open(theURL,'Neutrino-EPG','scrollbars=no,width=100,height=100');}</script>");
				request->SocketWriteLn("<body>");

				if(request->ParameterList->GetIndex("channel") != -1)
				{
					unsigned channel_id = atol( request->ParameterList->GetValue(request->ParameterList->GetIndex("channel")) );
					GetCurrentEPG(request,channel_id);
					return true;
				}
				else if(request->ParameterList->GetIndex("epgid") != -1)
				{
					char * idstr = request->ParameterList->GetValue(request->ParameterList->GetIndex("epgid"));
					long long id;
					sscanf(idstr, "%llx", &id);

					GetEPG( request, id );

				}
				else
					printf("Fehler\n");
				request->SocketWriteLn("</body></html>");
				return true;
			}
		}
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
		else if (strcmp(request->Filename->c_str(),"switch.dbox2") == 0) 
		{
			if(request->ParameterList->Count > 0)
			{
				if(request->ParameterList->GetIndex("zapto") != - 1)
				{
					ZapTo(request->ParameterList->GetValue(request->ParameterList->GetIndex("zapto")) );
					request->SocketWriteLn("HTTP/1.0 302 Moved Temporarily");
					request->SocketWriteLn("Location: channellist.dbox2#akt");
//					request->SocketWriteLn("Content-Type: text/html\n");
					return true;
				}
 
				if(request->ParameterList->GetIndex("eventlist") != -1)
				{
					request->SendPlainHeader("text/html");
					unsigned id = atol( request->ParameterList->GetValue(request->ParameterList->GetIndex("eventlist")) );
					GetEventList( request, NULL, id );
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
					ChannelListEvents.clear();
					SetZapitMode(7);
					if(request->Parent->DEBUG) printf("tv");
					ShowChannelList(request);
					return true;
				}
				
				if(request->ParameterList->GetIndex("radiomode") != -1)
				{
					ChannelListEvents.clear();
					SetZapitMode(6);
					if(request->Parent->DEBUG) printf("radio");
					ShowChannelList(request);
					return true;
				}

				if(request->ParameterList->GetIndex("settings") != -1)
				{
					if(request->Parent->DEBUG) printf("settings\n");
					request->SocketWriteLn("HTTP/1.0 200 OK");
					request->SocketWriteLn("Content-Type: text/html\n");
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

TWebDbox::TWebDbox(TWebserver *server)
{
	Parent=server;
	ChannelList = NULL;
	EPGDate = 0;
//	ChannelListEvents.clear();
	if(!ChannelList)
		GetChannelList();
	GetEPGList();

}
//-------------------------------------------------------------------------

TWebDbox::~TWebDbox()
{
	if(ChannelList)
		delete ChannelList;
}
//-------------------------------------------------------------------------
void TWebDbox::ShowSettings(TWebserverRequest* request)
{
	request->SendPlainHeader("text/html");
	char dbox_names[4][10]={"","Nokia","Sagem","Philips"};
	char videooutput_names[3][13]={"Composite","RGB","S-Video"};
	char videoformat_names[3][13]={"automatic","16:9","4:3"};

	char buf[10];
	char boxtype = controld.getBoxType();
	char videooutput = controld.getVideoOutput();
	char videoformat = controld.getVideoFormat();

	request->SocketWrite("<html>\n<body>");

	request->SocketWrite("Boxtype "); 
	request->SocketWrite(dbox_names[boxtype]);
	request->SocketWrite("<br>\n");

	request->SocketWrite("videooutput ");
	request->SocketWrite(videooutput_names[videooutput]);
	request->SocketWrite("<br>\n");

	request->SocketWrite("videoformat ");
	request->SocketWrite(videoformat_names[videoformat]);
	request->SocketWrite("<br>\n");
	
	request->SocketWrite("</body>\n</html>\n");
	printf("Ende ShowSettings\n");
}

//-------------------------------------------------------------------------
void TWebDbox::ShowChannelList(TWebserverRequest* request)
{
	if(!ChannelList)
		if(!GetChannelList())
			return;
	GetEPGList();

	request->SendPlainHeader("text/html");
	
	//show chanlist
	
	request->SocketWriteLn("<html>");
	request->SocketWriteLn("<head><title>DBOX2-Neutrino Channellist</title><link rel=\"stylesheet\" type=\"text/css\" href=\"../channellist.css\"></head>");
	//request->println("<script> function openWindow(theURL) { window.open(theURL,'Neutrino-EPG','scrollbars=no,width=100,height=100');}</script>");

	request->SocketWriteLn("<body>");
	request->SocketWriteLn("<table cellspacing=\"0\">");

	TChannel *channel = ChannelList->Head;
	int i = 1;
	char classname[] = "a\0";
	char buffer[400];
	int current_channel = GetcurrentONIDSID();
	do
	{

		TParameterList *params = new TParameterList;
		
		classname[0] = (i&1)?'a':'b';
		if(channel->onid_tsid == current_channel)
		{
			classname[0] = 'c';
			params->Add("AKT","<a name=akt></a>"); 
		}
		else
			params->Add("AKT"," ");

		params->Add("CLASS",classname);
		char id[10] ={0};
		sprintf(id,"%ld",channel->onid_tsid);
		params->Add("CHANNEL_ID",id);
		params->Add("CHANNEL_NAME",channel->Name);
		char nr[3] ={0};
		sprintf(nr,"%d",i);
		params->Add("CHANNEL_NR",nr);
		

		char teststr[] = "<tr><td class=\"%%CLASS%%\">%%AKT%%<a href=\"switch.dbox2?zapto=%%CHANNEL_ID%%\">%%CHANNEL_NR%%. %%CHANNEL_NAME%%</a> <a href=\"switch.dbox2?eventlist=%%CHANNEL_ID%%\"><img src=\"../images/elist.gif\" border=\"0\"></a></td></tr>\n\0";
		ParseString(request,teststr,params);
		delete params;
	
		if(channel->EPG != NULL)
		{
			sprintf(buffer,"<tr><td class=\"%cepg\"><a href=epg.dbox2?channel=%d>%s</a> <font size=-3>(%d von %d min, %d\%)</font></td></tr>\n",classname[0],channel->onid_tsid,channel->EPG->c_str(),(time(NULL)-channel->Starttime)/60,channel->Duration / 60,100 * (time(NULL)-channel->Starttime) / channel->Duration  );
			request->SocketWrite(buffer);
		}
		i++;
		channel = channel->Next;
	}while(channel && (i < ChannelList->Count));

	request->SocketWriteLn("</table>");
	request->SocketWriteLn("</body></html>");
}
//-------------------------------------------------------------------------

void TWebDbox::GetEPGList()
{
int sock_fd;
Tmconnect con;

	if(!ChannelList)
	{
		printf("Keine Channelliste\n");
		return;
	}

	if((EPGDate + 2 * 60L) > time(NULL))
	{
		if(Parent->DEBUG) printf("EPGListe ist aktuell\n");
		return;
	}
	if(Parent->DEBUG) printf("EPG aktualisieren\n");
	EPGDate = time(NULL);
//-----------------------------
	TChannel *channel = ChannelList->Head;
	int i = 1;
	char classname[] = "a\0";
	char buffer[400];
	int current_channel = GetcurrentONIDSID();
	do
	{
		if(Parent->DEBUG) printf("GetEPG for %s\n",channel->Name);
		if((sock_fd = Parent->SocketConnect(&con,sectionsd::portNumber)) != -1)
		{
			sectionsd::msgRequestHeader req;
			req.version = 2;
			req.command = sectionsd::actualEPGchannelID;
			req.dataLength = 4;
			write(sock_fd,&req,sizeof(req));
			write(sock_fd,&channel->onid_tsid,4);

			sectionsd::msgResponseHeader resp;
			memset(&resp, 0, sizeof(resp));
			if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0)
			{
				if(Parent->DEBUG) printf("nix gelesen\n");
				close(sock_fd);
//				return;
			}
			else
			{
				if(resp.dataLength<=0)
				{
					if(Parent->DEBUG) printf("keine epg Daten\n");
					close(sock_fd);
//					return;
				}
				else
				{
			
					char* pData = new char[resp.dataLength];
					if(Parent->DEBUG) printf("Lese eine %d byte lange EventList\n",resp.dataLength);
					if ( read(sock_fd, pData, resp.dataLength)<=0 )
					{
						delete[] pData;
						close(sock_fd);
//						return;
					}
					else
					{

						char *eventid;
//						char * anfang, *ende;
						char *name,*text,*text2,*contentClassification,*userClassification,fsk;
						char *dp;
						sectionsd::sectionsdTime* epg_times;

						eventid = pData;
						name = pData + sizeof(long long);
						dp = name;

						dp+=strlen(dp)+1;
						text = dp;
						dp+=strlen(dp)+1;
						text2 = dp;
						dp+=strlen(dp)+1;
						contentClassification = dp;
						dp+=strlen(dp)+1;
						userClassification = dp;
						dp+=strlen(dp)+1;
						fsk = *dp++;

						epg_times = (sectionsd::sectionsdTime*) dp;

						struct tm *pStartZeit = localtime(&(*epg_times).startzeit);
//						printf("Startzeit %s Dauer: %ld\n",asctime(pStartZeit),(*epg_times).dauer);
						channel->Starttime = (*epg_times).startzeit;
						channel->Duration = (*epg_times).dauer;

//						printf("ID: '%s' NAME: '%s' TEXT: '%s'\n",eventid,name,text); 
						if(channel->EPG) delete channel->EPG;
						channel->EPG = new TString(name);

						if(channel->ExtendedEPG) delete channel->ExtendedEPG;
						channel->ExtendedEPG = new TString(text);

						delete[] pData;
					}
				}
			}	
		}
			
		channel = channel->Next;
		i++;
	}while(channel && (i < ChannelList->Count));

}
//-------------------------------------------------------------------------

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

//				printf("<TR><TD>%ld</TD><TD>%ld</TD><TD>%ld</TD><TR>\n",event->StartTime,event->Channel,event->EPGID);
				sprintf(buffer,"<TR><TD>%02d.%02d.</TD><TD>%02d:%02d</TD><TD>%ld</TD><TD>%ld</TD><TD>%s</TD><TR>\n",zeit->tm_mday,zeit->tm_mon+1,zeit->tm_hour,zeit->tm_min,event->Channel,event->EPGID,event->Processed?"ja":"nein");
				request->SocketWrite(buffer);
			}
		}
	}
	request->SocketWrite("</TABLE></BODY></HTML>\n");

}
//-------------------------------------------------------------------------

void TWebDbox::ZapTo(char *target)
{
int sock_fd;
Tmconnect con;

	int sidonid = atol(target);
	if(sidonid == GetcurrentONIDSID())
	{
		if(Parent->DEBUG) printf("Kanal ist aktuell\n");
		return;
	}
	if((sock_fd = Parent->SocketConnect(&con,EZAP_PORT)) != -1)
	{
		st_rmsg	sendmessage;

		if(Parent->DEBUG) printf("Schalte auf Kanal %s\n",target);
		sendmessage.version=1;
		sendmessage.cmd = 'd';
		snprintf( (char*) &sendmessage.param3, 10, "%x", sidonid);

		write(sock_fd, &sendmessage, sizeof(sendmessage));
		char return_buf[4] = {0};
		if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
			perror("recv(zapit)");
			close(sock_fd);
			return;
		}
			close(sock_fd);
	}
}
void TWebDbox::GetEventList(TWebserverRequest *request,TEventList *Events,unsigned onidSid, bool cgi = false)
{
char epgID[20];
char edate[6];
char etime[6];
char eduration[10];
char ename[100];
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
				request->SocketWriteLn("<html>");
				request->SocketWriteLn("<head><title>DBOX2-Neutrino Channellist</title><link rel=\"stylesheet\" type=\"text/css\" href=\"../eventlist.css\"></head>");
				request->SocketWriteLn("<body>");

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
				*epgID=0;
				actPos = copyStringto( actPos, epgID, sizeof(epgID), ' ');
				*edate=0;
				actPos = copyStringto( actPos, edate, sizeof(edate), ' ');
				*etime=0;
				actPos = copyStringto( actPos, etime, sizeof(etime), ' ');
				*eduration=0;
				actPos = copyStringto( actPos, eduration, sizeof(eduration), ' ');
				*ename=0;
				actPos = copyStringto( actPos, ename, sizeof(ename), '\n');

				char *buf = new char[1400];
//				printf( "EPGID:%-15s Datum: %s Zeit: %s Dauer: %s Name: %s\n", epgID, edate, etime, eduration, ename);
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
//				printf("zeit: %s rawzeit = %ld\n",ctime(&zeit),zeit);
				if((zeit + (atoi(eduration) * 60)) >= time(NULL))
				{	
					if(cgi)
					{
						sprintf(buf, "%s %s %s %s %s\n", epgID, edate, etime, eduration, ename);
					}
					else
					{
						classname = (pos&1)?'a':'b';
						
//						sprintf(buf, "<tr><td class=\"%c\">%s, %s <small>(%smin)</small> <a href=\"timer.dbox2?set&time=%ld&channel=%ld\"><IMG SRC=\"../images/timer.gif\" WIDTH=21 HEIGHT=21 BORDER=0 ALT=\"Timer\"></a></tr></td>", classname, edate, etime, eduration,zeit,onidSid);
						sprintf(buf, "<tr><td class=\"%c\">%s, %s <small>(%smin)</small></tr></td>", classname, edate, etime, eduration,zeit,onidSid);
						sprintf(&buf[strlen(buf)], "<tr><td class=\"%ctext\"><a href=epg.dbox2?epgid=%s>%s</a></td></tr>\n", classname,epgID, ename);
					}
					request->SocketWrite(buf);
					pos++;
				}
				delete[] buf;
			}

			delete[] buffer;
		}
		close(sock_fd);

		request->SocketWriteLn("</table>");
		request->SocketWriteLn("</body></html>");
	}
}
//-------------------------------------------------------------------------

void TWebDbox::GetCurrentEPG(TWebserverRequest *request,unsigned onidSid)
{
int sock_fd;
Tmconnect con;
	if((sock_fd = Parent->SocketConnect(&con,sectionsd::portNumber)) != -1)
	{
		printf("Connected to sectionsd\n");
		sectionsd::msgRequestHeader req;
	//	printf("Versuche Daten zu lesen\n");
		req.version = 2;
		req.command = sectionsd::actualEPGchannelID;
		req.dataLength = 4;
		write(sock_fd, &req, sizeof(req));
		write(sock_fd, &onidSid, req.dataLength);

		sectionsd::msgResponseHeader resp;
		memset(&resp, 0, sizeof(resp));
		if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0) 
		{
			printf("Fehler in read\n");
			close(sock_fd);
			return;
		}
		if ( resp.dataLength>0 )
		{
			char* buffer = new char[resp.dataLength] ;
			int buffer_len;
			if( (buffer_len = read(sock_fd, buffer, resp.dataLength)) <= 0)
			{
				printf("Fehler\n");
				return;
			}
			printf("%d Bytes gelesen\n",buffer_len);
			char *titel,*titel2,*text;
			titel = buffer + sizeof(long long);
			titel2 = titel + strlen(titel) + 1;
			text = titel2 + strlen(titel2) + 1;
			sectionsd::sectionsdTime *epg_times = (sectionsd::sectionsdTime*) (text + strlen(text) + 1);
			
			char *buffer2 = new char[buffer_len + 400];
			buffer[0]=0;
//			sprintf(buffer2,"Titel: %s<BR>\nTitel2: %s<BR>\nText: %s<BR>\n<BR>\n\0",titel,titel2,text);
			sprintf(buffer2,"<H1>%s</H1><BR>\n<H2>%s</H2><BR>\n<B>%s</B><BR>\0",titel,titel2,text);
			request->SocketWrite(buffer2);

			delete[] buffer2;
			delete[] buffer;
		}
		close(sock_fd);
	}
}
//-------------------------------------------------------------------------

void TWebDbox::GetEPG(TWebserverRequest *request,long long epgid,bool cgi = false)
{
int sock_fd;
Tmconnect con;

	if((sock_fd = Parent->SocketConnect(&con,sectionsd::portNumber)) != -1)
	{

//		printf("Connected to sectionsd\n");
		sectionsd::msgRequestHeader req;
	//	printf("Versuche Daten zu lesen\n");
		req.version = 2;
		req.command = sectionsd::epgEPGidShort;
		req.dataLength = 8;
		write(sock_fd, &req, sizeof(req));
		write(sock_fd, &epgid, req.dataLength);

		sectionsd::msgResponseHeader resp;
		memset(&resp, 0, sizeof(resp));
		if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0) 
		{
			printf("Fehler in read\n");
			close(sock_fd);
			return;
		}
		Parent->DEBUG=true;
		if(Parent->DEBUG) printf("GetEPG: response gelesen:%ld\n",resp.dataLength);
		if ( resp.dataLength>0 )
		{
			char* buffer = new char[resp.dataLength] ;
			int buffer_len;
			if( (buffer_len = read(sock_fd, buffer, resp.dataLength)) <= 0)
			{
				printf("Fehler\n");
				return;
			}
			if(Parent->DEBUG) printf("GetEPG %d Bytes gelesen\n",buffer_len);
			char * anfang, *ende;
			char *title,*info1,*info2;
			int i;
			title = buffer;
			for(i = 0; (i < buffer_len) && (buffer[i] != 255);i++);
			buffer[i] = 0;
			info1 = &buffer[i+1];
			for(i = i + 1; (i < buffer_len) && (buffer[i] != 255);i++);
			buffer[i] = 0;
			info2 = &buffer[i+1];
			for(i = i + 1; (i < buffer_len) && (buffer[i] != 255);i++);
			buffer[i] = 0;

			char *buffer2 = new char[buffer_len + 400];
			buffer2[0]=0;
			if(cgi)
				sprintf(buffer2, "%s\n%s\n%s\n", title, info1, info2);
			else
				sprintf(buffer2,"<H1>%s</H1><BR>\n<H2>%s</H2><BR>\n<B>%s</B><BR>\n\0",title,info1,info2);
			request->SocketWrite(buffer2);
	//		printf(buffer2);

			delete[] buffer2;
			delete[] buffer;
		}
		close(sock_fd);
	}
}
//-------------------------------------------------------------------------

unsigned int TWebDbox::GetcurrentONIDSID()
{
unsigned int curOnidSid = 0;
char return_buf[4] = {0};
int sock_fd;
Tmconnect con;

	if((sock_fd = Parent->SocketConnect(&con,EZAP_PORT)) != -1)
	{
		st_rmsg sendmessage;

		sendmessage.version=1;
		sendmessage.cmd = 's';

		write(sock_fd, &sendmessage, sizeof(sendmessage));

		if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
			perror("recv(zapit)");
			exit(-1);
		}

		if(strcmp(return_buf,"00s")==0)
			read(sock_fd, &curOnidSid, sizeof(curOnidSid));

		close(sock_fd);
		return curOnidSid;
	}
}
//-------------------------------------------------------------------------

void TWebDbox::SetZapitMode(int mode)
{
st_rmsg sendmessage;
char return_buf[4] = {0};
int sock_fd;
Tmconnect con;

	if((sock_fd = Parent->SocketConnect(&con,EZAP_PORT)) != -1)
	{

		sendmessage.version=1;
		sendmessage.cmd = mode;


		write(sock_fd, &sendmessage, sizeof(sendmessage));
		if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
			perror("recv(zapit)");
			exit(-1);
		}
		close(sock_fd);
	}
}

//-------------------------------------------------------------------------

char* TWebDbox::GetServiceName(int onidsid)
{
	if(!ChannelList)
	{
		if(!GetChannelList())
			return "";
	}
	TChannel * channel = ChannelList->Head;
	do
	{
		if( channel->onid_tsid == onidsid)
		{
//			printf("Gefunden\n");
			return channel->Name;
		}
		channel = channel->Next;
	}while(channel);
//	printf("nicht gefunden\n");
	return "";
}
//-------------------------------------------------------------------------

bool TWebDbox::GetChannelList()
{
int sock_fd;
Tmconnect con;

	if((sock_fd = Parent->SocketConnect(&con,EZAP_PORT)) != -1)
	{
	    channel_msg_2   zapitchannel;
		st_rmsg		sendmessage;
		TChannel *channel = NULL,*last = NULL;
		int i;
		
		sendmessage.version=1;
        // neu! war 5, mit neuem zapit holen wir uns auch die onid_tsid
		sendmessage.cmd = 'c';
		
		write(sock_fd, &sendmessage, sizeof(sendmessage));
		char result[4]={0};

		if (recv(sock_fd, result, 3,0) <= 0 ) {
			perror("recv(zapit)");
			return false;
		}
//		printf("GetChannelList: result:%s\n",result);

		if ( strncmp(result, "00c",3)!= 0 )
		{
			printf("Wrong Command was send for channelsInit(). Exiting.\n");
			return false;
		}

		memset(&zapitchannel,0,sizeof(zapitchannel));

		if(!ChannelList)
			ChannelList = new TChannelList;
//		printf("GetChannelList\n");
		i = 0;
		while (recv(sock_fd, &zapitchannel, sizeof(zapitchannel),0)>0)
        {
			
			if(Parent->DEBUG) printf("%u %s\n", zapitchannel.onid_tsid, zapitchannel.name);
			channel= new TChannel(zapitchannel.chan_nr,zapitchannel.onid_tsid,zapitchannel.name);
			ChannelList->Add(channel,last);
			last = channel;
			i++;
		}
		if(Parent->DEBUG) printf("Got %ld Channels\n",i);
		return true;
	}
}
