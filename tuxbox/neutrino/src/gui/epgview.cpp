/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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

#include "epgdata.h"
#include "../global.h"

CEpgData::CEpgData()
{
}

void CEpgData::start()
{

	ox = 540;
    sx = (((g_settings.screen_EndX-g_settings.screen_StartX) -ox) / 2) + g_settings.screen_StartX;
    oy = 320;
	topheight= g_Fonts->epg_title->getHeight();
	topboxheight=topheight+6;
	botheight=g_Fonts->epg_date->getHeight();
	botboxheight=botheight+6;
	medlineheight=g_Fonts->epg_info1->getHeight();
	medlinecount=(oy- botboxheight)/medlineheight;

	oy = botboxheight+medlinecount*medlineheight; // recalculate
	sy = (((g_settings.screen_EndY-g_settings.screen_StartY)-(oy- topboxheight) ) / 2) + g_settings.screen_StartY;
	toph = topboxheight;

}

void CEpgData::addTextToArray( string text  )
{
	//printf("line: >%s<\n", text.c_str() );
	if (text==" ")
	{
		emptyLineCount ++;
		if(emptyLineCount<2)
		{
			epgText.insert(epgText.end(), text );
		}
	}
	else
	{
		emptyLineCount = 0;
		epgText.insert(epgText.end(), text );
	}
}

void CEpgData::processTextToArray( string text )
{
	string	aktLine = "";
	string	aktWord = "";
	int	aktWidth = 0;
	text+= " ";
	char* text_= (char*) text.c_str();

	while(*text_!=0)
	{
		if ( (*text_==' ') || (*text_=='\n') || (*text_=='-') || (*text_=='.') )
		{
			//check the wordwidth - add to this line if size ok
			if(*text_=='\n')
			{	//enter-handler
				//printf("enter-");
				addTextToArray( aktLine );
				aktLine = "";
				aktWidth= 0;
			}
			else
			{
				aktWord += *text_;

				int aktWordWidth = g_Fonts->epg_info2->getRenderWidth(aktWord.c_str());
				if((aktWordWidth+aktWidth)<(ox- 20- 15))
				{//space ok, add
					aktWidth += aktWordWidth;
					aktLine += aktWord;
				}
				else
				{//new line needed
					addTextToArray( aktLine );
					aktLine = aktWord;
					aktWidth = aktWordWidth;
				}
				aktWord = "";
			}
		}
		else
		{
			aktWord += *text_;
		}
		text_++;
	}
	//add the rest
	addTextToArray( aktLine + aktWord );
}

void CEpgData::showText( int startPos, int ypos )
{
	int textCount = epgText.size();
	int y=ypos;
	int linecount=medlinecount;
	string t;
	int sb = linecount* medlineheight;
	g_FrameBuffer->paintBoxRel(sx, y, ox- 15, sb, COL_MENUCONTENT);

	for(int i=startPos; i<textCount && i<startPos+linecount; i++,y+=medlineheight)
	{
		t=epgText[i];
		if ( i< info1_lines )
			g_Fonts->epg_info1->RenderString(sx+10, y+medlineheight, ox- 15- 15, t.c_str(), COL_MENUCONTENT);
		else
			g_Fonts->epg_info2->RenderString(sx+10, y+medlineheight, ox- 15- 15, t.c_str(), COL_MENUCONTENT);
	}

	g_FrameBuffer->paintBoxRel(sx+ ox- 15, ypos, 15, sb,  COL_MENUCONTENT+ 1);

	int sbc= ((textCount- 1)/ linecount)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (startPos+ 1)/ linecount;

	g_FrameBuffer->paintBoxRel(sx+ ox- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);

}

