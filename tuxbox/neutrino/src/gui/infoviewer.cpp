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

// Revision 1.70  2002/01/30 21:41:01  McClean
// channame-position
//
// Revision 1.69  2002/01/30 20:58:55  field
// DD-Symbol
//
// Revision 1.67  2002/01/29 17:26:51  field
// Jede Menge Updates :)
//
// Revision 1.66  2002/01/28 23:46:47  field
// Boxtyp automatisch, Vol im Scartmode, Kleinigkeiten
//
// Revision 1.65  2002/01/28 19:52:32  field
// Streaminfo ausfuehrlicher
//
// Revision 1.64  2002/01/23 13:37:08  McClean
// final infobar-fix
//
// Revision 1.63  2002/01/23 12:45:10  McClean
// fix infobar?
//
// Revision 1.62  2002/01/18 23:34:19  McClean
// repair infobar
//
// Revision 1.61  2002/01/16 03:08:08  McClean
// HOTFIX for infobar-clear (timeout) - field..
//
// Revision 1.60  2002/01/16 02:09:04  McClean
// cleanups+quickzap-fix
//
// Revision 1.59  2002/01/15 23:17:59  McClean
// cleanup
//
// Revision 1.58  2002/01/03 20:03:20  McClean
// cleanup
//
// Revision 1.57  2001/12/13 01:10:35  McClean
// show rest time (current) in infobar
//
// Revision 1.56  2001/12/13 00:51:52  McClean
// fix infobar - showepg-bug
//
// Revision 1.55  2001/12/12 19:11:32  McClean
// prepare timing setup...
//
// Revision 1.54  2001/12/12 11:46:06  McClean
// performance-improvements
//
// Revision 1.53  2001/12/12 11:33:57  McClean
// major epg-fixes
//
// Revision 1.52  2001/11/26 02:34:04  McClean
// include (.../../stuff) changed - correct unix-formated files now
//
// Revision 1.51  2001/11/22 13:39:33  field
// Verbesserungen
//
// Revision 1.47  2001/11/15 11:42:41  McClean
// gpl-headers added
//
// Revision 1.46  2001/11/05 17:13:26  field
// wiederholungen...?
//
// Revision 1.45  2001/11/05 16:04:25  field
// nvods/subchannels ver"c++"ed
//
// Revision 1.44  2001/11/03 15:43:17  field
// Perspektiven
//
// Revision 1.43  2001/11/03 03:13:10  field
// EPG Anzeige verbessert
//
// Revision 1.42  2001/10/25 12:26:09  field
// NVOD-Zeiten im Infoviewer stimmen
//
// Revision 1.41  2001/10/21 13:06:17  field
// nvod-zeiten funktionieren!
//
// Revision 1.40  2001/10/18 21:03:14  field
// EPG Previous/Next
//
// Revision 1.39  2001/10/16 19:21:30  field
// NVODs! Zeitanzeige geht noch nicht
//
// Revision 1.38  2001/10/15 17:27:19  field
// nvods (fast) implementiert (umschalten funkt noch nicht)
//
// Revision 1.37  2001/10/14 14:30:47  rasc
// -- EventList Darstellung ueberarbeitet
// -- kleiner Aenderungen und kleinere Bugfixes
// -- locales erweitert..
//
// Revision 1.36  2001/10/10 17:17:13  field
// zappen auf onid_sid umgestellt
//
// Revision 1.35  2001/10/09 21:09:23  fnbrd
// Fixed small bug.
//
// Revision 1.34  2001/10/09 20:10:08  fnbrd
// Ein paar fehlende Initialisierungen implementiert.
//
// Revision 1.33  2001/10/07 12:17:22  McClean
// video mode setup (pre)
//
// Revision 1.32  2001/10/02 17:56:33  McClean
// time in infobar (thread probs?) and "0" quickzap added
//
// Revision 1.31  2001/09/27 11:23:50  field
// Numzap gefixt, kleiner Bugfixes
//
// Revision 1.30  2001/09/26 15:02:34  field
// Anzeige zu verschluesselten Sendern
//
// Revision 1.29  2001/09/26 11:40:48  field
// Tontraegerauswahl haut hin (bei Kanaelen mit EPG)
//
// Revision 1.28  2001/09/26 09:57:02  field
// Tontraeger-Auswahl ok (bei allen Chans. auf denen EPG geht)
//
// Revision 1.27  2001/09/23 21:34:07  rasc
// - LIFObuffer Module, pushbackKey fuer RCInput,
// - In einige Helper und widget-Module eingebracht
//   ==> harmonischeres Menuehandling
// - Infoviewer Breite fuer Channelsdiplay angepasst (>1000 Channels)
//
// Revision 1.26  2001/09/22 17:57:03  McClean
// infobar painting modified
//
// Revision 1.25  2001/09/22 14:57:45  field
// Anzeige, wenn Kanal nicht verfuegbar
//
// Revision 1.24  2001/09/20 14:10:10  field
// neues EPG-Handling abschaltbar
//
// Revision 1.23  2001/09/20 11:56:00  field
// Final fix for new structure
//
// Revision 1.22  2001/09/20 11:37:29  fnbrd
// Fixed small bug.
//
// Revision 1.21  2001/09/20 11:21:06  field
// buggy...
//
// Revision 1.20  2001/09/20 00:36:32  field
// epg mit zaopit zum grossteil auf onid & s_id umgestellt
//
// Revision 1.19  2001/09/19 18:03:14  field
// Infobar, Sprachauswahl
//
// Revision 1.18  2001/09/18 20:20:26  field
// Eventlist in den Infov. verschoben (gelber Knopf), Infov.-Anzeige auf Knoepfe
// vorbereitet
//
// Revision 1.17  2001/09/17 12:45:12  field
// Sprache online umstellbar, kleine Aufraeumarbeiten
//
// Revision 1.16  2001/09/16 02:27:22  McClean
// make neutrino i18n
//
// Revision 1.15  2001/09/14 16:18:46  field
// Umstellung auf globale Variablen...
//
// Revision 1.14  2001/09/13 17:15:51  field
// verbessertes warten aufs epg
//
// Revision 1.13  2001/09/13 10:12:41  field
// Major update! Beschleunigtes zappen & EPG uvm...
//
// Revision 1.12  2001/09/09 23:53:46  fnbrd
// Fixed some bugs, only shown compiling with -Os.
// Conclusion: use -Os ;)
//
// Revision 1.11  2001/09/05 21:21:16  McClean
// design-fix
//
// Revision 1.10  2001/08/22 11:29:31  McClean
// infoviewer designfix
//
// Revision 1.9  2001/08/20 13:07:10  tw-74
// cosmetic changes and changes for variable font size
//
// Revision 1.8  2001/08/16 23:19:18  McClean
// epg-view and quickview changed
//
// Revision 1.7  2001/08/16 00:19:44  fnbrd
// Removed debug output.
//
//

