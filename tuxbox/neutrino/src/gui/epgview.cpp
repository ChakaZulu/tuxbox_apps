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

//
// $Id: epgview.cpp,v 1.26 2001/11/26 02:34:04 McClean Exp $
//
// $Log: epgview.cpp,v $
// Revision 1.26  2001/11/26 02:34:04  McClean
// include (.../../stuff) changed - correct unix-formated files now
//
// Revision 1.25  2001/11/23 17:29:10  McClean
// no timeout dont worxx!!!!!!!!
//
// Revision 1.23  2001/11/15 11:42:41  McClean
// gpl-headers added
//
// Revision 1.22  2001/11/05 17:13:26  field
// wiederholungen...?
//
// Revision 1.21  2001/11/03 22:44:41  McClean
// radiomode paint bug fixed
//
// Revision 1.20  2001/10/18 22:01:31  field
// kleiner Bugfix
//
// Revision 1.18  2001/10/18 17:14:08  field
// bugfix
//
// Revision 1.15  2001/10/11 21:04:58  rasc
// - EPG:
//   Event: 2 -zeilig: das passt aber noch nicht  ganz (read comments!).
//   Key-handling etwas harmonischer gemacht  (Left/Right/Exit)
// - Code etwas restrukturiert und eine Fettnaepfe meinerseits beseitigt
//   (\r\n wg. falscher CSV Einstellung...)
//
// Revision 1.14  2001/09/22 13:18:07  field
// epg-anzeige bug gefixt
//
// Revision 1.13  2001/09/21 14:33:39  field
// Eventlist - ok/? vertauscht, epg-Breite flexibel
//
// Revision 1.12  2001/09/20 17:02:16  field
// event-liste zeigt jetzt auch epgs an...
//
// Revision 1.11  2001/09/20 13:44:57  field
// epg-Anzeige verbessert
//
// Revision 1.10  2001/09/16 03:38:44  McClean
// i18n + small other fixes
//
// Revision 1.9  2001/09/14 16:18:46  field
// Umstellung auf globale Variablen...
//
// Revision 1.8  2001/09/13 10:12:41  field
// Major update! Beschleunigtes zappen & EPG uvm...
//
// Revision 1.7  2001/09/03 03:34:04  tw-74
// cosmetic fixes, own "Mg" fontmetrics
//
// Revision 1.6  2001/08/20 13:07:10  tw-74
// cosmetic changes and changes for variable font size
//
// Revision 1.5  2001/08/16 00:19:44  fnbrd
// Removed debug output.
//
//

#include "epgdata.h"
#include "../global.h"

static char* ocopyStringto(const char* from, char* to, int len)
{
	const char *fromend=from+len;
	while(*from!='\xff' && from<fromend)
		*(to++)=*(from++);
	*to=0;
	return (char *)++from;
}

CEpgData::CEpgData()
{
}