string GetGenre( char contentClassification )
{
	string res= "UNKNOWN";
	char subClass[2];
	sprintf( subClass, "%d", (contentClassification&0x0F) );

	switch (contentClassification&0x0F0)
	{
		case 0x010: {
						res="MOVIE.";
						if ( (contentClassification&0x0F)< 9 )
							res+= subClass;
						else
							res+= "0";
					 	break;
					}
		case 0x020: {
						res="NEWS.";
						if ( (contentClassification&0x0F)< 5 )
							res+= subClass;
						else
							res+= "0";
					 	break;
					}
		case 0x030: {
						res="SHOW.";
						if ( (contentClassification&0x0F)< 4 )
							res+= subClass;
						else
							res+= "0";
					 	break;
					}
		case 0x040: {
						res="SPORTS.";
						if ( (contentClassification&0x0F)< 12 )
							res+= subClass;
						else
							res+= "0";
					 	break;
					}
		case 0x050: {
						res="CHILDRENs_PROGRAMMES.";
						if ( (contentClassification&0x0F)< 6 )
							res+= subClass;
						else
							res+= "0";
					 	break;
					}
		case 0x060: {
						res="MUSIC_DANCE.";
						if ( (contentClassification&0x0F)< 7 )
							res+= subClass;
						else
							res+= "0";
					 	break;
					}
		case 0x070: {
						res="ARTS.";
						if ( (contentClassification&0x0F)< 12 )
							res+= subClass;
						else
							res+= "0";
					 	break;
					}
		case 0x080: {
						res="SOZIAL_POLITICAL.";
						if ( (contentClassification&0x0F)< 4 )
							res+= subClass;
						else
							res+= "0";
					 	break;
					}
		case 0x090: {
						res="DOCUS_MAGAZINES.";
						if ( (contentClassification&0x0F)< 8 )
							res+= subClass;
						else
							res+= "0";
					 	break;
					}
		case 0x0A0: {
						res="TRAVEL_HOBBIES.";
						if ( (contentClassification&0x0F)< 8 )
							res+= subClass;
						else
							res+= "0";
					 	break;
					}
	}
	return g_Locale->getText("GENRE."+res);
}