#include "infoviewer.h"
#include "../global.h"

#define COL_INFOBAR_BUTTONS				COL_INFOBAR_SHADOW+ 1
#define COL_INFOBAR_BUTTONS_GRAY		COL_INFOBAR_SHADOW+ 1

#define ICON_LARGE 30
#define ICON_SMALL 20
#define ICON_OFFSET (2*ICON_LARGE+ ICON_SMALL+ 5)
#define BOTTOM_BAR_OFFSET 0
#define SHADOW_OFFSET 6


CInfoViewer::CInfoViewer()
{
        intShowDuration = g_settings.timing_infobar;
        BoxStartX= BoxStartY= BoxEndX= BoxEndY=0;
        is_visible=false;
        ShowInfo_Info=false;

        strcpy( running, "");
        strcpy( next, "");
        strcpy( runningStart, "");
        strcpy( nextStart, "");
        strcpy( runningDuration, "");
        strcpy( runningRest, "");
        strcpy( nextDuration, "");

        runningPercent = 0;
        CurrentChannel = "";

        pthread_cond_init( &epg_cond, NULL );
        pthread_mutexattr_t   mta;

	    if (pthread_mutexattr_init(&mta) != 0 )
    		perror("CInfoViewer: pthread_mutexattr_init failed\n");
    	if (pthread_mutexattr_settype( &mta, PTHREAD_MUTEX_ERRORCHECK ) != 0 )
			perror("CInfoViewer: pthread_mutexattr_settype failed\n");
		if (pthread_mutex_init( &epg_mutex, &mta ) != 0)
			perror("CInfoViewer: pthread_mutex_init failed\n");


        if (pthread_create (&thrViewer, NULL, InfoViewerThread, (void *) this) != 0 )
        {
                perror("CInfoViewer::CInfoViewer create thrViewer failed\n");
        }

        pthread_cond_init( &cond_PIDs_available, NULL );

        if (pthread_create (&thrLangViewer, NULL, LangViewerThread, (void *) this) != 0 )
        {
                perror("CInfoViewer::CInfoViewer create thrLangViewer failed\n");
        }

}

void CInfoViewer::start()
{
        InfoHeightY = g_Fonts->infobar_number->getHeight()*9/8 +
                      2*g_Fonts->infobar_info->getHeight() +
                      25;
        InfoHeightY_Info = g_Fonts->infobar_small->getHeight()+ 5;

        ChanWidth = g_Fonts->infobar_number->getRenderWidth("0000") + 10;
        ChanHeight = g_Fonts->infobar_number->getHeight()*9/8;

}

