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
// $Id: infoviewer.cpp,v 1.63 2002/01/23 12:45:10 McClean Exp $
//
// $Log: infoviewer.cpp,v $
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

char* copyStringto( char* from, char* to, int len)
{
	while( *from != '\n' )
	{
		if (len>2)
		{
			*to = *from;
			to++;
			len--;
		}
		from++;
	}
	*to = 0;
	from ++;
	return from;
}

CInfoViewer::CInfoViewer()
{
	intShowDuration = g_settings.timing_infobar; //15 means7,5 sec
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
	pthread_mutex_init( &epg_mutex, NULL );

	if (pthread_create (&thrViewer, NULL, InfoViewerThread, (void *) this) != 0 )
	{
		perror("CInfoViewer::CInfoViewer create thrViewer failed\n");
	}

	pthread_cond_init( &lang_cond, NULL );

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

	if ( CalledFromNumZap )
		EPG_NotFound_Text = (char*) g_Locale->getText("infoviewer.epgnotload").c_str();
	else
		EPG_NotFound_Text =  (char*) g_Locale->getText("infoviewer.epgwait").c_str();

	BoxStartX = g_settings.screen_StartX+ 20;
	BoxEndX   = g_settings.screen_EndX- 20;
	BoxEndY   = g_settings.screen_EndY- 20;


	if ( ShowInfo_Info )
		BoxStartY = BoxEndY- InfoHeightY- InfoHeightY_Info+ 6;
	else
		BoxStartY = BoxEndY- InfoHeightY;

	KillShowEPG = false;
	pthread_mutex_unlock( &epg_mutex );


	//frameBuffer->paintVLine(settings->screen_StartX,0,576, 3);
	//frameBuffer->paintVLine(settings->screen_EndX,0,576, 3);
	//frameBuffer->paintHLine(0,719, settings->screen_EndY,3);


	g_FrameBuffer->paintBackgroundBox(BoxStartX, BoxStartY+ ChanHeight, BoxStartX + (ChanWidth >>1), BoxStartY+ ChanHeight+ InfoHeightY_Info+ 10);

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
	g_FrameBuffer->paintBox(ChanNameX, ChanNameY, BoxEndX, BoxEndY, COL_INFOBAR);

	// ... with channel name
	int height=g_Fonts->infobar_channame->getHeight()+5;
	g_Fonts->infobar_channame->RenderString(ChanNameX+ 20, ChanNameY+height, BoxEndX-ChanNameX- 140, Channel.c_str(), COL_INFOBAR);

	//time? todo - thread suxx...
	char timestr[50];
	struct timeb tm;
	ftime(&tm);
	strftime((char*) &timestr, 20, "%H:%M", localtime(&tm.time) );
	int timewidth = g_Fonts->infobar_channame->getRenderWidth(timestr);
	g_Fonts->infobar_channame->RenderString(BoxEndX-timewidth-10, ChanNameY+height, timewidth+5, timestr, COL_INFOBAR);


	ChanInfoX = BoxStartX + (ChanWidth >>1);
	int ChanInfoY = BoxStartY + ChanHeight+10;
	g_FrameBuffer->paintBox(ChanInfoX, ChanInfoY, ChanNameX, BoxEndY, COL_INFOBAR);

	if ( ShowInfo_Info )
	{
		ButtonWidth = (BoxEndX- ChanInfoX)>> 2;

		g_FrameBuffer->paintHLine(ChanInfoX, BoxEndX,  BoxEndY-InfoHeightY_Info, COL_INFOBAR_SHADOW);
		//g_FrameBuffer->paintHLine(ChanInfoX, BoxEndX,  BoxEndY-InfoHeightY_Info+1, COL_INFOBAR_SHADOW); 2Lines wegen scanline?

		g_FrameBuffer->paintIcon("blau.raw", BoxEndX- ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
		g_Fonts->infobar_small->RenderString(BoxEndX- ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 31, g_Locale->getText("infoviewer.streaminfo").c_str(), COL_INFOBAR);

		showButtonNVOD(true);

		g_RemoteControl->CopyAPIDs();
		showButtonAudio();
	}

	pthread_mutex_lock( &epg_mutex );
	is_visible = true;
	pthread_mutex_unlock( &epg_mutex );

	pthread_cond_signal( &epg_cond );

	usleep(50);

	int key;

	if ( !CalledFromNumZap )
	{
		printf("in infobar-input\n");
		key = g_RCInput->getKey( intShowDuration*5 );

		if ( ( key != CRCInput::RC_timeout ) && ( ( key != CRCInput::RC_ok ) || ( CalledFromNumZap ) ) )
		{
			g_RCInput->pushbackKey(key);
		}

		if ( ( key != g_settings.key_quickzap_up ) && ( key != g_settings.key_quickzap_down ) && ( key != CRCInput::RC_help ) )
		{
			killTitle();
		}
	}
}


void CInfoViewer::showButtonNVOD(bool CalledFromShowData = false)
{
	CSubChannel_Infos subChannels= g_RemoteControl->getSubChannels();

	if ( subChannels.has_subChannels_for( getActiveChannelID() ) )
	{
		// gelbe Taste für NVODs / Subservices
		g_FrameBuffer->paintIcon("gelb.raw", BoxEndX- 2* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
		if ( subChannels.are_subchannels )
			// SubServices
			g_Fonts->infobar_small->RenderString(BoxEndX- 2* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.subservice").c_str(), COL_INFOBAR);
		else
			// NVOD
			g_Fonts->infobar_small->RenderString(BoxEndX- 2* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.selecttime").c_str(), COL_INFOBAR);

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
				sprintf((char*) &runningDuration, "%d min", dauer);

				struct      tm *pStartZeit = localtime(&subChannels.list[sel].startzeit);
				sprintf((char*) &runningStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
				runningPercent=(unsigned)((float)(time(NULL)-subChannels.list[sel].startzeit)/(float)subChannels.list[sel].dauer*100.);
				if (runningPercent>100)
					runningPercent=0;

				Flag|= sectionsd::epg_has_current;
				//printf("%s %s %d\n", runningDuration, runningStart, runningPercent);
			}
		}
	}

	int height = g_Fonts->infobar_channame->getHeight()/3;
	//int ChanNameY = BoxStartY + (ChanHeight>>1)+3;
	int ChanInfoY = BoxStartY + ChanHeight+ 15; //+10


	//percent

	if ( ShowInfo_Info )
	{
		int posy = BoxStartY+12;
		int height2= 20;//int( g_Fonts->infobar_channame->getHeight()/1.7);

		//g_FrameBuffer->paintBox(BoxEndX-130, BoxStartY, BoxEndX, ChanNameY+2, COL_INFOBAR+1); //bounding box (off)
		if ( Flag & sectionsd::epg_has_current)
		{
			g_FrameBuffer->paintBoxRel(BoxEndX-114, posy,   2+100+2, height2, COL_INFOBAR_SHADOW); //border
			g_FrameBuffer->paintBoxRel(BoxEndX-112, posy+2, runningPercent+2, height2-4, COL_INFOBAR+7);//fill(active)
			g_FrameBuffer->paintBoxRel(BoxEndX-112+runningPercent, posy+2, 100-runningPercent, height2-4, COL_INFOBAR+3);//fill passive
		}
		if ( Flag & sectionsd::epg_has_anything )
		{
			g_FrameBuffer->paintIcon("rot.raw", BoxEndX- 4* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
			g_Fonts->infobar_small->RenderString(BoxEndX- 4* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.eventlist").c_str(), COL_INFOBAR);
		}

	}

	//info running
	//int start1width      = g_Fonts->infobar_info->getRenderWidth(runningStart);
	int duration1Width   = g_Fonts->infobar_info->getRenderWidth(runningRest);
	int duration1TextPos = BoxEndX-duration1Width-10;
	height = g_Fonts->infobar_info->getHeight();
	int xStart= BoxStartX + ChanWidth + 30;

	if ( is_nvod )
		g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

	if ( Flag & sectionsd::epg_not_broadcast )
	{
		// kein EPG verfügbar
		ChanInfoY += height;
		g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height, COL_INFOBAR);
		g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, BoxEndX- xStart, g_Locale->getText("infoviewer.noepg").c_str(), COL_INFOBAR);
	}
	else
	{
		// irgendein EPG gefunden
		if ( ( Flag & sectionsd::epg_has_next ) && ( !( Flag & sectionsd::epg_has_current )) )
		{
			// spätere Events da, aber kein aktuelles...
			//            ChanInfoY += height;
			g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height, COL_INFOBAR);
			g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, BoxEndX- xStart, g_Locale->getText("infoviewer.nocurrent").c_str(), COL_INFOBAR);


			ChanInfoY += height;

			//info next
			g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

			int duration2Width   = g_Fonts->infobar_info->getRenderWidth(nextDuration);
			int duration2TextPos = BoxEndX-duration2Width-10;
			g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR);
			g_Fonts->infobar_info->RenderString(BoxStartX + ChanWidth + 30,  ChanInfoY+height, duration1TextPos- (BoxStartX + ChanWidth + 40)-10, next, COL_INFOBAR);
			g_Fonts->infobar_info->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR);
		}
		else
		{
			g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, runningStart, COL_INFOBAR);
			g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, duration1TextPos- (BoxStartX + ChanWidth + 40)-10, running, COL_INFOBAR);
			g_Fonts->infobar_info->RenderString(duration1TextPos,            ChanInfoY+height, duration1Width, runningRest, COL_INFOBAR);

			ChanInfoY += height;

			//info next
			g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

			if ( ( !is_nvod ) && ( Flag & sectionsd::epg_has_next ) )
			{
				int duration2Width   = g_Fonts->infobar_info->getRenderWidth(nextDuration);
				int duration2TextPos = BoxEndX-duration2Width-10;
				g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR);
				g_Fonts->infobar_info->RenderString(BoxStartX + ChanWidth + 30,  ChanInfoY+height, duration1TextPos- (BoxStartX + ChanWidth + 40)-10, next, COL_INFOBAR);
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
		if ( ( g_RemoteControl->GetECMPID()== 0 ) || ( g_RemoteControl->audio_chans.count_apids== 0 ) )
		{
			int height = g_Fonts->infobar_info->getHeight();
			int ChanInfoY = BoxStartY + ChanHeight+ 15+ 2* height;
			int xStart= BoxStartX + ChanWidth + 30;

			//int ChanNameX = BoxStartX + ChanWidth + 10;
			int ChanNameY = BoxStartY + ChanHeight + 10;


			string  disp_text;
			if ( ( g_RemoteControl->GetECMPID()== 0 ) && ( g_RemoteControl->audio_chans.count_apids!= 0 ) )
			{
#ifdef USEACTIONLOG
				g_ActionLog->println("cannot decode");
#endif

				disp_text= g_Locale->getText("infoviewer.cantdecode");
			}
			else
			{
#ifdef USEACTIONLOG
				g_ActionLog->println("not available");
#endif

				disp_text= g_Locale->getText("infoviewer.notavailable");
			}

			g_FrameBuffer->paintBox(ChanInfoX, ChanNameY, BoxEndX, ChanInfoY, COL_INFOBAR);
			g_Fonts->infobar_info->RenderString(xStart, ChanInfoY, BoxEndX- xStart, disp_text.c_str(), COL_INFOBAR);
			KillShowEPG = true;
		};


		// grün, wenn mehrere APIDs
		if ( g_RemoteControl->audio_chans.count_apids> 1 )
		{
			g_FrameBuffer->paintIcon("gruen.raw", BoxEndX- 3* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
			g_Fonts->infobar_small->RenderString(BoxEndX- 3* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.languages").c_str(), COL_INFOBAR);
		}
	}
}