int CEpgData::show( string channelName, unsigned int onid_tsid, unsigned long long id, time_t* startzeit, bool doLoop )
{
	int res = menu_return::RETURN_REPAINT;

	int height;
	height = g_Fonts->epg_date->getHeight();
	if (doLoop)
	{
		g_FrameBuffer->paintBoxRel(g_settings.screen_StartX, g_settings.screen_StartY, 50, height+5, COL_INFOBAR);
		g_Fonts->epg_date->RenderString(g_settings.screen_StartX+10, g_settings.screen_StartY+height, 40, "-@-", COL_INFOBAR);
	}

	GetEPGData( channelName, onid_tsid, id, startzeit );
	if (doLoop)
	{
		g_FrameBuffer->paintBackgroundBoxRel(g_settings.screen_StartX, g_settings.screen_StartY, 50, height+5);
	}

	if(epgData.title.length()==0)
	{
		//no epg-data found :(
		char *text = (char*) g_Locale->getText("epgviewer.notfound").c_str();
		int oy = 30;
		int ox = g_Fonts->epg_info2->getRenderWidth(text)+30;
		int sx = (((g_settings.screen_EndX- g_settings.screen_StartX)-ox) / 2) + g_settings.screen_StartX;
		int sy = (((g_settings.screen_EndY- g_settings.screen_StartY)-oy) / 2) + g_settings.screen_StartY;
		height = g_Fonts->epg_info2->getHeight();
		g_FrameBuffer->paintBoxRel(sx, sy, ox, height+ 10, COL_INFOBAR_SHADOW); //border
		g_FrameBuffer->paintBoxRel(sx+ 1, sy+ 1, ox- 2, height+ 8, COL_MENUCONTENT);
		g_Fonts->epg_info2->RenderString(sx+15, sy+height+5, ox-30, text, COL_MENUCONTENT);


		uint msg; uint data;
		g_RCInput->getMsg( &msg, &data, 20 );
		neutrino->handleMsg( msg, data );

		g_FrameBuffer->paintBackgroundBoxRel(sx, sy, ox, height+10);
		return res;
	}


	int pos;
	string text1 = epgData.title;
	string text2 = "";
	if ( g_Fonts->epg_title->getRenderWidth(text1.c_str())> 520 )
    {
    	do
    	{
			pos = text1.find_last_of("[ .]+");
			if ( pos!=-1 )
				text1 = text1.substr( 0, pos );
		} while ( ( pos != -1 ) && ( g_Fonts->epg_title->getRenderWidth(text1.c_str())> 520 ) );
        text2 = epgData.title.substr(text1.length()+ 1, uint(-1) );
	}

	int oldtoph= toph;

	if (text2!="")
		toph = 2* topboxheight;
	else
		toph = topboxheight;


	if ( (oldtoph> toph) && (!doLoop) )
	{
		g_FrameBuffer->paintBackgroundBox (sx, sy- oldtoph- 1, sx+ ox, sy- toph);
	}

	if(epgData.info1.length()!=0)
	{
		processTextToArray( epgData.info1.c_str() );
	}
	info1_lines = epgText.size();

	//scan epg-data - sort to list
	if ( ( epgData.info2.length()==0 ) && (info1_lines == 0) )
	{
		epgData.info2= g_Locale->getText("epgviewer.nodetailed");
	}

	processTextToArray( epgData.info2.c_str() );

	if (epgData.fsk > 0)
	{
		char _tfsk[11];
		sprintf (_tfsk, "FSK: ab %d", epgData.fsk );
		processTextToArray( _tfsk );
	}

	if (epgData.contentClassification.length()> 0)
		processTextToArray( GetGenre(epgData.contentClassification[0]) );
//	processTextToArray( epgData.userClassification.c_str() );

	//show the epg
	g_FrameBuffer->paintBoxRel(sx, sy- toph, ox, toph, COL_MENUHEAD);
	g_Fonts->epg_title->RenderString(sx+10, sy- toph+ topheight+ 3, ox-15, text1.c_str(), COL_MENUHEAD);
	if (text2!="")
		g_Fonts->epg_title->RenderString(sx+10, sy- toph+ 2* topheight+ 3, ox-15, text2.c_str(), COL_MENUHEAD);

	//show date-time....
	g_FrameBuffer->paintBoxRel(sx, sy+oy-botboxheight, ox, botboxheight, COL_MENUHEAD);
	string fromto;
	int widthl,widthr;
	fromto= epgData.start+ " - "+ epgData.end;

	widthl = g_Fonts->epg_date->getRenderWidth(fromto.c_str());
	g_Fonts->epg_date->RenderString(sx+40,  sy+oy-3, widthl, fromto.c_str(), COL_MENUHEAD);
	widthr = g_Fonts->epg_date->getRenderWidth(epgData.date.c_str());
	g_Fonts->epg_date->RenderString(sx+ox-40-widthr,  sy+oy-3, widthr, epgData.date.c_str(), COL_MENUHEAD);

	int showPos = 0;
	textCount = epgText.size();
	int textypos = sy;
	showText(showPos, textypos);

	//show progressbar
	if ( epgData.done!= -1 )
	{
		int pbx = sx + 10 + widthl + 10 + ((ox-104-widthr-widthl-10-10-20)>>1);
		g_FrameBuffer->paintBoxRel(pbx, sy+oy-height, 104, height-6, COL_MENUCONTENT+6);
		g_FrameBuffer->paintBoxRel(pbx+2, sy+oy-height+2, 100, height-10, COL_MENUCONTENT);
		g_FrameBuffer->paintBoxRel(pbx+2, sy+oy-height+2, epgData.done, height-10, COL_MENUCONTENT+3);
	}

	GetPrevNextEPGData(current_id, &current_zeit);
	if (prev_id != 0)
	{
		g_FrameBuffer->paintBoxRel(sx+ 5, sy+ oy- botboxheight+ 4, botboxheight- 8, botboxheight- 8,  COL_MENUCONTENT+ 3);
		g_Fonts->epg_date->RenderString(sx+ 10, sy+ oy- 3, widthr, "<", COL_MENUCONTENT+ 3);
	}

	if (next_id != 0)
	{
		g_FrameBuffer->paintBoxRel(sx+ ox- botboxheight+ 8- 5, sy+ oy- botboxheight+ 4, botboxheight- 8, botboxheight- 8,  COL_MENUCONTENT+ 3);
		g_Fonts->epg_date->RenderString(sx+ ox- botboxheight+ 8, sy+ oy- 3, widthr, ">", COL_MENUCONTENT+ 3);
	}

	if ( doLoop )
	{
		bool loop=true;
		int scrollCount;
		while(loop)
		{
			uint msg; uint data;
			g_RCInput->getMsg( &msg, &data, g_settings.timing_epg );

			scrollCount = medlinecount;

			switch ( msg )
			{
				case CRCInput::RC_left:
					if (prev_id != 0)
					{
						g_FrameBuffer->paintBoxRel(sx+ 5, sy+ oy- botboxheight+ 4, botboxheight- 8, botboxheight- 8,  COL_MENUCONTENT+ 1);
						g_Fonts->epg_date->RenderString(sx+ 10, sy+ oy- 3, widthr, "<", COL_MENUCONTENT+ 1);

						show(channelName, onid_tsid, prev_id, &prev_zeit, false);
					}
					break;

				case CRCInput::RC_right:
					if (next_id != 0)
					{
						g_FrameBuffer->paintBoxRel(sx+ ox- botboxheight+ 8- 5, sy+ oy- botboxheight+ 4, botboxheight- 8, botboxheight- 8,  COL_MENUCONTENT+ 1);
						g_Fonts->epg_date->RenderString(sx+ ox- botboxheight+ 8, sy+ oy- 3, widthr, ">", COL_MENUCONTENT+ 1);

						show(channelName, onid_tsid, next_id, &next_zeit, false);
					}
					break;

				case CRCInput::RC_down:
					if(showPos+scrollCount<textCount)
					{
						showPos += scrollCount;
						showText(showPos,textypos);
					}
					break;

				case CRCInput::RC_up:
					showPos -= scrollCount;
					if(showPos<0)
						showPos = 0;
					else
						showText(showPos,textypos);
					break;

				case CRCInput::RC_red:
					g_RCInput->postMsg( msg, data );

				case CRCInput::RC_ok:
				case CRCInput::RC_help:
				case CRCInput::RC_timeout:
					loop = false;
					break;

				default:
					// konfigurierbare Keys handlen...
					if ( msg == g_settings.key_channelList_cancel )
						loop = false;
					else
					{
						if ( neutrino->handleMsg( msg, data ) & messages_return::cancel_all )
						{
							loop = false;
							res = menu_return::RETURN_EXIT_ALL;
						}
					}
			}
		}
		hide();
	}
	return res;
}