void CInfoViewer::setDuration( int Duration )
{
        intShowDuration = Duration;
}

const std::string CInfoViewer::getActiveChannelID()
{
        string  s_id;
        char anid[10];
        snprintf( anid, 10, "%x", Current_onid_tsid );
        s_id= anid;

        return s_id;
}

void CInfoViewer::showTitle( int ChanNum, string Channel, unsigned int onid_tsid, bool CalledFromNumZap )
{
        pthread_mutex_lock( &epg_mutex );

        CurrentChannel = Channel;
        Current_onid_tsid = onid_tsid;

        ShowInfo_Info = !CalledFromNumZap;

        EPG_NotFound_Text = (char*) g_Locale->getText(CalledFromNumZap?"infoviewer.epgnotload":"infoviewer.epgwait").c_str();

        BoxStartX = g_settings.screen_StartX+ 20;
        BoxEndX   = g_settings.screen_EndX- 20;
        BoxEndY   = g_settings.screen_EndY- 20;

        int BoxEndInfoY = ShowInfo_Info?(BoxEndY- InfoHeightY_Info):(BoxEndY);
		BoxStartY = BoxEndInfoY- InfoHeightY;

        KillShowEPG = false;
        pthread_mutex_unlock( &epg_mutex );

		// kill linke seite
        g_FrameBuffer->paintBackgroundBox(BoxStartX, BoxStartY+ ChanHeight, BoxStartX + (ChanWidth/3), BoxStartY+ ChanHeight+ InfoHeightY_Info+ 10);
        // kill progressbar
        g_FrameBuffer->paintBackgroundBox(BoxEndX- 120, BoxStartY, BoxEndX, BoxStartY+ ChanHeight);

        //number box
        g_FrameBuffer->paintBoxRel(BoxStartX+10, BoxStartY+10, ChanWidth, ChanHeight, COL_INFOBAR_SHADOW);
        g_FrameBuffer->paintBoxRel(BoxStartX,    BoxStartY,    ChanWidth, ChanHeight, COL_INFOBAR);

        //channel number
        char strChanNum[10];
        sprintf( (char*) strChanNum, "%d", ChanNum);
        int ChanNumXPos = BoxStartX + ((ChanWidth - g_Fonts->infobar_number->getRenderWidth(strChanNum))>>1);
        g_Fonts->infobar_number->RenderString(ChanNumXPos, BoxStartY+ChanHeight, ChanWidth, strChanNum, COL_INFOBAR);

        //infobox
        int ChanNameX = BoxStartX + ChanWidth + 10;
        int ChanNameY = BoxStartY + (ChanHeight>>1)   + 5; //oberkante schatten?

       	g_FrameBuffer->paintBox(ChanNameX, ChanNameY, BoxEndX, BoxEndInfoY, COL_INFOBAR);

        int height=g_Fonts->infobar_channame->getHeight()+5;

        //time? todo - thread suxx...
        char timestr[50];
        struct timeb tm;
        ftime(&tm);
        strftime((char*) &timestr, 20, "%H:%M", localtime(&tm.time) );
        int timewidth = g_Fonts->infobar_channame->getRenderWidth(timestr);
        g_Fonts->infobar_channame->RenderString(BoxEndX-timewidth-10, ChanNameY+height, timewidth+ 5, timestr, COL_INFOBAR);

		// ... with channel name
        g_Fonts->infobar_channame->RenderString(ChanNameX+ 10, ChanNameY+height, BoxEndX- (ChanNameX+ 20)- timewidth- 15, Channel.c_str(), COL_INFOBAR);

        ChanInfoX = BoxStartX + (ChanWidth / 3);
        int ChanInfoY = BoxStartY + ChanHeight+10;

        g_FrameBuffer->paintBox(ChanInfoX, ChanInfoY, ChanNameX, BoxEndInfoY, COL_INFOBAR);

        if ( ShowInfo_Info )
        {
        		if ( BOTTOM_BAR_OFFSET> 0 )
	        		g_FrameBuffer->paintBackgroundBox(ChanInfoX, BoxEndInfoY, BoxEndX, BoxEndInfoY+ BOTTOM_BAR_OFFSET);
        		g_FrameBuffer->paintBox(ChanInfoX, BoxEndInfoY+ BOTTOM_BAR_OFFSET, BoxEndX, BoxEndY, COL_INFOBAR_BUTTONS);

                ButtonWidth = (BoxEndX- ChanInfoX- ICON_OFFSET)>> 2;

                //g_FrameBuffer->paintHLine(ChanInfoX, BoxEndX,  BoxEndY-InfoHeightY_Info, COL_INFOBAR_SHADOW);
                //g_FrameBuffer->paintHLine(ChanInfoX, BoxEndX,  BoxEndY-InfoHeightY_Info+1, COL_INFOBAR_SHADOW); 2Lines wegen scanline?

                // blau
                g_FrameBuffer->paintIcon("blau.raw", BoxEndX- ICON_OFFSET- ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.streaminfo").c_str(), COL_INFOBAR_BUTTONS);

				// gelb
				//g_FrameBuffer->paintIcon("gray.raw", BoxEndX- ICON_OFFSET- 2* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
				//g_Fonts->infobar_small->RenderString(BoxEndX- 2* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.subservice").c_str(), COL_INFOBAR_BUTTONS_GRAY);

				// grün
				//g_FrameBuffer->paintIcon("gray.raw", BoxEndX- ICON_OFFSET- 3* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
				//g_Fonts->infobar_small->RenderString(BoxEndX- 3* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.languages").c_str(), COL_INFOBAR_BUTTONS_GRAY);

				// rot
				//g_FrameBuffer->paintIcon("gray.raw", BoxEndX- ICON_OFFSET- 4* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
				//g_Fonts->infobar_small->RenderString(BoxEndX- 4* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.eventlist").c_str(), COL_INFOBAR_BUTTONS_GRAY);

                g_FrameBuffer->paintIcon("dd_gray.raw", BoxEndX- ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                g_FrameBuffer->paintIcon((GetVideoFormat() == 3)?"16_9.raw":"16_9_gray.raw", BoxEndX- 2* ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                g_FrameBuffer->paintIcon("vtxt_gray.raw", BoxEndX- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );

                showButtonNVOD(true);

                g_RemoteControl->CopyPIDs();
                showButtonAudio();
        }

		// Schatten
        g_FrameBuffer->paintBox(BoxEndX, ChanNameY+ SHADOW_OFFSET, BoxEndX+ SHADOW_OFFSET, BoxEndY, COL_INFOBAR_SHADOW);
        g_FrameBuffer->paintBox(ChanInfoX+ SHADOW_OFFSET, BoxEndY, BoxEndX+ SHADOW_OFFSET, BoxEndY+ SHADOW_OFFSET, COL_INFOBAR_SHADOW);


        pthread_mutex_lock( &epg_mutex );
        is_visible = true;
        pthread_mutex_unlock( &epg_mutex );

        pthread_cond_signal( &epg_cond );

        usleep(50);

        int msg; uint data;

        if ( !CalledFromNumZap )
        {
        		for (int i= 0; i< 10; i++)
        		{
        			g_FrameBuffer->paintIcon((GetVideoFormat() == 3)?"16_9.raw":"16_9_gray.raw", BoxEndX- 2* ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );


					g_RCInput->getMsg( &msg, &data, intShowDuration>>1 ); // weil Intervall in 500ms angegeben ist

					if ( msg != CRCInput::RC_timeout )
						break;
				}


				if ( ( msg != CRCInput::RC_timeout ) && ( msg != CRCInput::RC_ok ) )
				{
            		if ( neutrino->handleMsg( msg, data ) ==  CRCInput::MSG_unhandled )
            			g_RCInput->pushbackMsg( msg, data );
				}

				if ( ( msg != g_settings.key_quickzap_up ) &&
                	 ( msg != g_settings.key_quickzap_down ) &&
                	 ( msg != CRCInput::RC_help ) )
                {
                   	killTitle();
                }
        }
}



int CInfoViewer::GetVideoFormat()
{
	FILE* fd = fopen("/proc/bus/bitstream", "rt");
	if (fd==NULL)
	{
		printf("error while opening proc-bitstream\n" );
		return 1;
	}

	char *tmpptr, buf[100];
	int value= 1;
	int pos= 0;
	while(!feof(fd))
	{
		if(fgets(buf,29,fd)!=NULL)
		{
			if ( pos == 3 )
			{
				buf[strlen(buf)-1]=0;
				tmpptr=buf;
				strsep(&tmpptr,":");
				for(;tmpptr[0]==' ';tmpptr++)
					;
				value= atoi(tmpptr);
				break;
			}
			pos++;
		}
	}
	fclose(fd);
	return value;
}


void CInfoViewer::showButtonNVOD(bool CalledFromShowData = false)
{
        CSubChannel_Infos subChannels= g_RemoteControl->getSubChannels();

        if ( subChannels.has_subChannels_for( getActiveChannelID() ) )
        {
                // gelbe Taste für NVODs / Subservices
                g_FrameBuffer->paintIcon("gelb.raw", BoxEndX- ICON_OFFSET- 2* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                if ( subChannels.are_subchannels )
                        // SubServices
                        g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- 2* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.subservice").c_str(), COL_INFOBAR_BUTTONS);
                else
                        // NVOD
                        g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- 2* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.selecttime").c_str(), COL_INFOBAR_BUTTONS);

                if (!CalledFromShowData)
                        showData();
        };
}

void CInfoViewer::showData()
{
        int is_nvod= false;

        if (ShowInfo_Info)
        {
                CSubChannel_Infos subChannels= g_RemoteControl->getSubChannels();

                if ( SubServiceList.size()> 0 )
                {
                        string activeID= getActiveChannelID();
                        if ( !subChannels.has_subChannels_for( activeID ) )
                        {
                                //printf("subservices %d\n", SubServiceList.size());
                                subChannels= CSubChannel_Infos( activeID.c_str(), true );
                                for(unsigned int count=0;count<SubServiceList.size();count++)
                                {
                                        subChannels.list.insert( subChannels.list.end(),
                                                                 CSubService(SubServiceList[count]->originalNetworkId<<16 | SubServiceList[count]->serviceId,
                                                                             SubServiceList[count]->transportStreamId,
                                                                             SubServiceList[count]->name) );
                                }
                                g_RemoteControl->CopySubChannelsToZapit( subChannels );
                                showButtonNVOD(true);
                        }
                }

                if ( !subChannels.are_subchannels )
                {
                        if ( subChannels.has_subChannels_for( getActiveChannelID() ) )
                        {
                                // NVOD- Zeiten aus dem aktuell selektierten holen!
                                is_nvod= true;

                                unsigned sel= subChannels.selected;
                                unsigned dauer= subChannels.list[sel].dauer/ 60;
                                //sprintf((char*) &runningDuration, "%d min", dauer);

                                struct      tm *pStartZeit = localtime(&subChannels.list[sel].startzeit);
                                sprintf((char*) &runningStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
                                runningPercent=(unsigned)((float)(time(NULL)-subChannels.list[sel].startzeit)/(float)subChannels.list[sel].dauer*100.);

        						unsigned seit = ( time(NULL) - subChannels.list[sel].startzeit ) / 60;
                                unsigned rest = ( (subChannels.list[sel].startzeit + subChannels.list[sel].dauer) - time(NULL) ) / 60;

								sprintf((char*) &runningRest, "%d / %d min", seit, rest);
                                if (runningPercent>100)
                                        runningPercent=0;

                                Flag|= sectionsd::epg_has_current;
                                //printf("%s %s %d\n", runningDuration, runningStart, runningPercent);
                        }
                }
        }

        int height = g_Fonts->infobar_channame->getHeight()/3;
        int ChanInfoY = BoxStartY + ChanHeight+ 15; //+10

        //percent
        if ( ShowInfo_Info )
        {
                int posy = BoxStartY+12;
                int height2= 20;//int( g_Fonts->infobar_channame->getHeight()/1.7);

                //      g_FrameBuffer->paintBox(BoxEndX-130, BoxStartY, BoxEndX, ChanNameY+2, COL_INFOBAR+1); //bounding box (off)
                if ( Flag & sectionsd::epg_has_current)
                {
                        g_FrameBuffer->paintBoxRel(BoxEndX-114, posy,   2+100+2, height2, COL_INFOBAR_SHADOW); //border
                        g_FrameBuffer->paintBoxRel(BoxEndX-112, posy+2, runningPercent+2, height2-4, COL_INFOBAR+7);//fill(active)
                        g_FrameBuffer->paintBoxRel(BoxEndX-112+runningPercent, posy+2, 100-runningPercent, height2-4, COL_INFOBAR+3);//fill passive
                }
                if ( Flag & sectionsd::epg_has_anything )
                {
                        g_FrameBuffer->paintIcon("rot.raw", BoxEndX- ICON_OFFSET- 4* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                        g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- 4* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.eventlist").c_str(), COL_INFOBAR_BUTTONS);
                }

        }

        height = g_Fonts->infobar_info->getHeight();
        int xStart= BoxStartX + ChanWidth;// + 20;

//        if ( is_nvod )
                g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

        if ( Flag & sectionsd::epg_not_broadcast )
        {
                // kein EPG verfügbar
                ChanInfoY += height;
                g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height, COL_INFOBAR);
                g_Fonts->infobar_info->RenderString(BoxStartX + ChanWidth + 20,  ChanInfoY+height, BoxEndX- (BoxStartX + ChanWidth + 20), g_Locale->getText("infoviewer.noepg").c_str(), COL_INFOBAR);
        }
        else
        {
                // irgendein EPG gefunden
                int duration1Width   = g_Fonts->infobar_info->getRenderWidth(runningRest);
        		int duration1TextPos = BoxEndX- duration1Width- 10;

                int duration2Width   = g_Fonts->infobar_info->getRenderWidth(nextDuration);
                int duration2TextPos = BoxEndX- duration2Width- 10;

                if ( ( Flag & sectionsd::epg_has_next ) && ( !( Flag & sectionsd::epg_has_current )) )
                {
                        // spätere Events da, aber kein aktuelles...
                        //g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, BoxEndX- xStart, g_Locale->getText("infoviewer.nocurrent").c_str(), COL_INFOBAR);

                        ChanInfoY += height;

                        //info next
                        g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

                        g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, duration2TextPos- xStart- 5, next, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR);
                }
                else
                {
                		//g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, runningStart, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, duration1TextPos- xStart- 5, running, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(duration1TextPos,            ChanInfoY+height, duration1Width, runningRest, COL_INFOBAR);

                        ChanInfoY += height;

                        //info next
                        g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

                        if ( ( !is_nvod ) && ( Flag & sectionsd::epg_has_next ) )
                        {
                                g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR);
                                g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, duration2TextPos- xStart- 5, next, COL_INFOBAR);
                                g_Fonts->infobar_info->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR);
                        }
                }
        }
        if (!is_visible)
        {
                killTitle();
        }
}

void CInfoViewer::showButtonAudio()
{
        string  to_compare= getActiveChannelID();

        if ( strcmp(g_RemoteControl->audio_chans.name, to_compare.c_str() )== 0 )
        {
                if ( ( g_RemoteControl->ecmpid == invalid_ecmpid_found ) ||
                     ( ( g_RemoteControl->audio_chans.count_apids == 0 ) && ( g_RemoteControl->vpid == 0 ) ) )
                {
                        int height = g_Fonts->infobar_info->getHeight();
                        int ChanInfoY = BoxStartY + ChanHeight+ 15+ 2* height;
                        int xStart= BoxStartX + ChanWidth + 20;

                        //int ChanNameX = BoxStartX + ChanWidth + 10;
                        int ChanNameY = BoxStartY + ChanHeight + 10;


                        string  disp_text;
                        if ( ( g_RemoteControl->ecmpid == invalid_ecmpid_found ) )
						{
                                disp_text= g_Locale->getText("infoviewer.cantdecode");
								#ifdef USEACTIONLOG
									g_ActionLog->println("cannot decode");
								#endif
						}
                        else
						{
                                disp_text= g_Locale->getText("infoviewer.notavailable");
								#ifdef USEACTIONLOG
									g_ActionLog->println("not available");
								#endif
						}

                        g_FrameBuffer->paintBox(ChanInfoX, ChanNameY, BoxEndX, ChanInfoY, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(xStart, ChanInfoY, BoxEndX- xStart, disp_text.c_str(), COL_INFOBAR);
                        KillShowEPG = true;
                };


                // grün, wenn mehrere APIDs
                if ( g_RemoteControl->audio_chans.count_apids> 1 )
                {
                        g_FrameBuffer->paintIcon("gruen.raw", BoxEndX- ICON_OFFSET- 3* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                        g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- 3* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.languages").c_str(), COL_INFOBAR_BUTTONS);
                };

                for (int count= 0; count< g_RemoteControl->audio_chans.count_apids; count++)
                	if ( g_RemoteControl->audio_chans.apids[count].is_ac3 )
                	{
                		if (g_RemoteControl->audio_chans.selected== count )
                		{
	                		g_FrameBuffer->paintIcon("dd.raw", BoxEndX- ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
	                		break;
	                	}
	                	else
	                		g_FrameBuffer->paintIcon("dd_avail.raw", BoxEndX- ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                	}

                if ( g_RemoteControl->vtxtpid != 0 )
                	g_FrameBuffer->paintIcon("vtxt.raw", BoxEndX- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
        };
}

void CInfoViewer::showWarte()
{

        int height = g_Fonts->infobar_info->getHeight();
        int ChanInfoY = BoxStartY + ChanHeight+ 15+ 2* height;
        int xStart= BoxStartX + ChanWidth + 20;

        pthread_mutex_lock( &epg_mutex );
        if ( ( !KillShowEPG ) && ( is_visible ) )
                g_Fonts->infobar_info->RenderString(xStart, ChanInfoY, BoxEndX- xStart, EPG_NotFound_Text, COL_INFOBAR);
        pthread_mutex_unlock( &epg_mutex );
}

void CInfoViewer::killTitle()
{
        pthread_mutex_lock( &epg_mutex );
        if (is_visible )
        {
                is_visible = false;
                g_FrameBuffer->paintBackgroundBox(BoxStartX, BoxStartY, BoxEndX+ SHADOW_OFFSET, BoxEndY+ SHADOW_OFFSET );
        }
        pthread_mutex_unlock( &epg_mutex );
}


void * CInfoViewer::LangViewerThread (void *arg)
{
        CInfoViewer* InfoViewer = (CInfoViewer*) arg;
        while(1)
        {
                pthread_mutex_lock( &InfoViewer->epg_mutex );
                pthread_cond_wait( &InfoViewer->cond_PIDs_available, &InfoViewer->epg_mutex );

                if ( ( InfoViewer->is_visible ) && ( InfoViewer->ShowInfo_Info ) )
                {
                        g_RemoteControl->CopyPIDs();
                        InfoViewer->showButtonAudio();

                        InfoViewer->showButtonNVOD();
                }

                pthread_mutex_unlock( &InfoViewer->epg_mutex );
        }
}

void * CInfoViewer::InfoViewerThread (void *arg)
{
	int repCount;
	string query = "";
	string old_query = "";
	unsigned int    query_onid_tsid;
	bool gotEPG, requeryEPG;
	struct timespec abs_wait;
	struct timeval now;
	char old_flags;

	CInfoViewer* InfoViewer = (CInfoViewer*) arg;
	while(1)
	{
		pthread_mutex_lock( &InfoViewer->epg_mutex );
		pthread_cond_wait( &InfoViewer->epg_cond, &InfoViewer->epg_mutex );

		if ( ( InfoViewer->is_visible ) )
		{
			gotEPG = true;
			repCount = 10;
            query = "";
			do
			{
				if ( !gotEPG )
				{
					if ( ( repCount > 0 ) &&
					     !( InfoViewer->Flag & ( sectionsd::epg_has_later | sectionsd::epg_has_current ) ) )
						InfoViewer->showWarte();

					gettimeofday(&now, NULL);
					TIMEVAL_TO_TIMESPEC(&now, &abs_wait);
					abs_wait.tv_sec += 1;

					pthread_mutex_lock( &InfoViewer->epg_mutex );
					pthread_cond_timedwait( &InfoViewer->epg_cond, &InfoViewer->epg_mutex, &abs_wait );

					repCount--;
				}

				old_flags = InfoViewer->Flag;
				old_query = query;

				pthread_mutex_lock( &InfoViewer->epg_mutex );
				query = InfoViewer->CurrentChannel;
				query_onid_tsid = InfoViewer->Current_onid_tsid;
				pthread_mutex_unlock( &InfoViewer->epg_mutex );

				gotEPG = ( ( InfoViewer->getEPGData(query, query_onid_tsid) ) &&
				           ( InfoViewer->Flag & sectionsd::epg_has_next ) &&
						   ( ( InfoViewer->Flag & sectionsd::epg_has_current ) || ( InfoViewer->Flag & sectionsd::epg_has_no_current ) ) );

				pthread_mutex_lock( &InfoViewer->epg_mutex );

				if ( ( InfoViewer->Flag & ( sectionsd::epg_has_later | sectionsd::epg_has_current ) ) && (!gotEPG) )
				{
					if (!InfoViewer->ShowInfo_Info)
					{
						gotEPG= true;
					}
					else
					if ( ( (query!=old_query) ||
						   ( (InfoViewer->Flag & sectionsd::epg_has_current) != (old_flags & sectionsd::epg_has_current) ) ||
						   ( (InfoViewer->Flag & sectionsd::epg_has_later) != (old_flags & sectionsd::epg_has_later) ) ) &&
						 ( !InfoViewer->KillShowEPG ) && ( InfoViewer->is_visible ) )
					{
						InfoViewer->showData();
					}
				}
				else
				{
					gotEPG= gotEPG || ( InfoViewer->Flag & sectionsd::epg_not_broadcast );
				}

				requeryEPG = ( ( (!gotEPG) || (query!=InfoViewer->CurrentChannel) ) &&
				               ( InfoViewer->is_visible ) );

				if (query!=InfoViewer->CurrentChannel)
					repCount = 10;

				if ( InfoViewer->KillShowEPG )
					repCount = 0;


				if ( ( !requeryEPG) && ( InfoViewer->is_visible ) && ( !InfoViewer->KillShowEPG) )
					InfoViewer->showData();

				pthread_mutex_unlock( &InfoViewer->epg_mutex );

			} while ( ( requeryEPG ) && (repCount > 0) );
		}

	}
    return NULL;
}

bool CInfoViewer::getEPGData( string channelName, unsigned int onid_tsid )
{
        int sock_fd;
        SAI servaddr;
        char rip[]="127.0.0.1";
        bool retval = false;
        unsigned short SubServiceCount;

        sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        memset(&servaddr,0,sizeof(servaddr));
        servaddr.sin_family=AF_INET;
        servaddr.sin_port=htons(sectionsd::portNumber);
        inet_pton(AF_INET, rip, &servaddr.sin_addr);

        strcpy( running, "");
        strcpy( next, "");
        strcpy( runningStart, "");
        strcpy( nextStart, "");
        strcpy( runningDuration, "");
        strcpy( runningRest, "");
        strcpy( nextDuration, "");
        runningPercent = 0;
        Flag= 0;

        for(unsigned int count=0;count<SubServiceList.size();count++)
        {
                delete SubServiceList[count];
        }
        SubServiceList.clear();

        if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
        {
                perror("Couldn't connect to server!");
                return false;
        }

        if ( ( onid_tsid != 0 ) )
        {
                // query mit onid_tsid...

                sectionsd::msgRequestHeader req;
                req.version = 2;
                req.command = sectionsd::currentNextInformationID;
                req.dataLength = 4;
                write(sock_fd,&req,sizeof(req));

                write(sock_fd, &onid_tsid, sizeof(onid_tsid));
                //            char    num_evts = 2;
                //            write(sock_fd, &num_evts, 1);
                printf("[infoviewer]: query epg for >%x< (%s)\n", onid_tsid, channelName.c_str());

                sectionsd::msgResponseHeader resp;
                memset(&resp, 0, sizeof(resp));

                read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

                int nBufSize = resp.dataLength;
                if(nBufSize>0)
                {

                        char* pData = new char[nBufSize+1] ;
                        //read(sock_fd, pData, nBufSize);
                        recv(sock_fd, pData, nBufSize, MSG_WAITALL);

                        unsigned long long          tmp_id;
                        sectionsd::sectionsdTime*   epg_times;
                        char* dp = pData;

                        // current
                        tmp_id = *((unsigned long long *)dp);
                        dp+= sizeof(tmp_id);
                        epg_times = (sectionsd::sectionsdTime*) dp;
                        dp+= sizeof(sectionsd::sectionsdTime);

                        unsigned dauer = epg_times->dauer / 60;
                        unsigned rest = ( (epg_times->startzeit + epg_times->dauer) - time(NULL) ) / 60;
                        //sprintf((char*) &runningDuration, "%d min", dauer);

                        unsigned seit = ( time(NULL) - epg_times->startzeit ) / 60;
						sprintf((char*) &runningRest, "%d / %d min", seit, rest);
                        //sprintf((char*) &runningRest, "%d min", rest);

                        struct      tm *pStartZeit = localtime(&epg_times->startzeit);
                        sprintf((char*) &runningStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
                        runningPercent=(unsigned)((float)(time(NULL)-epg_times->startzeit)/(float)epg_times->dauer*100.);
                        strncpy(running, dp, sizeof(running));
                        dp+=strlen(dp)+1;

                        // next
                        tmp_id = *((unsigned long long *)dp);
                        dp+= sizeof(tmp_id);
                        epg_times = (sectionsd::sectionsdTime*) dp;
                        dp+= sizeof(sectionsd::sectionsdTime);

                        dauer = epg_times->dauer/ 60;
                        sprintf((char*) &nextDuration, "%d min", dauer);
                        pStartZeit = localtime(&epg_times->startzeit);
                        sprintf((char*) &nextStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
                        strncpy(next, dp, sizeof(next));
                        //printf("[infoviewer]: next= %s\n", next);
                        dp+=strlen(dp)+1;

                        Flag = (unsigned char)* dp;
                        dp+= sizeof(unsigned char);
                        SubServiceCount= *((unsigned short *)dp);
                        //printf("got %d SubServiceCount\n", SubServiceCount);
                        dp+= sizeof(unsigned short);
                        for (int count= 0; count< SubServiceCount; count++)
                        {
                                SubService* aSubService = new SubService();
                                aSubService->name = dp;
                                //printf("SubServiceName %s\n", aSubService->name.c_str());
                                dp+= strlen(dp)+1;
                                aSubService->transportStreamId= *((unsigned short *)dp);
                                dp+= sizeof(unsigned short);
                                aSubService->originalNetworkId= *((unsigned short *)dp);
                                dp+= sizeof(unsigned short);
                                aSubService->serviceId= *((unsigned short *)dp);
                                dp+= sizeof(unsigned short);

                                SubServiceList.insert(SubServiceList.end(), aSubService);
                        }


                        delete[] pData;
                        retval = true;

                }
        }
        close(sock_fd);
        return retval;
}