void CInfoViewer::showWarte()
{

	int height = g_Fonts->infobar_info->getHeight();
	int ChanInfoY = BoxStartY + ChanHeight+ 15+ 2* height;
	int xStart= BoxStartX + ChanWidth + 30;

	pthread_mutex_trylock( &epg_mutex );
	if ( !KillShowEPG )
		g_Fonts->infobar_info->RenderString(xStart, ChanInfoY, BoxEndX- xStart, EPG_NotFound_Text, COL_INFOBAR);
	pthread_mutex_unlock( &epg_mutex );
}

void CInfoViewer::killTitle()
{
	pthread_mutex_lock( &epg_mutex );
	if (is_visible )
	{
		is_visible = false;
		g_FrameBuffer->paintBackgroundBox(BoxStartX, BoxStartY, BoxEndX, BoxEndY );
	}
	pthread_mutex_unlock( &epg_mutex );
}


void * CInfoViewer::LangViewerThread (void *arg)
{
	CInfoViewer* InfoViewer = (CInfoViewer*) arg;
	while(1)
	{
		pthread_mutex_lock( &InfoViewer->epg_mutex );
		pthread_cond_wait( &InfoViewer->lang_cond, &InfoViewer->epg_mutex );

		if ( ( InfoViewer->is_visible ) && ( InfoViewer->ShowInfo_Info ) )
		{
			g_RemoteControl->CopyAPIDs();
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
	unsigned int    query_onid_tsid;
	bool gotEPG, requeryEPG;
	struct timespec abs_wait;
	struct timeval now;

	CInfoViewer* InfoViewer = (CInfoViewer*) arg;
	while(1)
	{
		pthread_mutex_lock( &InfoViewer->epg_mutex );
		pthread_cond_wait( &InfoViewer->epg_cond, &InfoViewer->epg_mutex );

		if ( ( InfoViewer->is_visible ) )
		{
			gotEPG = true;
			repCount = 10;

			do
			{
				if ( !gotEPG )
				{
					if ( repCount > 0 )
						InfoViewer->showWarte();

					//                    printf("CInfoViewer::InfoViewerThread before waiting long\n");
					//                    usleep( 1000000 );

					gettimeofday(&now, NULL);
					TIMEVAL_TO_TIMESPEC(&now, &abs_wait);
					abs_wait.tv_sec += 1;

					pthread_mutex_trylock( &InfoViewer->epg_mutex );
					pthread_cond_timedwait( &InfoViewer->epg_cond, &InfoViewer->epg_mutex, &abs_wait );

					//                    printf("CInfoViewer::InfoViewerThread after waiting long\n");

					repCount--;
				}

				pthread_mutex_trylock( &InfoViewer->epg_mutex );
				query = InfoViewer->CurrentChannel;
				query_onid_tsid = InfoViewer->Current_onid_tsid;
				pthread_mutex_unlock( &InfoViewer->epg_mutex );


				//                printf("CInfoViewer::InfoViewerThread getEPGData for %s\n", query.c_str());

				gotEPG = InfoViewer->getEPGData(query, query_onid_tsid);
				// gotEPG = gotEPG || ( InfoViewer->Flag & sectionsd::epg_not_broadcast );
				gotEPG = gotEPG && ( InfoViewer->Flag & sectionsd::epg_has_current ) && ( InfoViewer->Flag & sectionsd::epg_has_next ) ;

				if ( ( InfoViewer->Flag & ( sectionsd::epg_has_later | sectionsd::epg_has_current ) ) && (!gotEPG) )
				{
					if (repCount> 3)
					{
						repCount= 3;
						printf("CInfoViewer::InfoViewerThread epg noch nicht komplett -> repCount decreased to %d\n", repCount);
					}
					else
						if (repCount== 1)
							gotEPG= true;
				}


				pthread_mutex_trylock( &InfoViewer->epg_mutex );

				requeryEPG = ( ( (!gotEPG) || (query!=InfoViewer->CurrentChannel) ) &&
				               ( InfoViewer->is_visible ) );

				if (query!=InfoViewer->CurrentChannel)
					repCount = 10;

				if ( InfoViewer->KillShowEPG )
					repCount = 0;


				if ( ( !requeryEPG) && ( InfoViewer->is_visible ) && ( !InfoViewer->KillShowEPG) )
				{
					//                    printf("CInfoViewer::InfoViewerThread success\n");
					InfoViewer->showData();
				}
				else
				{
					//                    printf("CInfoViewer::InfoViewerThread unsuccessful\n");
				}
				pthread_mutex_unlock( &InfoViewer->epg_mutex );


			}
			while ( ( requeryEPG ) && (repCount > 0) );
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
		printf("query epg for onid_tsid >%x< (%s)\n", onid_tsid, channelName.c_str());

		sectionsd::msgResponseHeader resp;
		memset(&resp, 0, sizeof(resp));

		read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

		int nBufSize = resp.dataLength;
		if(nBufSize>0)
		{

			char* pData = new char[nBufSize+1] ;
			read(sock_fd, pData, nBufSize);
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
			sprintf((char*) &runningDuration, "%d min", dauer);
			sprintf((char*) &runningRest, "%d min", rest);

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