void CEpgData::hide()
{
	g_FrameBuffer->paintBackgroundBox (sx, sy- toph, sx+ ox, sy+ oy);
	#ifdef USEACTIONLOG
		g_ActionLog->println("epg: closed");
	#endif
}

void CEpgData::GetEPGData( const string channelName, const unsigned int onid_tsid, unsigned long long id, time_t* startzeit )
{
	int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";

	epgText.clear();
	emptyLineCount 	= 0;
	epgData.title	= "";
	epgData.info1 	= "";
	epgData.info2 	= "";
	epgData.date 	= "";
	epgData.start 	= "";
	epgData.end 	= "";
	epgData.done 	= -1;
	epgData.fsk	= 0;
	epgData.contentClassification	= "";
	epgData.userClassification		= "";

	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(sectionsd::portNumber);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("Couldn't connect to server!");
		return;
	}

	if ( (( onid_tsid != 0 )  ) || ( id!= 0 ) )
	{
		if ( id!= 0 )
		{
			// query EPG für bestimmtes Event
			sectionsd::msgRequestHeader req;
			req.version = 2;
			req.command = sectionsd::epgEPGid;
			req.dataLength = 12;
			write(sock_fd,&req,sizeof(req));

			write(sock_fd, &id, sizeof(id));
			write(sock_fd, startzeit, sizeof(*startzeit));
			printf("[epgdata] query for evt_id >%llx<, time %lx\n", id, *startzeit);
		}
		else
		{
			// query EPG normal
			sectionsd::msgRequestHeader req;
			req.version = 2;
			req.command = sectionsd::actualEPGchannelID;
			req.dataLength = 4;
			write(sock_fd,&req,sizeof(req));

			write(sock_fd, &onid_tsid, sizeof(onid_tsid));
			printf("[epgdata] query for onid_tsid >%x< (%s)\n", onid_tsid, channelName.c_str());
		}

		sectionsd::msgResponseHeader resp;
		memset(&resp, 0, sizeof(resp));
		read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

		int nBufSize = resp.dataLength;
		if(nBufSize>0)
		{
			char* pData = new char[nBufSize] ;
			read(sock_fd, pData, nBufSize);

			sectionsd::sectionsdTime*    epg_times;
			char* dp = pData;

			current_id = *((unsigned long long *)dp);
            dp+= sizeof(current_id);

			epgData.title = dp;
			dp+=strlen(dp)+1;
			epgData.info1 = dp;
			dp+=strlen(dp)+1;
			epgData.info2 = dp;
			dp+=strlen(dp)+1;
			epgData.contentClassification = dp;
			dp+=strlen(dp)+1;
			epgData.userClassification = dp;
			dp+=strlen(dp)+1;
			epgData.fsk = *dp++;
			// printf("fsk: %d\n", epgData.fsk);

			epg_times = (sectionsd::sectionsdTime*) dp;
            dp+= sizeof(sectionsd::sectionsdTime);

			struct tm *pStartZeit = localtime(&(*epg_times).startzeit);
			char temp[11];
			strftime( temp, sizeof(temp), "%d.%m.%Y", pStartZeit);
			epgData.date= temp;
			strftime( temp, sizeof(temp), "%H:%M", pStartZeit);
			epgData.start= temp;

			long int uiEndTime((*epg_times).startzeit+ (*epg_times).dauer);
			struct tm *pEndeZeit = localtime((time_t*)&uiEndTime);
			strftime( temp, sizeof(temp), "%H:%M", pEndeZeit);
            epgData.end= temp;

			if (( time(NULL)- (*epg_times).startzeit )>= 0 )
			{
				unsigned nProcentagePassed=(unsigned)((float)(time(NULL)-(*epg_times).startzeit)/(float)(*epg_times).dauer*100.);
				if (nProcentagePassed<= 100)
					epgData.done= nProcentagePassed;
			}

			#ifdef USEACTIONLOG
				char buf[1000];
				sprintf((char*) buf, "epg: %08x \"%s\" %s %s - %s, \"%s\"", onid_tsid, channelName.c_str(), epgData.date.c_str(), epgData.start.c_str(), epgData.end.c_str(), epgData.title.c_str() );
				g_ActionLog->println(buf);
			#endif

			delete[] pData;
		}
	}
	close(sock_fd);
}