void CEpgData::start()
{
	oy = 350;

	topheight= g_Fonts->epg_title->getHeight();
	topboxheight=topheight+6;
	botheight=g_Fonts->epg_date->getHeight();
	botboxheight=botheight+6;
	medlineheight=g_Fonts->epg_info1->getHeight();
	medlinecount=(oy-topboxheight-botboxheight)/medlineheight;

	oy=topboxheight+botboxheight+medlinecount*medlineheight; // recalculate

	sy = (((g_settings.screen_EndY-g_settings.screen_StartY)-oy) / 2) + g_settings.screen_StartY;
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

void CEpgData::processTextToArray( char* text  )
{
	string	aktLine = "";
	string	aktWord = "";
	int	aktWidth = 0;
    strcat(text, " ");

	//printf("orginaltext:\n%s\n\n", text);
	while(*text!=0)
	{
		if ( (*text==' ') || (*text=='\n') || (*text=='-') || (*text=='.') )
		{
			//check the wordwidth - add to this line if size ok
            if(*text=='\n')
			{	//enter-handler
				//printf("enter-");
				addTextToArray( aktLine );
				aktLine = "";
				aktWidth= 0;
			}
            else
            {
    			aktWord += *text;

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
			aktWord += *text;
		}
		text++;
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

void CEpgData::show( string channelName, unsigned int onid_tsid, unsigned long long id, time_t* startzeit, bool doLoop )
{
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

	if(strlen(epgData.title)==0)
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

		g_RCInput->getKey(20);
		g_FrameBuffer->paintBackgroundBoxRel(sx, sy, ox, height+10);
		return;
	}

    int oldx= ox;
    int oldsx= sx;
    // Variable Breite, falls der Text zu lang' ist...
    ox = g_Fonts->epg_title->getRenderWidth( epgData.title )+ 15;
    if (ox < 500 )
        ox = 500;
    else if ( ox > ( g_settings.screen_EndX- g_settings.screen_StartX - 20) )
        ox = ( g_settings.screen_EndX- g_settings.screen_StartX - 20);

	sx = (((g_settings.screen_EndX-g_settings.screen_StartX) -ox) / 2) + g_settings.screen_StartX;

    if ( (oldx> ox) && (!doLoop) )
    {
        g_FrameBuffer->paintBackgroundBoxRel (oldsx, sy, sx- oldsx, oy+10);
        g_FrameBuffer->paintBackgroundBoxRel (sx+ ox, sy, sx- oldsx, oy+10);
    }

	if(strlen(epgData.info1)!=0)
	{
		processTextToArray( epgData.info1 );
	}
    info1_lines = epgText.size();

	//scan epg-data - sort to list
	if ( ( strlen(epgData.info2)==0 ) && (info1_lines == 0) )
	{
        strcpy(epgData.info2, g_Locale->getText("epgviewer.nodetailed").c_str());
    }

    processTextToArray( epgData.info2 );

	//show the epg
	g_FrameBuffer->paintBoxRel(sx, sy, ox, topboxheight, COL_MENUHEAD);
	g_Fonts->epg_title->RenderString(sx+10, sy+topheight+3, ox-15, epgData.title, COL_MENUHEAD);

	//show date-time....
	g_FrameBuffer->paintBoxRel(sx, sy+oy-botboxheight, ox, botboxheight, COL_MENUHEAD);
	char fromto[40];
	int widthl,widthr;
	strcpy(fromto,epgData.start); strcat(fromto," - "); strcat(fromto,epgData.end);
	widthl = g_Fonts->epg_date->getRenderWidth(fromto);
	g_Fonts->epg_date->RenderString(sx+40,  sy+oy-3, widthl, fromto, COL_MENUHEAD);
	widthr = g_Fonts->epg_date->getRenderWidth(epgData.date);
	g_Fonts->epg_date->RenderString(sx+ox-40-widthr,  sy+oy-3, widthr, epgData.date, COL_MENUHEAD);

	int showPos = 0;
	textCount = epgText.size();
	int textypos = sy+topboxheight;
	showText(showPos, textypos);

	//show progressbar
    if ( strlen(epgData.done)!= 0 )
    {
    	int progress = atoi(epgData.done);
    	int pbx = sx + 10 + widthl + 10 + ((ox-104-widthr-widthl-10-10-20)>>1);
    	g_FrameBuffer->paintBoxRel(pbx, sy+oy-height, 104, height-6, COL_MENUHEAD+7);
    	g_FrameBuffer->paintBoxRel(pbx+2, sy+oy-height+2, 100, height-10, COL_MENUHEAD+2);
    	g_FrameBuffer->paintBoxRel(pbx+2, sy+oy-height+2, progress, height-10, COL_MENUHEAD+5);
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
    		int key = g_RCInput->getKey(1000);

    		scrollCount = medlinecount;

    		if (key==CRCInput::RC_left)
    		{
                if (prev_id != 0)
                {
                    g_FrameBuffer->paintBoxRel(sx+ 5, sy+ oy- botboxheight+ 4, botboxheight- 8, botboxheight- 8,  COL_MENUCONTENT+ 1);
                    g_Fonts->epg_date->RenderString(sx+ 10, sy+ oy- 3, widthr, "<", COL_MENUCONTENT+ 1);

                    show("", 0, prev_id, &prev_zeit, false);
                }

    		}
            else if (key==CRCInput::RC_right)
    		{
                if (next_id != 0)
                {
                    g_FrameBuffer->paintBoxRel(sx+ ox- botboxheight+ 8- 5, sy+ oy- botboxheight+ 4, botboxheight- 8, botboxheight- 8,  COL_MENUCONTENT+ 1);
                    g_Fonts->epg_date->RenderString(sx+ ox- botboxheight+ 8, sy+ oy- 3, widthr, ">", COL_MENUCONTENT+ 1);

                    show("", 0, next_id, &next_zeit, false);
                }

    		}
            else if (key==CRCInput::RC_down)
    		{
    			if(showPos+scrollCount<textCount)
    			{
    				showPos += scrollCount;
    				showText(showPos,textypos);
    			}
    		}
    		else if (key==CRCInput::RC_up)
    		{
    			showPos -= scrollCount;
    			if(showPos<0)
    				showPos = 0;
                else
    				showText(showPos,textypos);
    		}
            else if (key==CRCInput::RC_red)
    		{
                g_RCInput->pushbackKey(key);
    			loop = false;
    		}
    		else if ( (key==CRCInput::RC_ok) || (key==CRCInput::RC_help)  || (key==g_settings.key_channelList_cancel) || (key==CRCInput::RC_timeout))
    		{
    			loop = false;
    		}

    	}
    	hide();
    }
}

void CEpgData::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel (sx, sy, ox+10, oy+10);
}

void CEpgData::GetEPGData( string channelName, unsigned int onid_tsid, unsigned long long id, time_t* startzeit )
{
	int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";

	epgText.clear();
	emptyLineCount = 0;
	strcpy(epgData.title,"");
	strcpy(epgData.info1,"");
	strcpy(epgData.info2,"");
	strcpy(epgData.date,"");
	strcpy(epgData.start,"");
	strcpy(epgData.end,"");
	strcpy(epgData.done,"");

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

	#ifdef EPG_SECTIONSD
		//use new sectionsd-daemon

    if ( (( onid_tsid != 0 ) && ( g_settings.epg_byname == 0 ) )||
         ( id!= 0 ) )
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
            printf("query epg for evt_id >%llx<, time %lx\n", id, *startzeit);
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
            printf("query epg for onid_tsid >%x< (%s)\n", onid_tsid, channelName.c_str());
        }

		sectionsd::msgResponseHeader resp;
		memset(&resp, 0, sizeof(resp));
		read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

		int nBufSize = resp.dataLength;
		if(nBufSize>0)
		{
			char* pData = new char[nBufSize] ;
			read(sock_fd, pData, nBufSize);

            sectionsd::sectionsdTime    epg_times;
            char* dp = pData;

            sscanf(dp, "%012llx\xFF", &current_id);
            dp+= 13;
            dp = ocopyStringto( dp, epgData.title, sizeof(epgData.title) );
            dp = ocopyStringto( dp, epgData.info1, sizeof(epgData.info1) );
			dp = ocopyStringto( dp, epgData.info2, sizeof(epgData.info2) );

            /* char whs[256];
            dp = ocopyStringto( dp, whs, sizeof(whs) );
            printf("WH: %s\n", whs); */

            sscanf(dp, "%08lx\xFF%08x\xFF", &epg_times.startzeit, &epg_times.dauer );
            current_zeit= epg_times.startzeit;

            struct tm *pStartZeit = localtime(&epg_times.startzeit);
            int nSDay(pStartZeit->tm_mday), nSMon(pStartZeit->tm_mon+1), nSYear(pStartZeit->tm_year+1900),
                nSH(pStartZeit->tm_hour), nSM(pStartZeit->tm_min);
            sprintf( epgData.date, "%02d.%02d.%04d", nSDay, nSMon, nSYear );
            sprintf( epgData.start, "%02d:%02d", nSH, nSM );

            long int uiEndTime(epg_times.startzeit+ epg_times.dauer);
            struct tm *pEndeZeit = localtime((time_t*)&uiEndTime);
            int nFH(pEndeZeit->tm_hour), nFM(pEndeZeit->tm_min);
            sprintf( epgData.end, "%02d:%02d", nFH, nFM );


            if (( time(NULL)- epg_times.startzeit )>= 0 )
            {
                unsigned nProcentagePassed=(unsigned)((float)(time(NULL)-epg_times.startzeit)/(float)epg_times.dauer*100.);
                if (nProcentagePassed<= 100)
                    sprintf( epgData.done, "%03u", nProcentagePassed );
            }

			delete[] pData;
		}
    }
    else
    {
		sectionsd::msgRequestHeader req;
		req.version = 2;
		req.command = sectionsd::actualEPGchannelName;
		req.dataLength = channelName.length()+1;
		write(sock_fd,&req,sizeof(req));

		char chanName[50];
		strcpy(chanName, channelName.c_str());
		for(int count=strlen(chanName)-1;count>=0;count--)
		{
			if((chanName[count]==' ') || (chanName[count]==0))
			{
				chanName[count]=0;
			}
			else
				break;
		}
		printf("query epg for >%s<\n", chanName);
		write(sock_fd, chanName, strlen(chanName)+1);

		sectionsd::msgResponseHeader resp;
		memset(&resp, 0, sizeof(resp));
		read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

		int nBufSize = resp.dataLength;
		if(nBufSize>0)
		{
//			printf("neutrino nBufsize: %d\n", nBufSize);
			char* pData = new char[nBufSize] ;
			read(sock_fd, pData, nBufSize);
//			printf("neutrino epgdata: \n%s\n", pData);

			char tmp[20];
			char *pos = ocopyStringto( pData, tmp, sizeof(tmp));
//			printf("id: %s\n", tmp);
			pos = ocopyStringto( pos, epgData.title, sizeof(epgData.title));
//			printf("title: %s\n", epgData.title);
			pos = ocopyStringto( pos, epgData.info1, sizeof(epgData.info1) );
			pos = ocopyStringto( pos, epgData.info2, sizeof(epgData.info2));
			pos = ocopyStringto( pos, epgData.date, sizeof(epgData.date));
			pos = ocopyStringto( pos, epgData.start, sizeof(epgData.start));
			pos = ocopyStringto( pos, epgData.end, sizeof(epgData.end));
			pos = ocopyStringto( pos, epgData.done, sizeof(epgData.done));

			delete[] pData;
//			printf("copied\n");
		}
    }
	#else
		//for old epgd users
		struct  msgEPGRequest
		{
		    char version;
		    char cmd;
		    char Name[50];
		} req;

		req.version = 1;
		req.cmd     = 1;
		strcpy( req.Name, channelName.c_str() );

		write(sock_fd,&req,sizeof(req));

		struct msgEPGResponse
		{
				char version;
				char sizeOfBuffer[6];
				char pEventBuffer[1];
		} rep;

		read(sock_fd, &rep, sizeof(rep.version)+sizeof(rep.sizeOfBuffer));
		int nBufSize = atol(rep.sizeOfBuffer);
		char* pData = new char[nBufSize+1] ;

		read(sock_fd, pData, nBufSize);

//		printf("neutrino epgdata: %i: %s\n", rep.version, rep.sizeOfBuffer);

		if( nBufSize > 0 )
		{
			printf("%s",pData);
			char *pos = ocopyStringto( pData, epgData.title, sizeof(epgData.title));
			pos = ocopyStringto( pos, epgData.info1, sizeof(epgData.info1) );
			pos = ocopyStringto( pos, epgData.info2, sizeof(epgData.info2));
			pos = ocopyStringto( pos, epgData.date, sizeof(epgData.date));
			pos = ocopyStringto( pos, epgData.start, sizeof(epgData.start));
			pos = ocopyStringto( pos, epgData.end, sizeof(epgData.end));
			pos = ocopyStringto( pos, epgData.done, sizeof(epgData.done));
		}
		delete[] pData;
	#endif

	//printf("exit epg-get\n\n");
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
    printf("query prev/next for evt_id >%llx<, time %lx\n", id, *startzeit);

    sectionsd::msgResponseHeader resp;
    memset(&resp, 0, sizeof(resp));
	read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

	int nBufSize = resp.dataLength;
	if(nBufSize>0)
	{
        char* pData = new char[nBufSize] ;
        read(sock_fd, pData, nBufSize);

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