void CEpgData::GetPrevNextEPGData( unsigned long long id, time_t* startzeit )
{
	int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";

	prev_id= 0;
	next_id= 0;
	//        time_t* prev_zeit;
	//        time_t* next_zeit;

	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(sectionsd::portNumber);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("Couldn't connect to server!");
		return;
	}

	// query PrevNext für bestimmtes Event
	sectionsd::msgRequestHeader req;
	req.version = 2;
	req.command = sectionsd::getEPGPrevNext;
	req.dataLength = 12;
	write(sock_fd,&req,sizeof(req));

	write(sock_fd, &id, sizeof(id));
	write(sock_fd, startzeit, sizeof(*startzeit));
	printf("[epgdata] query prev/next for evt_id >%llx<, time %lx\n", id, *startzeit);

	sectionsd::msgResponseHeader resp;
	memset(&resp, 0, sizeof(resp));
	read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

	int nBufSize = resp.dataLength;
	if(nBufSize>0)
	{
		char* pData = new char[nBufSize] ;
		recv(sock_fd, pData, nBufSize, MSG_WAITALL);

		char* dp = pData;
		//printf("%s \n", dp);
		sscanf(dp, "%012llx\xFF", &prev_id);
		dp+= 13;
		sscanf(dp, "%08lx\xFF", &prev_zeit);
		dp+= 9;
		sscanf(dp, "%012llx\xFF", &next_id);
		dp+= 13;
		sscanf(dp, "%08lx\xFF", &next_zeit);

		//printf("got prev evt_id >%llx<, time %x\n", prev_id, prev_zeit);
		//printf("got next evt_id >%llx<, time %x\n", next_id, next_zeit);

		delete[] pData;
	}

	close(sock_fd);
}

