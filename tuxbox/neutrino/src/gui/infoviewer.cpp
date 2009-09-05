/*
	$Id: infoviewer.cpp,v 1.267 2009/09/05 20:26:50 dbt Exp $

	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Bugfixes/cleanups/dreambox port (C) 2007-2009 Stefan Seyfried
	(C) 2008 Novell, Inc. Author: Stefan Seyfried

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/infoviewer.h>
#include <gui/widget/icons.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/progressbar.h>


#include <daemonc/remotecontrol.h>
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */
#include <zapit/client/zapittools.h>

#include <global.h>
#include <neutrino.h>

#include <algorithm>
#include <string>
#include <system/settings.h>
#include <system/helper.h>

#include <time.h>
#include <sys/param.h>

/* for showInfoFile... */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define COL_INFOBAR_BUTTONS            (COL_INFOBAR_SHADOW + 1)
#define COL_INFOBAR_BUTTONS_BACKGROUND (COL_INFOBAR_SHADOW_PLUS_1)

#define ICON_LARGE_WIDTH 26
#define ICON_SMALL_WIDTH 16
#define ICON_HEIGHT 16
#define ICON_OFFSET (2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 6)
#define BOTTOM_BAR_OFFSET 0
#define borderwidth 4

// in us
#define FADE_TIME 40000

#define LCD_UPDATE_TIME_TV_MODE (60 * 1000 * 1000)

#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_start(int tpid);
extern "C" int  tuxtxt_stop();
#endif

int time_left_width;
int time_dot_width;
int time_width;
int time_height;
char old_timestr[10];
/* hack: remember the last shown event IDs to reduce flickering */
static event_id_t last_curr_id = 0, last_next_id = 0;

extern CZapitClient::SatelliteList satList;

static bool sortByDateTime (const CChannelEvent& a, const CChannelEvent& b)
{
	return a.startTime < b.startTime;
}

CInfoViewer::CInfoViewer()
{
	frameBuffer      = CFrameBuffer::getInstance();

	BoxStartX        = BoxStartY = BoxEndX = BoxEndY = 0;
	recordModeActive = false;
	is_visible       = false;
	showButtonBar    = false;
	gotTime          = g_Sectionsd->getIsTimeSet();
	CA_Status        = false;
	virtual_zap_mode = false;
}

void CInfoViewer::start()
{
	InfoHeightY = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getHeight()*9/8 +
		2*g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight() +
		25;
	InfoHeightY_Info = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight()+ 5;

	ChanWidth = 4* g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getRenderWidth(widest_number) + 10;
	ChanHeight = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getHeight()*9/8;

	aspectRatio = g_Controld->getAspectRatio();
	
	time_height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getHeight()+5;
	time_left_width = 2* g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getRenderWidth(widest_number);
	time_dot_width = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getRenderWidth(":");
	time_width = time_left_width* 2+ time_dot_width;

	lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_TV_MODE, false, true );
}

void CInfoViewer::paintTime( bool show_dot, bool firstPaint )
{
	if ( gotTime )
	{
	    ChanNameY = BoxStartY + (ChanHeight>>1)   + SHADOW_OFFSET; //oberkante schatten?

		char timestr[10];
		struct timeval tm;

		gettimeofday(&tm, NULL);
		strftime((char*) &timestr, 10, "%H:%M", localtime(&tm.tv_sec) );

		if ( ( !firstPaint ) && ( strcmp( timestr, old_timestr ) == 0 ) )
		{
			if ( show_dot ) // top dot
				frameBuffer->paintBoxRel(BoxEndX- time_width+ time_left_width- 10, ChanNameY, time_dot_width, time_height/2+2, COL_INFOBAR_PLUS_0);
			else
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(BoxEndX- time_width+ time_left_width- 10, ChanNameY+ time_height, time_dot_width, ":", COL_INFOBAR);
			strcpy( old_timestr, timestr );
		}
		else
		{
			strcpy( old_timestr, timestr );

			if ( !firstPaint ) // background
			{
				// must also be painted with rounded corner on top right, if infobar have also a rounded corner on top right
				frameBuffer->paintBoxRel(BoxEndX- time_width- 10, ChanNameY, time_width+ 10, time_height, COL_INFOBAR_PLUS_0, CORNER_RADIUS_LARGE, CORNER_TOP_RIGHT);
			}

			timestr[2]= 0;
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(BoxEndX- time_width- 10, ChanNameY+ time_height, time_left_width, timestr, COL_INFOBAR);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(BoxEndX- time_left_width- 10, ChanNameY+ time_height, time_left_width, &timestr[3], COL_INFOBAR);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(BoxEndX- time_width+ time_left_width- 10, ChanNameY+ time_height, time_dot_width, ":", COL_INFOBAR);
			if ( show_dot )
				frameBuffer->paintBoxRel(BoxEndX- time_left_width- time_dot_width- 10, ChanNameY, time_dot_width, time_height/2+2, COL_INFOBAR_PLUS_0);
		}
	}
}

void CInfoViewer::showRecordIcon(const bool show)
{
	if(recordModeActive)
	{
		int rec_icon_x = BoxStartX + ChanWidth + 20;
		if(show)
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, rec_icon_x, BoxStartY+10 );
		}
		else
		{
			frameBuffer->paintBackgroundBoxRel(rec_icon_x, BoxStartY+10, 20, 20);
		}
	}
}

/*
		 ___BoxStartX
		|-ChanWidth-|
		|           |  _recording icon                 _progress bar
    BoxStartY---+-----------+ |                               |
	|	|           | *  infobar.txt text            #######____
	|	|           |-------------------------------------------+--ChanNameY
	|	|           | Channelname                               |
    ChanHeight--+-----------+                                           |
		   |                                                    |
		   |01:23     Current Event                             |
		   |02:34     Next Event                                |
		   |                                                    |
    BoxEndY--------+----------------------------------------------------+
		                                                        |
		                                                BoxEndX-/
*/
void CInfoViewer::paintBackground(int col_NumBox)
{
	int c_rad_large = RADIUS_LARGE;
	int c_shadow_width = (c_rad_large * 2) + 1;
	int c_rad_mid = RADIUS_MID;
	int ChanInfoY = BoxStartY + ChanHeight+ SHADOW_OFFSET;
	int BoxEndInfoY = BoxEndY;
	if (showButtonBar) // should we just always kill the button bar area, too?
		BoxEndInfoY += InfoHeightY_Info;
	// kill left side
	frameBuffer->paintBackgroundBox(BoxStartX,
					BoxStartY + ChanHeight - 6,
					BoxStartX + (ChanWidth/3),
					BoxStartY + ChanHeight + InfoHeightY_Info + 10 + 6);
	// kill progressbar + info-line
	frameBuffer->paintBackgroundBox(BoxStartX + ChanWidth + 40, // 40 for the recording icon!
					BoxStartY, BoxEndX, BoxStartY+ ChanHeight);

	// shadow...
	frameBuffer->paintBoxRel(BoxEndX - c_shadow_width,
				 ChanNameY + SHADOW_OFFSET,
				 SHADOW_OFFSET + c_shadow_width,
				 BoxEndInfoY - ChanNameY,
				 COL_INFOBAR_SHADOW_PLUS_0, c_rad_large, CORNER_RIGHT);
	frameBuffer->paintBoxRel(ChanInfoX + SHADOW_OFFSET,
				 BoxEndInfoY - c_shadow_width,
				 BoxEndX - ChanInfoX - SHADOW_OFFSET - c_shadow_width,
				 SHADOW_OFFSET + c_shadow_width,
				 COL_INFOBAR_SHADOW_PLUS_0, c_rad_large, CORNER_BOTTOM_LEFT);

	// background for channel name, epg data
	frameBuffer->paintBoxRel(ChanNameX - SHADOW_OFFSET,
				 ChanNameY,
				 BoxEndX - ChanNameX + SHADOW_OFFSET,
				 BoxEndY - ChanNameY,
				 COL_INFOBAR_PLUS_0, c_rad_large, CORNER_TOP_RIGHT | (showButtonBar ? 0 : CORNER_BOTTOM_RIGHT));

	// number box
	frameBuffer->paintBoxRel(BoxStartX + SHADOW_OFFSET,
				 BoxStartY + SHADOW_OFFSET,
				 ChanWidth,
				 ChanHeight,
				 COL_INFOBAR_SHADOW_PLUS_0, c_rad_mid);
	frameBuffer->paintBoxRel(BoxStartX,
				 BoxStartY,
				 ChanWidth,
				 ChanHeight,
				 col_NumBox, c_rad_mid);
	// paint background left side
	frameBuffer->paintBoxRel(ChanInfoX,
				 ChanInfoY,
				 ChanNameX - ChanInfoX,
				 BoxEndY - ChanInfoY,
				 COL_INFOBAR_PLUS_0, c_rad_large, showButtonBar ? 0 : CORNER_BOTTOM_LEFT);
}

void CInfoViewer::showMovieTitle(const int playstate, const std::string &title, const std::string &sub_title,
				 const int percent, const int ac3state, const int num_apids)
{
	showButtonBar = true;
	bool fadeIn = (g_info.box_Type != CControld::TUXBOX_MAKER_NOKIA) && // dreambox and eNX only 
		g_settings.widget_fade && (!is_visible);

	is_visible = true;
	BoxStartX = g_settings.screen_StartX + 20;
	BoxEndX   = g_settings.screen_EndX - 20;
	BoxStartY = g_settings.screen_EndY - 20 - InfoHeightY - InfoHeightY_Info;
	BoxEndY   = g_settings.screen_EndY - 20 - InfoHeightY_Info;

	if (fadeIn)
	{
		frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(100));
		frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(100));
		frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(100));
		frameBuffer->paletteSet();
#ifdef HAVE_DREAMBOX_HARDWARE
		usleep(100000);	// otherwise, the fade-in-effect is flashing on the dreambox :-(
#endif
	}

	ChanInfoX = BoxStartX + (ChanWidth / 3);
	ChanNameX = BoxStartX + ChanWidth;
	ChanNameY = BoxStartY + (ChanHeight / 2) + SHADOW_OFFSET;
	int c_rad_large = RADIUS_LARGE;

	paintBackground(COL_INFOBAR_PLUS_0);

	//paint play state icon
	const char *icon;
	if (playstate == 4) // CMoviePlayerGui::PAUSE
		icon = "pause.raw";
	else 
		icon = "play.raw";
	/* calculate play state icon position, useful for using customized icons with other sizes */
	int icon_h = frameBuffer->getIconHeight(icon);
	int icon_w = frameBuffer->getIconWidth(icon);
	int icon_x = BoxStartX + ChanWidth / 2 - icon_w / 2;
	int icon_y = BoxStartY + ChanHeight / 2 - icon_h / 2;
	frameBuffer->paintIcon(icon, icon_x, icon_y);

	frameBuffer->paintBoxRel(ChanInfoX, BoxEndY + BOTTOM_BAR_OFFSET,
				 BoxEndX - ChanInfoX, InfoHeightY_Info - BOTTOM_BAR_OFFSET,
				 COL_INFOBAR_BUTTONS_BACKGROUND, c_rad_large, CORNER_BOTTOM);

	// buttons
	const char* txt = g_Locale->getText(LOCALE_MOVIEPLAYER_BOOKMARK);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE,
			       BoxEndX - ICON_OFFSET - ButtonWidth + 2,
			       BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(
				BoxEndX - ICON_OFFSET - ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2),
				BoxEndY + InfoHeightY_Info - 2,
				ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2 + 2),
				txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8

	if (playstate == 4) // CMoviePlayerGui::PAUSE
		txt = g_Locale->getText(LOCALE_AUDIOPLAYER_PLAY);
	else 
		txt = g_Locale->getText(LOCALE_AUDIOPLAYER_PAUSE);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW,
			       BoxEndX- ICON_OFFSET - 2 * ButtonWidth + 2,
			       BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(
				BoxEndX - ICON_OFFSET - 2 * ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2),
				BoxEndY + InfoHeightY_Info - 2,
				ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2 + 2),
				txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8

	if (num_apids > 1)
	{
		txt = g_Locale->getText(LOCALE_INFOVIEWER_LANGUAGES);
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN,
				       BoxEndX - ICON_OFFSET - 3 * ButtonWidth + 2 + 8,
				       BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(
					BoxEndX - ICON_OFFSET - 3 * ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2) + 8,
					BoxEndY + InfoHeightY_Info - 2,
					ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2 + 2),
					txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
	}

	txt = g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP16);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED,
			       BoxEndX - ICON_OFFSET - 4 * ButtonWidth + 2,
			       BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(
				BoxEndX - ICON_OFFSET - 4 * ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2),
				BoxEndY + InfoHeightY_Info - 2,
				ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + 2) + 8,
				txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8

	switch (ac3state)
	{
	case 1:	// AC3_availabe
		icon = "dd_avail.raw";
		break;
	case 2:	// AC3 active
		icon = "dd.raw";
		break;
	case 0:	// no AC3
	default:
		icon = "dd_gray.raw";
		break;
	}
	frameBuffer->paintIcon(icon,
			       BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2 + ICON_SMALL_WIDTH + 6),
			       BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);

	display_Info(title.c_str(), sub_title.c_str(), true, false, (percent * 120) / 100);

	infobarLoop(false, fadeIn);
}

void CInfoViewer::showTitle(const int ChanNum, const std::string & Channel, const t_satellite_position satellitePosition, const t_channel_id new_channel_id, const bool calledFromNumZap, int epgpos)
{
	/* reset the "last shown eventid" markers */
	last_curr_id = last_next_id = 0;
	showButtonBar = !calledFromNumZap;
	ChannelName = Channel;
	bool new_chan = false;
	bool subChannelNameIsUTF = true;
	
	bool fadeIn = (g_info.box_Type != CControld::TUXBOX_MAKER_NOKIA) && // dreambox and eNX only 
		g_settings.widget_fade &&
		(!is_visible) &&
		showButtonBar;

	is_visible = true;
#ifdef ENABLE_RADIOTEXT
	if (g_settings.radiotext_enable && g_Radiotext) {
		g_Radiotext->S_RtOsd = true;
		g_Radiotext->RT_MsgShow = true;
	}
#endif
	
	BoxStartX = g_settings.screen_StartX+ 20;
	BoxEndX   = g_settings.screen_EndX- 20;
	BoxStartY = g_settings.screen_EndY - 20 - InfoHeightY - InfoHeightY_Info;
	BoxEndY   = g_settings.screen_EndY - 20 - InfoHeightY_Info;

	if ( !gotTime )
		gotTime = g_Sectionsd->getIsTimeSet();

	if ( fadeIn )
	{
		frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(100));
		frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(100));
		frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(100));
		frameBuffer->paletteSet();
#ifdef HAVE_DREAMBOX_HARDWARE
		usleep(100000);	// otherwise, the fade-in-effect is flashing on the dreambox :-(
#endif
	}

	int col_NumBoxText;
	int col_NumBox;
	if (virtual_zap_mode)
	{
		col_NumBoxText = COL_MENUHEAD;
		col_NumBox = COL_MENUHEAD_PLUS_0;
		if ((channel_id != new_channel_id) || (evtlist.empty()))
		{
			evtlist.clear();
			evtlist = g_Sectionsd->getEventsServiceKey(new_channel_id);
			if (!evtlist.empty())
				sort(evtlist.begin(),evtlist.end(), sortByDateTime);
			new_chan = true;
		}
	}
	else
	{
		col_NumBoxText = COL_INFOBAR;
		col_NumBox = COL_INFOBAR_PLUS_0;
	}

	// get channel-id
	// ...subchannel is selected
	if (! calledFromNumZap && !(g_RemoteControl->subChannels.empty()) && (g_RemoteControl->selected_subchannel > 0)) 
	{
		channel_id = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].getChannelID();
		ChannelName = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].subservice_name;
		subChannelNameIsUTF = false;
	}
	else // ...channel is selected
	{
		channel_id = new_channel_id;
	}

	//infobox
	ChanInfoX = BoxStartX + (ChanWidth / 3);
	ChanNameX = BoxStartX + ChanWidth;
	ChanNameY = BoxStartY + (ChanHeight>>1)   + SHADOW_OFFSET; //oberkante schatten?
	int c_rad_large = RADIUS_LARGE;
	int ChanNumYPos = BoxStartY + ChanHeight;

	paintBackground(col_NumBox);
	// sat display
	if (g_settings.infobar_sat_display)
	{
		for (CZapitClient::SatelliteList::const_iterator satList_it = satList.begin(); satList_it != satList.end(); satList_it++)
			if (satList_it->satPosition == satellitePosition)
			{
				int satNameWidth = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(satList_it->satName);
				if (satNameWidth > ChanWidth)
					satNameWidth = ChanWidth;

				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxStartX + ((ChanWidth - satNameWidth)>>1), BoxStartY + 22, ChanWidth, satList_it->satName, col_NumBoxText);
				ChanNumYPos += 10;

				break;
			}
	}
	
/* paint channel number, channelname or/and channellogo */
	sprintf((char*) strChanNum, "%d", ChanNum);

	int ChannelLogoMode = showChannelLogo(channel_id); // get logo mode, paint channel logo if adjusted

	if (ChannelLogoMode != 1) // no logo in numberbox
	{
		// show logo in numberbox
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->RenderString(BoxStartX + ((ChanWidth - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getRenderWidth(strChanNum))>>1), ChanNumYPos, ChanWidth, strChanNum, col_NumBoxText);
	}

	ChanNameW = BoxEndX- (ChanNameX+ 20)- time_width- 15; // set channel name width

	// ... with channel name 
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(ChanNameX + 10, ChanNameY+ time_height, BoxEndX- (ChanNameX+ 20)- time_width- 15, ChannelName, COL_INFOBAR, 0, subChannelNameIsUTF); // UTF-8
/* paint channel number, channelname or/and channellogo */

	paintTime( false, true );
	showInfoFile();

	ButtonWidth = (BoxEndX- ChanInfoX- ICON_OFFSET)>> 2;

	if ( showButtonBar )
	{
		sec_timer_id = g_RCInput->addTimer(1000000, false);

		if ( BOTTOM_BAR_OFFSET> 0 )
			frameBuffer->paintBackgroundBox(ChanInfoX, BoxEndY, BoxEndX, BoxEndY+ BOTTOM_BAR_OFFSET);

		frameBuffer->paintBoxRel(ChanInfoX, BoxEndY + BOTTOM_BAR_OFFSET, BoxEndX - ChanInfoX, InfoHeightY_Info - BOTTOM_BAR_OFFSET, COL_INFOBAR_BUTTONS_BACKGROUND, c_rad_large, CORNER_BOTTOM);

		// show blue button
		// USERMENU
		const char* txt = NULL;
		if( !g_settings.usermenu_text[SNeutrinoSettings::BUTTON_BLUE].empty() )
			txt = g_settings.usermenu_text[SNeutrinoSettings::BUTTON_BLUE].c_str();
		else	
			txt = g_Locale->getText(LOCALE_INFOVIEWER_STREAMINFO);

		if ( txt != NULL )
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE,
					       BoxEndX - ICON_OFFSET - ButtonWidth + 2,
					       BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(
					BoxEndX - ICON_OFFSET - ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2),
					BoxEndY + InfoHeightY_Info - 2,
					ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2 + 2),
					txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
		}
		
		showInfoIcons();
	}

	g_Sectionsd->getCurrentNextServiceKey(channel_id, info_CurrentNext);
	if (!calledFromNumZap)
		CLCD::getInstance()->setEPGTitle(info_CurrentNext.current_name);

	if (!evtlist.empty()) {
		if (new_chan) {
			for ( eli=evtlist.begin(); eli!=evtlist.end(); ++eli ) {
				if ((uint)eli->startTime >= info_CurrentNext.current_zeit.startzeit + info_CurrentNext.current_zeit.dauer)
					break;
			}
			if (eli == evtlist.end()) // the end is not valid, so go back
				--eli;
		}

		/* epgpos != 0 => virtual zap mode */
		if (epgpos != 0) {
			info_CurrentNext.flags = 0;
			if ((epgpos > 0) && (eli != evtlist.end())) {
				++eli; // next epg
				if (eli == evtlist.end()) // the end is not valid, so go back
					--eli;
			}
			else if ((epgpos < 0) && (eli != evtlist.begin())) {
				--eli; // prev epg
			}
			info_CurrentNext.flags = CSectionsdClient::epgflags::has_current;
			info_CurrentNext.current_uniqueKey	= eli->eventID;
			info_CurrentNext.current_zeit.startzeit	= eli->startTime;
			info_CurrentNext.current_zeit.dauer	= eli->duration;
			if (eli->description.empty())
				info_CurrentNext.current_name	= ZapitTools::UTF8_to_Latin1(g_Locale->getText(LOCALE_INFOVIEWER_NOEPG));
			else
				info_CurrentNext.current_name	= eli->description;
			info_CurrentNext.current_fsk		= '\0';

			if (eli != evtlist.end()) {
				++eli;
				if (eli != evtlist.end()) {
					info_CurrentNext.flags 			= CSectionsdClient::epgflags::has_current | CSectionsdClient::epgflags::has_next;
					info_CurrentNext.next_uniqueKey		= eli->eventID;
					info_CurrentNext.next_zeit.startzeit 	= eli->startTime;
					info_CurrentNext.next_zeit.dauer	= eli->duration;
					if (eli->description.empty())
						info_CurrentNext.next_name	= ZapitTools::UTF8_to_Latin1(g_Locale->getText(LOCALE_INFOVIEWER_NOEPG));
					else
						info_CurrentNext.next_name	= eli->description;
				}
				--eli;
			}
			if (info_CurrentNext.flags)
				info_CurrentNext.flags |= CSectionsdClient::epgflags::has_anything;
		}
	}

	if (!(info_CurrentNext.flags & (CSectionsdClient::epgflags::has_anything | CSectionsdClient::epgflags::not_broadcast)))
	{
		// nicht gefunden / noch nicht geladen
		/* see the comment in showData() for a reasoning for this calculation */
		int CurrInfoY = (BoxStartY + InfoHeightY + ChanNameY + time_height)/2;	// lower end of current info box
		neutrino_locale_t loc;
		if (! gotTime)
			loc = LOCALE_INFOVIEWER_WAITTIME;
		else if (showButtonBar)
			loc = LOCALE_INFOVIEWER_EPGWAIT;
		else
			loc = LOCALE_INFOVIEWER_EPGNOTLOAD;
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(ChanNameX+ 10, CurrInfoY, BoxEndX- (ChanNameX+ 20), g_Locale->getText(loc), COL_INFOBAR, 0, true); // UTF-8
	}
	else
	{
		show_Data();
	}

	showLcdPercentOver();

	if ( ( g_RemoteControl->current_channel_id == channel_id) &&
		!( ( ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_next ) &&
			( info_CurrentNext.flags & ( CSectionsdClient::epgflags::has_current | CSectionsdClient::epgflags::has_no_current ) ) ) ||
				( info_CurrentNext.flags & CSectionsdClient::epgflags::not_broadcast ) ) )
	{
		// EVENT anfordern!
		g_Sectionsd->setServiceChanged(channel_id, true );
	}

#ifdef ENABLE_RADIOTEXT
	if ((g_settings.radiotext_enable) && (CNeutrinoApp::getInstance()->getMode() == 2)) {
		showRadiotext();
	}
	else if ((!g_settings.radiotext_enable) && (CNeutrinoApp::getInstance()->getMode() == 2))
	{
		showIcon_RadioText(false,false);	
	}
#endif
	infobarLoop(calledFromNumZap, fadeIn);
}

void CInfoViewer::infobarLoop(bool calledFromNumZap, bool fadeIn)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	int fadeValue;
	if (fadeIn)
		fadeValue = 100;
	else
		fadeValue= g_settings.infobar_alpha;
	bool fadeOut = false;

	CNeutrinoApp *neutrino = CNeutrinoApp::getInstance();

	if ( !calledFromNumZap )
	{
		bool tsmode = (neutrino->getMode() == NeutrinoMessages::mode_ts);
		bool show_dot= true;
		if ( fadeIn )
			fadeTimer = g_RCInput->addTimer( FADE_TIME, false );

		bool hideIt = true;
		unsigned long long timeoutEnd = (neutrino->getMode() != NeutrinoMessages::mode_radio) ?  CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR]) : CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR_RADIO]);

		int res = messages_return::none;

		while ( ! ( res & ( messages_return::cancel_info | messages_return::cancel_all ) ) )
		{
			g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
			neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;
			//printf(" g_RCInput->getMsgAbsoluteTimeout %x %x\n", msg, data);
#if 0
There is no need to poll for EPG when we are going to get events from sectionsd. Saves lots of useless
requests to sectionsd.
			if ( !( info_CurrentNext.flags & ( CSectionsdClient::epgflags::has_current ) ) )
			{
				if(difftime(time(&tb),ta) > 1.1)
				{
					time(&ta);
					info_CurrentNext = getEPG(channel_id);
					if ( ( info_CurrentNext.flags & ( CSectionsdClient::epgflags::has_current ) ) )
					{
						show_Data();
						showLcdPercentOver();
					}
				}
			}
#endif

			if ( msg == CRCInput::RC_help )
			{
				g_RCInput->postMsg( NeutrinoMessages::SHOW_EPG, 0 );
				res = messages_return::cancel_info;
			}
			else if ( ( msg == NeutrinoMessages::EVT_TIMER ) && ( data == fadeTimer ) )
			{
				if ( fadeOut )
				{
					fadeValue+= 15;

					if ( fadeValue>= 100 )
					{
						fadeValue= 100;
						g_RCInput->killTimer(fadeTimer);
						res = messages_return::cancel_info;
						frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(100) );
					}
					else
						frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(fadeValue) );
				}
				else
				{
					fadeValue-= 15;
					if ( fadeValue<= g_settings.infobar_alpha )
					{
						fadeValue= g_settings.infobar_alpha;
						g_RCInput->killTimer(fadeTimer);
						fadeTimer = 0;
//						fadeIn = false;
						frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(0) );
					}
					else
						frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(fadeValue) );
				}

				frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(fadeValue) );
				frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(fadeValue) );
				frameBuffer->paletteSet();
			}
			else if ((msg == CRCInput::RC_ok) ||
				 (msg == CRCInput::RC_home) ||
				 (msg == CRCInput::RC_timeout))
			{
				if ( fadeIn )
				{
					if (fadeTimer)
						g_RCInput->killTimer(fadeTimer);
					fadeIn = false;
					fadeOut = true;
					fadeTimer = g_RCInput->addTimer( FADE_TIME, false );
					timeoutEnd = CRCInput::calcTimeoutEnd( 1 );
				}
				else
				{
					if (( msg != CRCInput::RC_timeout ) && (msg != CRCInput::RC_ok))
						g_RCInput->postMsg( msg, data );
					res = messages_return::cancel_info;
				}
			}
			else if (msg_repeatok == g_settings.key_quickzap_up ||
				 msg_repeatok == g_settings.key_quickzap_down ||
				 msg == CRCInput::RC_0 ||
				 msg == NeutrinoMessages::SHOW_INFOBAR)
			{
				hideIt = tsmode; // in movieplayer mode, hide infobar
				g_RCInput->postMsg( msg, data );
				res = messages_return::cancel_info;
			}
			else if ( msg == NeutrinoMessages::EVT_TIMESET )
			{
				// Handle anyway!
				neutrino->handleMsg(msg, data);
				if (!tsmode)
				{
					g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );
					hideIt = false;
					res = messages_return::cancel_all;
				}
			}
			else if ( ( msg == NeutrinoMessages::EVT_TIMER ) && ( data == sec_timer_id ) )
			{
				paintTime( show_dot, false );
				showRecordIcon(show_dot);
				show_dot = !show_dot;

#ifdef ENABLE_RADIOTEXT
				if ((g_settings.radiotext_enable) && (CNeutrinoApp::getInstance()->getMode() == 2)) {
					showRadiotext();
				}
#endif
			}
			else if (!tsmode && g_settings.virtual_zap_mode &&
				 (msg == CRCInput::RC_right || msg == CRCInput::RC_left))
			{
				virtual_zap_mode = true;
				res = messages_return::cancel_all;
				hideIt = true;
			}
			else if (!(msg & CRCInput::RC_Release) && //ignore key release ...
				 msg != (CRCInput::RC_help | CRCInput::RC_Repeat)) //...and help key repeat
			{
				res = neutrino->handleMsg(msg, data);

				if ( res & messages_return::unhandled )
				{
					// raus hier und im Hauptfenster behandeln...
					g_RCInput->postMsg(  msg, data );
					res = messages_return::cancel_info;
				}
			}
		}

		if ( hideIt )
			killTitle();

		g_RCInput->killTimer(sec_timer_id);

		if ( fadeIn || fadeOut )
		{
			g_RCInput->killTimer(fadeTimer);
			frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
			frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
			frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(0) );
			frameBuffer->paletteSet();
		}
		if (virtual_zap_mode)
			CNeutrinoApp::getInstance()->channelList->virtual_zap_mode(msg == CRCInput::RC_right);
	}
}


void CInfoViewer::showSubchan()
{
	CNeutrinoApp *neutrino = CNeutrinoApp::getInstance();

	std::string subChannelName; 	// holds the name of the subchannel/audio channel
	int subchannel=0;		// holds the channel index
	bool subChannelNameIsUTF = false;

	if (!(g_RemoteControl->subChannels.empty())) {
		// get info for nvod/subchannel
		subchannel = g_RemoteControl->selected_subchannel;
		if ( g_RemoteControl->selected_subchannel >= 0)
			subChannelName = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].subservice_name;
	}
	else if (g_RemoteControl->current_PIDs.APIDs.size()>1 && g_settings.audiochannel_up_down_enable)
	{
		// get info for audio channel
		subchannel = g_RemoteControl->current_PIDs.PIDs.selected_apid;
		subChannelName = g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].desc;
		subChannelNameIsUTF = true;
	}

	if (!(subChannelName.empty()))
	{
		char text[100];
		sprintf( text, "%d - %s", subchannel, subChannelName.c_str() );

		int dx = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(text, subChannelNameIsUTF) + 20;
		int dy = 25;
		
		if( g_settings.infobar_subchan_disp_pos == 4 )				
		{
			// show full infobar for subschannel 
			g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR , 0 );
		}
		else
		{	
			if ( g_RemoteControl->director_mode )
			{
				int w= 20+ g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(g_Locale->getText(LOCALE_NVODSELECTOR_DIRECTORMODE), true) + 20; // UTF-8
				if ( w> dx )
					dx= w;
				dy= dy* 2;
			}
			else
				dy= dy +5;
	
			int x=0,y=0;
			if( g_settings.infobar_subchan_disp_pos == 0 )
			{
				// Rechts-Oben
				x = g_settings.screen_EndX - dx - 10;
				y = g_settings.screen_StartY + 10;
			}
			else if( g_settings.infobar_subchan_disp_pos == 1 )
			{
				// Links-Oben
				x = g_settings.screen_StartX + 10;
				y = g_settings.screen_StartY + 10;
			}
			else if( g_settings.infobar_subchan_disp_pos == 2 )
			{
				// Links-Unten
				x = g_settings.screen_StartX + 10;
				y = g_settings.screen_EndY - dy - 10;
			}
			else if( g_settings.infobar_subchan_disp_pos == 3 )
			{
				// Rechts-Unten
				x = g_settings.screen_EndX - dx - 10;
				y = g_settings.screen_EndY - dy - 10;
			}
	
			fb_pixel_t pixbuf[(dx+ 2* borderwidth) * (dy+ 2* borderwidth)];
			frameBuffer->SaveScreen(x- borderwidth, y- borderwidth, dx+ 2* borderwidth, dy+ 2* borderwidth, pixbuf);
	
			// clear border
			frameBuffer->paintBackgroundBoxRel(x- borderwidth, y- borderwidth, dx+ 2* borderwidth, borderwidth);
			frameBuffer->paintBackgroundBoxRel(x- borderwidth, y+ dy, dx+ 2* borderwidth, borderwidth);
			frameBuffer->paintBackgroundBoxRel(x- borderwidth, y, borderwidth, dy);
			frameBuffer->paintBackgroundBoxRel(x+ dx, y, borderwidth, dy);
			
			{			
			// show default small infobar for subchannel
			frameBuffer->paintBoxRel(x, y, dx, dy, COL_MENUCONTENT_PLUS_0, RADIUS_SMALL);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(x+10, y+ 30, dx-20, text, COL_MENUCONTENT, 0, subChannelNameIsUTF); // UTF-8
			
			// show yellow button
			// USERMENU
			const char* txt = NULL;
			if( !g_settings.usermenu_text[SNeutrinoSettings::BUTTON_YELLOW].empty() )
				txt = g_settings.usermenu_text[SNeutrinoSettings::BUTTON_YELLOW].c_str();
			else if(g_RemoteControl->director_mode)	
				txt = g_Locale->getText(LOCALE_NVODSELECTOR_DIRECTORMODE);
	
			if ( txt != NULL )
			{
				frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, x+ 8, y+ dy- 20 );
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+ 30, y+ dy- 2, dx- 40, txt, COL_MENUCONTENT, 0, true); // UTF-8
			}
			
			unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( 2 );
			int res = messages_return::none;
	
			neutrino_msg_t      msg;
			neutrino_msg_data_t data;
	
			while (!(res & (messages_return::cancel_info | messages_return::cancel_all)))
			{
				g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

				if (msg == CRCInput::RC_timeout)
				{
					res = messages_return::cancel_info;
				}
				else
				{
					res = neutrino->handleMsg(msg, data);

					if (res & messages_return::unhandled)
					{
						// raus hier und im Hauptfenster behandeln...
						g_RCInput->postMsg(msg, data);
						res = messages_return::cancel_info;
					}
				}
			}

			frameBuffer->RestoreScreen(x- borderwidth, y- borderwidth, dx+ 2* borderwidth, dy+ 2* borderwidth, pixbuf);
			}
		}
	}
	else
	{
		g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );
	}
}

#ifdef ENABLE_RADIOTEXT
void CInfoViewer::showIcon_RadioText(bool rt_available, bool rt_enabled) const
// painting the icon for radiotext mode
{
	int mode = g_Zapit->getMode();
	std::string rt_icon;
	if ((!virtual_zap_mode) && (mode == 2))
	{
		if (g_settings.radiotext_enable){
				rt_icon = rt_available ? "radiotextget.raw" : "radiotextwait.raw";
			}
		else if (rt_enabled==false){
				rt_icon = "radiotextoff.raw";
			}
		else rt_icon = "radiotextoff.raw";
	}
	frameBuffer->paintIcon(rt_icon, BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2 + ICON_SMALL_WIDTH + 6),BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
}
#endif

void CInfoViewer::showIcon_16_9() const
{
#ifdef ENABLE_RADIOTEXT
	if (g_Zapit->getMode() !=2)
#endif
	frameBuffer->paintIcon((aspectRatio != 0) ? "16_9.raw" : "16_9_gray.raw",
				BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2 + ICON_SMALL_WIDTH + 6),
				BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
}

void CInfoViewer::showIcon_VTXT() const
{
	int vtpid=g_RemoteControl->current_PIDs.PIDs.vtxtpid;
	frameBuffer->paintIcon((vtpid != 0) ? "vtxt.raw" : "vtxt_gray.raw",
				BoxEndX - (ICON_SMALL_WIDTH + 2 + ICON_SMALL_WIDTH + 6),
				BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
#ifndef TUXTXT_CFG_STANDALONE
	if(g_settings.tuxtxt_cache && !CNeutrinoApp::getInstance ()->recordingstatus)
	{
		static int last_vtpid=0;
		if(vtpid !=last_vtpid)
		{
			tuxtxt_stop();
			if(vtpid)
				tuxtxt_start(vtpid);
			last_vtpid=vtpid;
		}
	}
#endif
}

void CInfoViewer::showIcon_SubT() const
{
	int subpid = 0;

	for (unsigned i = 0 ;
		i < g_RemoteControl->current_PIDs.SubPIDs.size() ; i++) {
		if (g_RemoteControl->current_PIDs.SubPIDs[i].pid !=
			g_RemoteControl->current_PIDs.PIDs.vtxtpid) {
			subpid = g_RemoteControl->current_PIDs.SubPIDs[i].pid;
		}
	}

	frameBuffer->paintIcon((subpid != 0) ? "subt.raw" : "subt_gray.raw",
				BoxEndX - (ICON_SMALL_WIDTH + 6),
				BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
}

void CInfoViewer::showInfoIcons()
{
	showButton_SubServices();
	showIcon_16_9();
	showIcon_VTXT();
	showIcon_SubT();
	showButton_Audio();
	showIcon_CA_Status();
}

void CInfoViewer::showFailure()
{
	ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, g_Locale->getText(LOCALE_INFOVIEWER_NOTAVAILABLE), 430); // UTF-8
}

void CInfoViewer::showMotorMoving(int duration)
{
	char text[256];
	char buffer[10];
	
	sprintf(buffer, "%d", duration);
	strcpy(text, g_Locale->getText(LOCALE_INFOVIEWER_MOTOR_MOVING));
	strcat(text, " (");
	strcat(text, buffer);
	strcat(text, " s)");
	
	ShowHintUTF(LOCALE_MESSAGEBOX_INFO, text, g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(text) + 10, duration); // UTF-8
}

#ifdef ENABLE_RADIOTEXT
void CInfoViewer::killRadiotext()
{
	frameBuffer->paintBackgroundBox(rt_x, rt_y, rt_w, rt_h);
}

void CInfoViewer::showRadiotext()
{
	char stext[3][100];
	int yoff = 8, ii = 0;
	bool RTisIsUTF = false;

	if (g_Radiotext == NULL) return;

	if (1 && g_Radiotext->S_RtOsd) {
		showIcon_RadioText(g_Radiotext->RT_MsgShow, true);

		// dimensions of radiotext window
		rt_dx = BoxEndX - BoxStartX;
		rt_dy = 25;
		rt_x = BoxStartX;
		rt_y = g_settings.screen_StartY + 10;
		rt_h = rt_y + 7 + rt_dy*(g_Radiotext->S_RtOsdRows+1)+SHADOW_OFFSET;
		rt_w = rt_x+rt_dx+SHADOW_OFFSET;
		
		int lines = 0;
		for (int i = 0; i < g_Radiotext->S_RtOsdRows; i++) {
			if (g_Radiotext->RT_Text[i][0] != '\0') lines++;
		}
		if (lines == 0)
			frameBuffer->paintBackgroundBox(rt_x, rt_y, rt_w, rt_h);

		if (g_Radiotext->RT_MsgShow) {

			if (g_Radiotext->S_RtOsdTitle == 1) {

		// Title
		//	sprintf(stext[0], g_Radiotext->RT_PTY == 0 ? "%s - %s %s%s" : "%s - %s (%s)%s",
		//	g_Radiotext->RT_Titel, tr("Radiotext"), g_Radiotext->RT_PTY == 0 ? g_Radiotext->RDS_PTYN : g_Radiotext->ptynr2string(g_Radiotext->RT_PTY), g_Radiotext->RT_MsgShow ? ":" : tr("  [waiting ...]"));
				if ((lines) || (g_Radiotext->RT_PTY !=0)) {
					sprintf(stext[0], g_Radiotext->RT_PTY == 0 ? "%s %s%s" : "%s (%s)%s", tr("Radiotext"), g_Radiotext->RT_PTY == 0 ? g_Radiotext->RDS_PTYN : g_Radiotext->ptynr2string(g_Radiotext->RT_PTY), ":");
					
					// shadow
					frameBuffer->paintBoxRel(rt_x+SHADOW_OFFSET, rt_y+SHADOW_OFFSET, rt_dx, rt_dy, COL_INFOBAR_SHADOW_PLUS_0, CORNER_RADIUS_LARGE, CORNER_TOP);
					frameBuffer->paintBoxRel(rt_x, rt_y, rt_dx, rt_dy, COL_INFOBAR_PLUS_0, CORNER_RADIUS_LARGE, CORNER_TOP);
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rt_x+10, rt_y+ 30, rt_dx-20, stext[0], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
				}
				yoff = 17;
				ii = 1;
#if 0
			// RDS- or Rass-Symbol, ARec-Symbol or Bitrate
			int inloff = (ftitel->Height() + 9 - 20) / 2;
			if (Rass_Flags[0][0]) {
				osd->DrawBitmap(Setup.OSDWidth-51, inloff, rass, bcolor, fcolor);
				if (ARec_Record)
					osd->DrawBitmap(Setup.OSDWidth-107, inloff, arec, bcolor, 0xFFFC1414);	// FG=Red
				else
					inloff = (ftitel->Height() + 9 - ftext->Height()) / 2;
				osd->DrawText(4, inloff, RadioAudio->bitrate, fcolor, clrTransparent, ftext, Setup.OSDWidth-59, ftext->Height(), taRight);
			}
			else {
				osd->DrawBitmap(Setup.OSDWidth-84, inloff, rds, bcolor, fcolor);
				if (ARec_Record)
					osd->DrawBitmap(Setup.OSDWidth-140, inloff, arec, bcolor, 0xFFFC1414);	// FG=Red
				else
					inloff = (ftitel->Height() + 9 - ftext->Height()) / 2;
				osd->DrawText(4, inloff, RadioAudio->bitrate, fcolor, clrTransparent, ftext, Setup.OSDWidth-92, ftext->Height(), taRight);
			}
#endif
			}
			// Body
			if (lines) {
				frameBuffer->paintBoxRel(rt_x+SHADOW_OFFSET, rt_y+rt_dy+SHADOW_OFFSET, rt_dx, 7+rt_dy*lines, COL_INFOBAR_SHADOW_PLUS_0, CORNER_RADIUS_LARGE, CORNER_BOTTOM);
				frameBuffer->paintBoxRel(rt_x, rt_y+rt_dy, rt_dx, 7+rt_dy*lines, COL_INFOBAR_PLUS_0, CORNER_RADIUS_LARGE, CORNER_BOTTOM);

				// RT-Text roundloop
				int ind = (g_Radiotext->RT_Index == 0) ? g_Radiotext->S_RtOsdRows - 1 : g_Radiotext->RT_Index - 1;
				int rts_x = rt_x+10;
				int rts_y = rt_y+ 30;
				int rts_dx = rt_dx-20;
				if (g_Radiotext->S_RtOsdLoop == 1) { // latest bottom
					for (int i = ind+1; i < g_Radiotext->S_RtOsdRows; i++)
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rts_x, rts_y + (ii++)*rt_dy, rts_dx, g_Radiotext->RT_Text[i], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
					for (int i = 0; i <= ind; i++)
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rts_x, rts_y + (ii++)*rt_dy, rts_dx, g_Radiotext->RT_Text[i], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
				}
				else { // latest top
					for (int i = ind; i >= 0; i--)
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rts_x, rts_y + (ii++)*rt_dy, rts_dx, g_Radiotext->RT_Text[i], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
					for (int i = g_Radiotext->S_RtOsdRows-1; i > ind; i--)
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rts_x, rts_y + (ii++)*rt_dy, rts_dx, g_Radiotext->RT_Text[i], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
				}
			}
#if 0
			// + RT-Plus or PS-Text = 2 rows
			if ((S_RtOsdTags == 1 && RT_PlusShow) || S_RtOsdTags >= 2) {
				if (!RDS_PSShow || !strstr(RTP_Title, "---") || !strstr(RTP_Artist, "---")) {
					sprintf(stext[1], "> %s  %s", tr("Title  :"), RTP_Title);
					sprintf(stext[2], "> %s  %s", tr("Artist :"), RTP_Artist);
					osd->DrawText(4, 6+yoff+fheight*(ii++), stext[1], fcolor, clrTransparent, ftext, Setup.OSDWidth-4, ftext->Height());
					osd->DrawText(4, 3+yoff+fheight*(ii++), stext[2], fcolor, clrTransparent, ftext, Setup.OSDWidth-4, ftext->Height());
				}
				else {
					char *temp = "";
					int ind = (RDS_PSIndex == 0) ? 11 : RDS_PSIndex - 1;
					for (int i = ind+1; i < 12; i++)
						asprintf(&temp, "%s%s ", temp, RDS_PSText[i]);
					for (int i = 0; i <= ind; i++)
						asprintf(&temp, "%s%s ", temp, RDS_PSText[i]);
					snprintf(stext[1], 6*9, "%s", temp);
					snprintf(stext[2], 6*9, "%s", temp+(6*9));
					free(temp);
					osd->DrawText(6, 6+yoff+fheight*ii, "[", fcolor, clrTransparent, ftext, 12, ftext->Height());
					osd->DrawText(Setup.OSDWidth-12, 6+yoff+fheight*ii, "]", fcolor, clrTransparent, ftext, Setup.OSDWidth-6, ftext->Height());
					osd->DrawText(16, 6+yoff+fheight*(ii++), stext[1], fcolor, clrTransparent, ftext, Setup.OSDWidth-16, ftext->Height(), taCenter);
					osd->DrawText(6, 3+yoff+fheight*ii, "[", fcolor, clrTransparent, ftext, 12, ftext->Height());
					osd->DrawText(Setup.OSDWidth-12, 3+yoff+fheight*ii, "]", fcolor, clrTransparent, ftext, Setup.OSDWidth-6, ftext->Height());
					osd->DrawText(16, 3+yoff+fheight*(ii++), stext[2], fcolor, clrTransparent, ftext, Setup.OSDWidth-16, ftext->Height(), taCenter);
				}
			}
#endif
		}
#if 0
// framebuffer can only display raw images
		// show mpeg-still
		char *image;
		if (g_Radiotext->Rass_Archiv >= 0)
			asprintf(&image, "%s/Rass_%d.mpg", DataDir, g_Radiotext->Rass_Archiv);
		else
			asprintf(&image, "%s/Rass_show.mpg", DataDir);
		frameBuffer->useBackground(frameBuffer->loadBackground(image));// set useBackground true or false
		frameBuffer->paintBackground();
//		RadioAudio->SetBackgroundImage(image);
		free(image);
#endif
	}
	g_Radiotext->RT_MsgShow = false;

}
#endif /* ENABLE_RADIOTEXT */

int CInfoViewer::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	if ((msg == NeutrinoMessages::EVT_CURRENTNEXT_EPG) ||
	    (msg == NeutrinoMessages::EVT_NEXTPROGRAM    ))
	{
		if ((*(t_channel_id *)data) == channel_id)
		{
			getEPG(*(t_channel_id *)data, info_CurrentNext);
			CLCD::getInstance()->setEPGTitle(info_CurrentNext.current_name);
			if (is_visible && showButtonBar) // if we are called from numzap, showButtonBar is false
				show_Data( true );
			showLcdPercentOver();
		}
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_TIMER )
	{
		if ( data == fadeTimer )
		{
			// hierher kann das event nur dann kommen, wenn ein anderes Fenster im Vordergrund ist!
			g_RCInput->killTimer(fadeTimer);
			frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
			frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
			frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(0) );
			frameBuffer->paletteSet();

			return messages_return::handled;
		}
		else if ( data == lcdUpdateTimer )
		{
			if ( is_visible )
				show_Data( true );
			showLcdPercentOver();
			return messages_return::handled;
		}
		else if ( data == sec_timer_id )
			return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_RECORDMODE )
	{
		recordModeActive = data;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_GOTAPIDS )
	{
		if ((*(t_channel_id *)data) == channel_id)
		{
			if ( is_visible && showButtonBar )
				showButton_Audio();
#ifdef ENABLE_RADIOTEXT
			if (g_settings.radiotext_enable && g_Radiotext && ((CNeutrinoApp::getInstance()->getMode()) == NeutrinoMessages::mode_radio))
				g_Radiotext->setPid(g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
#endif
		}
	    return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_GOTPIDS )
	{
		if ((*(t_channel_id *)data) == channel_id)
		{
			if ( is_visible && showButtonBar ) {
				showIcon_VTXT();
				showIcon_SubT();
			}
		}
	    return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_GOT_SUBSERVICES )
	{
		if ((*(t_channel_id *)data) == channel_id)
		{
			if ( is_visible && showButtonBar )
				showButton_SubServices();
		}
	    return messages_return::handled;
	}
	else if ((msg == NeutrinoMessages::EVT_ZAP_COMPLETE) ||
		 (msg == NeutrinoMessages::EVT_ZAP_ISNVOD))
	{
		channel_id = (*(t_channel_id *)data);
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_ZAP_SUB_COMPLETE)
	{
		//if ((*(t_channel_id *)data) == channel_id)
		{
			if ( is_visible && showButtonBar &&  ( !g_RemoteControl->are_subchannels ) )
				show_Data( true );
		}
		showLcdPercentOver();
		eventname	= info_CurrentNext.current_name;
		CLCD::getInstance()->setEPGTitle(eventname);
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_ZAP_FAILED)
	{
		if ((*(t_channel_id *)data) == channel_id)
		{
			// show failure..!
			CLCD::getInstance()->showServicename("(" + g_RemoteControl->getCurrentChannelName() + ')');
			printf("zap failed!\n");
			showFailure();
			CLCD::getInstance()->showPercentOver(255);
		}
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_MOTOR)
	{
		showMotorMoving(data);
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_MODECHANGED )
	{
		aspectRatio = data & 0xFF; // strip away VCR aspect ratio
		if ( is_visible && showButtonBar )
			showIcon_16_9();

		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_TIMESET )
	{
		gotTime = true;
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_CA_CLEAR )
	{
		Set_CA_Status(false);
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_CA_LOCK )
	{
		Set_CA_Status(true);
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_CA_FTA )
	{
		Set_CA_Status(false);
		return messages_return::handled;
	}

   return messages_return::unhandled;
}


void CInfoViewer::showButton_SubServices()
{
	// show yellow button
	// USERMENU
	const char* txt = NULL;
	if( !g_settings.usermenu_text[SNeutrinoSettings::BUTTON_YELLOW].empty() )
		txt = g_settings.usermenu_text[SNeutrinoSettings::BUTTON_YELLOW].c_str();
	else if( !(g_RemoteControl->subChannels.empty()) )	
		txt = g_Locale->getText((g_RemoteControl->are_subchannels) ? LOCALE_INFOVIEWER_SUBSERVICE : LOCALE_INFOVIEWER_SELECTTIME);

	if ( txt != NULL )
	{
		// yellow button for subservices / NVODs
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW,
				       BoxEndX- ICON_OFFSET - 2 * ButtonWidth + 2,
				       BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(
					BoxEndX - ICON_OFFSET - 2 * ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2),
					BoxEndY + InfoHeightY_Info - 2,
					ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2 + 2),
					txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
	}
}

void CInfoViewer::getEPG(const t_channel_id for_channel_id, CSectionsdClient::CurrentNextInfo &info)
{
	static CSectionsdClient::CurrentNextInfo oldinfo;

	g_Sectionsd->getCurrentNextServiceKey(for_channel_id, info );

	if (info.current_uniqueKey != oldinfo.current_uniqueKey && info.next_uniqueKey != oldinfo.next_uniqueKey)
	{
		char *p = new char[sizeof(t_channel_id)];
		memcpy(p, &for_channel_id, sizeof(t_channel_id));
		neutrino_msg_t msg;
		if (info.flags & (CSectionsdClient::epgflags::has_current | CSectionsdClient::epgflags::has_next))
		{
			if (info.flags & CSectionsdClient::epgflags::has_current)
				msg = NeutrinoMessages::EVT_CURRENTEPG;
			else
				msg = NeutrinoMessages::EVT_NEXTEPG;
		}
		else
			msg = NeutrinoMessages::EVT_NOEPG_YET;
		g_RCInput->postMsg(msg, (const neutrino_msg_data_t)p, false); // data is pointer to allocated memory
		memcpy(&oldinfo, &info, sizeof(CSectionsdClient::CurrentNextInfo));
	}
}

void CInfoViewer::display_Info(const char *current, const char *next,
			       bool UTF8, bool starttimes, const int pb_pos,
			       const char *runningStart, const char *runningRest,
			       const char *nextStart, const char *nextDuration,
			       bool update_current, bool update_next)
{
	/* dimensions of the two-line current-next "box":
	   top of box    == ChanNameY + time_height (bottom of channel name)
	   bottom of box == BoxStartY + InfoHeightY
	   height of box == (BoxStartY + InfoHeightY) - (ChanNameY + time_height)
	   middle of box == top + height / 2
			 == ChanNameY + time_height + (BoxStartY + InfoHeightY - (ChanNameY + time_height))/2
			 == ChanNameY + time_height + (BoxStartY + InfoHeightY - ChanNameY - time_height)/2
			 == (BoxStartY + InfoHeightY + ChanNameY + time_height)/2
	   The bottom of current info and the top of next info is == middle of box.
	 */

	int height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight();
	int CurrInfoY = (BoxStartY + InfoHeightY + ChanNameY + time_height)/2;	// lower end of current info box
	int NextInfoY = CurrInfoY + height;	// lower end of next info box
	int xStart;
	int InfoX = ChanInfoX + 10;

	if (starttimes)
		xStart = BoxStartX + ChanWidth;
	else
		xStart = InfoX;

	if (pb_pos > -1)
	{
		int pb_p = pb_pos;
		int pb_w = 112;
		int pb_h = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight() - 4;
		if (pb_p > pb_w)
			pb_p = pb_w;
		CProgressBar pb;
		pb.paintProgressBar(BoxEndX - pb_w - SHADOW_OFFSET, BoxStartY + 12, pb_w, pb_h, pb_p, pb_w,
				    0, 0, COL_SILVER, COL_INFOBAR_SHADOW, "", COL_INFOBAR);
	}

	int currTimeW = 0;
	int nextTimeW = 0;
	if (runningRest != NULL)
		currTimeW = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(runningRest, UTF8);
	if (nextDuration != NULL)
		nextTimeW = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(nextDuration, UTF8);
	int currTimeX = BoxEndX - currTimeW - 10;
	int nextTimeX = BoxEndX - nextTimeW - 10;
	static int oldCurrTimeX = currTimeX; // remember the last pos. of remaining time, in case we change from 20/100min to 21/99min

	if (current != NULL && update_current)
	{
		frameBuffer->paintBox(InfoX, CurrInfoY - height, currTimeX, CurrInfoY, COL_INFOBAR_PLUS_0);
		if (runningStart != NULL)
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(InfoX, CurrInfoY, 100, runningStart, COL_INFOBAR, 0, UTF8);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(xStart, CurrInfoY, currTimeX - xStart - 5, current, COL_INFOBAR, 0, UTF8);
		oldCurrTimeX = currTimeX;
	}

	if (currTimeX < oldCurrTimeX)
		oldCurrTimeX = currTimeX;
	frameBuffer->paintBox(oldCurrTimeX, CurrInfoY-height, BoxEndX, CurrInfoY, COL_INFOBAR_PLUS_0);
	oldCurrTimeX = currTimeX;
	if (currTimeW != 0)
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(currTimeX, CurrInfoY, currTimeW, runningRest, COL_INFOBAR, 0, UTF8);

	if (next != NULL && update_next)
	{
		frameBuffer->paintBox(InfoX, NextInfoY-height, BoxEndX, NextInfoY, COL_INFOBAR_PLUS_0);
		if (nextStart != NULL)
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(InfoX, NextInfoY, 100, nextStart, COL_INFOBAR, 0, UTF8);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(xStart, NextInfoY, nextTimeX - xStart - 5, next, COL_INFOBAR, 0, UTF8);
		if (nextTimeW != 0)
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(nextTimeX, NextInfoY, nextTimeW, nextDuration, COL_INFOBAR, 0, UTF8);
	}
}

void CInfoViewer::show_Data(bool calledFromEvent)
{
	if (!is_visible) // no need to do anything else...
		return;

	CNeutrinoApp *neutrino = CNeutrinoApp::getInstance();
	if (neutrino->getMode() == NeutrinoMessages::mode_ts)
		return;	//do nothing in movieplayer mode

	char runningStart[10];
	char runningRest[20];
	int progressbarPos = -1;
	char nextStart[10];
	char nextDuration[10];
	bool is_nvod = false;

	if ((g_RemoteControl->current_channel_id == channel_id) &&
	    (g_RemoteControl->subChannels.size()> 0 ) && !g_RemoteControl->are_subchannels)
	{
		is_nvod = true;
		info_CurrentNext.current_zeit.startzeit = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].startzeit;
		info_CurrentNext.current_zeit.dauer = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].dauer;
	}
	/* what is this for? is it for preventing overlapping events to be displayed? */
	else
	{
		if ((info_CurrentNext.flags & CSectionsdClient::epgflags::has_current) &&
		    (info_CurrentNext.flags & CSectionsdClient::epgflags::has_next) &&
		    showButtonBar)
		{
			if ((uint)info_CurrentNext.next_zeit.startzeit < info_CurrentNext.current_zeit.startzeit + info_CurrentNext.current_zeit.dauer)
			{
				is_nvod = true;
			}
		}
	}
	
	time_t jetzt=time(NULL);
	if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current)
	{
		int seit = (jetzt - info_CurrentNext.current_zeit.startzeit + 30) / 60;
		int rest = (info_CurrentNext.current_zeit.dauer / 60) - seit ;
		if (seit< 0)
		{
			progressbarPos = 0;
			sprintf((char*)&runningRest, "in %d min", -seit);
		}
		else
		{
			progressbarPos = (jetzt - info_CurrentNext.current_zeit.startzeit) * 112 / info_CurrentNext.current_zeit.dauer;
			if (rest >= 0)
				sprintf((char*)&runningRest, "%d / %d min", seit, rest);
			else 
				sprintf((char*)&runningRest, "%d +%d min", info_CurrentNext.current_zeit.dauer / 60, -rest);
		}
		struct tm *pStartZeit = localtime(&info_CurrentNext.current_zeit.startzeit);
		sprintf((char*)&runningStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
	} else
		last_curr_id = 0;

	if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_next)
	{
		unsigned dauer = info_CurrentNext.next_zeit.dauer / 60;
		sprintf((char*)&nextDuration, "%d min", dauer);
		struct tm *pStartZeit = localtime(&info_CurrentNext.next_zeit.startzeit);
		sprintf((char*)&nextStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
	} else
		last_next_id = 0;

	if (showButtonBar)
	{
		// show percent/event progressbar?
		if (!(info_CurrentNext.flags & CSectionsdClient::epgflags::has_current))
			progressbarPos = -1; // no!

		// show red button
		// USERMENU
		const char* txt = NULL;
		if(!g_settings.usermenu_text[SNeutrinoSettings::BUTTON_RED].empty())
			txt = g_settings.usermenu_text[SNeutrinoSettings::BUTTON_RED].c_str();
		else if(info_CurrentNext.flags & CSectionsdClient::epgflags::has_anything)
			txt = g_Locale->getText(LOCALE_INFOVIEWER_EVENTLIST);

		if (txt != NULL)
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED,
					       BoxEndX - ICON_OFFSET - 4 * ButtonWidth + 2,
					       BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(
						BoxEndX - ICON_OFFSET - 4 * ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2),
						BoxEndY + InfoHeightY_Info - 2,
						ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + 2) + 8,
						txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
		}
	}

	if ((info_CurrentNext.flags & CSectionsdClient::epgflags::not_broadcast) ||
	    (calledFromEvent) && !(info_CurrentNext.flags & (CSectionsdClient::epgflags::has_next|CSectionsdClient::epgflags::has_current)))
	{
		// no EPG available
		display_Info(NULL, g_Locale->getText(gotTime ? LOCALE_INFOVIEWER_NOEPG : LOCALE_INFOVIEWER_WAITTIME));
		/* send message. Parental pin check gets triggered on EPG events... */
		char *p = new char[sizeof(t_channel_id)];
		memcpy(p, &channel_id, sizeof(t_channel_id));
		g_RCInput->postMsg(NeutrinoMessages::EVT_NOEPG_YET, (const neutrino_msg_data_t)p, false); // data is pointer to allocated memory
		return;
	}

	// irgendein EPG gefunden
	const char *current   = NULL;
	const char *curr_time = NULL;
	const char *curr_rest = NULL;
	const char *next      = NULL;
	const char *next_time = NULL;
	const char *next_dur  = NULL;
	bool curr_upd = true;
	bool next_upd = true;

	if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current)
	{
		if (info_CurrentNext.current_uniqueKey != last_curr_id)
		{
			last_curr_id = info_CurrentNext.current_uniqueKey;
			curr_time = runningStart;
			current = info_CurrentNext.current_name.c_str();
		}
		else
			curr_upd = false;
		curr_rest = runningRest;
	}
	else
		current = g_Locale->getText(LOCALE_INFOVIEWER_NOCURRENT);

	if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_next)
	{
		if (!(is_nvod && (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current))
		    && info_CurrentNext.next_uniqueKey != last_next_id)
		{	/* if current is shown, show next only if !nvod. Why? I don't know */
			//printf("SHOWDATA: last_next_id = 0x%016llx next_id = 0x%016llx\n", last_next_id, info_CurrentNext.next_uniqueKey);
			last_next_id = info_CurrentNext.next_uniqueKey;
			next_time = nextStart;
			next = info_CurrentNext.next_name.c_str();
			next_dur = nextDuration;
		}
		else
			next_upd = false;
	}
	display_Info(current, next, false, true, progressbarPos, curr_time, curr_rest, next_time, next_dur, curr_upd, next_upd);
}

void CInfoViewer::showInfoFile()
{
	char infotext[80];
	int fd, xStart, xEnd, height, r;
	ssize_t cnt;

	fd = open("/tmp/infobar.txt", O_RDONLY);

	if (fd < 0)
		return;

	cnt = read(fd, infotext, 79);
	if (cnt < 0) {
		fprintf(stderr, "CInfoViewer::showInfoFile: could not read from infobar.txt: %m");
		close(fd);
		return;
	}
	close(fd);
	infotext[cnt] = '\0';

	xStart = BoxStartX + ChanWidth + 40;	// right of record icon
	xEnd   = BoxEndX - 125;			// left of progressbar
	height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight() + 2;
	r = height / 3;
	// background
	frameBuffer->paintBox(xStart, BoxStartY, xEnd, BoxStartY + height, COL_INFOBAR_PLUS_0, r);

	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(
		xStart + r, BoxStartY + height, xEnd - xStart - r*2, (std::string)infotext, COL_INFOBAR, height, false);
}

void CInfoViewer::showButton_Audio()
{
/*        std::string  to_compare= getActiveChannelID();

        if ( strcmp(g_RemoteControl->audio_chans.name, to_compare.c_str() )== 0 )
        {
                if ( ( g_RemoteControl->ecmpid == invalid_ecmpid_found ) ||
                     ( ( g_RemoteControl->audio_chans.count_apids == 0 ) && ( g_RemoteControl->vpid == 0 ) ) )
                {
                        int height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight();
                        int ChanInfoY = BoxStartY + ChanHeight+ 15+ 2* height;
                        int xStart= BoxStartX + ChanWidth + 20;

                        //int ChanNameX = BoxStartX + ChanWidth + 10;
                        int ChanNameY = BoxStartY + ChanHeight + 10;


                        std::string  disp_text;
                        if ( ( g_RemoteControl->ecmpid == invalid_ecmpid_found ) )
			{
			disp_text= g_Locale->getText(LOCALE_INFOVIEWER_CANTDECODE);
			}
                        else
			{
			disp_text= g_Locale->getText(LOCALE_INFOVIEWER_NOTAVAILABLE);
			}

                        frameBuffer->paintBox(ChanInfoX, ChanNameY, BoxEndX, ChanInfoY, COL_INFOBAR_PLUS_0);
                        g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(xStart, ChanInfoY, BoxEndX- xStart, disp_text, COL_INFOBAR, 0, true); // UTF-8
                        KillShowEPG = true;
                };
*/
	// green, in case of several APIDs
	// -- always show Audio Option, due to audio option restructuring (2005-08-31 rasc)
	uint count = g_RemoteControl->current_PIDs.APIDs.size();

	// show green button
	// USERMENU
	const char* txt = NULL;
	if( !g_settings.usermenu_text[SNeutrinoSettings::BUTTON_GREEN].empty() )
		txt = g_settings.usermenu_text[SNeutrinoSettings::BUTTON_GREEN].c_str();
	else if( g_settings.audio_left_right_selectable || count > 1 )	
		txt = g_Locale->getText(LOCALE_INFOVIEWER_LANGUAGES);

	if ( txt != NULL )
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN,
				       BoxEndX - ICON_OFFSET - 3 * ButtonWidth + 2 + 8,
				       BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(
					BoxEndX - ICON_OFFSET - 3 * ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2) + 8,
					BoxEndY + InfoHeightY_Info - 2,
					ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2 + 2),
					txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
	};

	const char * dd_icon;
	if ( ( g_RemoteControl->current_PIDs.PIDs.selected_apid < count ) &&
	     ( g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].is_ac3 ) )
		dd_icon = "dd.raw";
	else if ( g_RemoteControl->has_ac3 )
		dd_icon = "dd_avail.raw";
	else
		dd_icon = "dd_gray.raw";

	frameBuffer->paintIcon(dd_icon,
			       BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2 + ICON_SMALL_WIDTH + 6),
			       BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
}

void CInfoViewer::killTitle()
{
	if (is_visible )
	{
		is_visible = false;
		int bottom = BoxEndY + SHADOW_OFFSET;
		if (showButtonBar)
			bottom += InfoHeightY_Info;
		frameBuffer->paintBackgroundBox(BoxStartX, BoxStartY, BoxEndX+ SHADOW_OFFSET, bottom);
#ifdef ENABLE_RADIOTEXT
		if (g_settings.radiotext_enable && g_Radiotext) {
			g_Radiotext->S_RtOsd = 0;
			killRadiotext();
		}
#endif
	}
	showButtonBar = false;
}

void CInfoViewer::showIcon_CA_Status() const
{
	frameBuffer->paintIcon((CA_Status) ? "ca.raw" : "fta.raw",
				BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2 + ICON_SMALL_WIDTH + 6),
				BoxEndY + (InfoHeightY_Info - ICON_HEIGHT) / 2);
}

void CInfoViewer::Set_CA_Status(int Status)
{
	CA_Status = Status;
	if ( is_visible && showButtonBar )
		showIcon_CA_Status();
}

int CInfoViewer::showChannelLogo( const t_channel_id logo_channel_id  )
/* ****************************************************************************
returns mode of painted channel logo,
0 = no logo painted
1 = in number box 
2 = in place of channel name
3 = beside channel name
*******************************************************************************
*/
{
	char strChanId[16];
	sprintf((char*) strChanId, "%llx", logo_channel_id);
	std::string	mimetype = "raw",
			strLogoIDName = (std::string)strChanId + "." + mimetype,
			strLogoName = ChannelName + "." + mimetype,
			strAbsIconChIDPath = (std::string)g_settings.infobar_channel_logodir +"/"+ strLogoIDName,
			strAbsIconChNamePath = (std::string)g_settings.infobar_channel_logodir +"/"+ strLogoName,
			strAbsIconPath,
			strErrText= "[infoviewer] error while painting channel logo\n -> channel logo too large...use maximal %2dpx%2dpx or change display mode\n -> current logo size: %2dpx%2dpx\n -> current mode: %d\n";	

	int x_mid, y_mid, logo_w, logo_h; 
	int logo_x=0, logo_y=0;
	int res = 0;
	int start_x = ChanNameX, chan_w = BoxEndX- (start_x+ 20)- time_width- 15;
	
	bool logo_available;
	
	if (g_settings.infobar_show_channellogo) // show logo only if "infobar_show_channellogo" adjusted to true, else use defaults
	{

		// check if logo is available
		if (access(strAbsIconChIDPath.c_str(), 0) != -1)
		{
			strAbsIconPath = strAbsIconChIDPath;
			logo_available = true;
		}
		else if (access(strAbsIconChNamePath.c_str(), 0) != -1)
		{
			strAbsIconPath = strAbsIconChNamePath; // strLogoName;
			logo_available = true;
		}
		else
			logo_available = false;
		
		if (logo_available)
		{
			// get logo sizes
			logo_w = frameBuffer->getIconWidth(strAbsIconPath.c_str());
			logo_h = frameBuffer->getIconHeight(strAbsIconPath.c_str());
			if ((logo_w == 0) || (logo_h == 0)) // corrupt logo size?
			{
				printf("[infoviewer] channel logo: \n -> %s (%s) has no size\n -> please check logo file!\n",strAbsIconPath.c_str(), ChannelName.c_str());
				return 0;
			}
							
			{	
				if (g_settings.infobar_show_channellogo == 1) // paint logo in numberbox
				{
					// calculate mid of numberbox
					int satNameHeight = g_settings.infobar_sat_display ? 8 : 0; // consider sat display and set an offset for y if exists
					x_mid = BoxStartX+ChanWidth/2;
					y_mid = (BoxStartY+satNameHeight)+ChanHeight/2;
						
					// check logo dimensions
					if ((logo_w > ChanWidth) || (logo_h > ChanHeight))	
					{
						printf(strErrText.c_str(),ChanWidth, ChanHeight, logo_w, logo_h, g_settings.infobar_show_channellogo);
						res = 0;
					}
					else
					{
						// channel name with number
						ChannelName = (std::string)strChanNum + ". " + ChannelName;
						// get position of channel logo, must be centered in number box
						logo_x = x_mid - logo_w/2;
						logo_y = y_mid - logo_h/2;
						res =  1;
					}
				}
				else if (g_settings.infobar_show_channellogo == 2) // paint logo in place of channel name
				{
					// check logo dimensions
					if ((logo_w > chan_w) || (logo_h > ChanHeight))
					{
						printf(strErrText.c_str(), chan_w, ChanHeight, logo_w, logo_h, g_settings.infobar_show_channellogo);
						res =  0;
					}
					else
					{
						// hide channel name
						ChannelName = "";
						// calculate logo position
						y_mid = (ChanNameY+time_height) - time_height/2;
						logo_x = start_x+10;
						logo_y = y_mid - logo_h/2;				
						res =  2;
					}
				}
				else if (g_settings.infobar_show_channellogo == 3) // paint logo beside channel name
				{
					// check logo dimensions
					int Logo_max_width = chan_w - logo_w - 10;
					if ((logo_w > Logo_max_width) || (logo_h > ChanHeight))
					{
						printf(strErrText.c_str(), Logo_max_width, ChanHeight, logo_w, logo_h, g_settings.infobar_show_channellogo);
						res =  0;
					}
					else
					{
						// calculate logo position
						y_mid = (ChanNameY+time_height) - time_height/2;
						logo_x = start_x+10;
						logo_y = y_mid - logo_h/2;
	
						// set channel name x pos
						ChanNameX =  start_x + logo_w + 10;
						res =  3;
					}
				}			
				else
				{
					res = 0;
				}
			
				// paint logo background (shaded/framed)
				if ((g_settings.infobar_channellogo_background !=0) && (res !=0)) // with background
				{	
					int frame_w = 2, logo_bg_x=0, logo_bg_y=0, logo_bg_w=0, logo_bg_h=0;
					
					if (g_settings.infobar_channellogo_background == 1) // framed
					{
						//sh_offset = 2;
						logo_bg_x = logo_x-frame_w;
						logo_bg_y = logo_y-frame_w;
						logo_bg_w = logo_w+frame_w*2;
						logo_bg_h = logo_h+frame_w*2;
					}
					else if (g_settings.infobar_channellogo_background == 2) // shaded
					{
						//sh_offset = 3;
						logo_bg_x = logo_x+SHADOW_OFFSET;
						logo_bg_y = logo_y+SHADOW_OFFSET;
						logo_bg_w = logo_w;
						logo_bg_h = logo_h;
					}
					frameBuffer->paintBoxRel(logo_bg_x, logo_bg_y, logo_bg_w, logo_bg_h, COL_INFOBAR_BUTTONS_BACKGROUND);
				}

				// paint the logo
				if (res !=0) {
					if (!frameBuffer->paintIcon(strAbsIconPath, logo_x, logo_y)) 
						return 0; // paint logo was failed
				}
			}
		}
	}

	return res;
}

void CInfoViewer::showLcdPercentOver()
{
	if (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME] != 1)
	{
		static long long old_interval = 0;
		int runningPercent=-1;
		time_t jetzt=time(NULL);
#if 0
No need to poll for EPG, we are getting events from sectionsd!
		if ( ! (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current) ||
		     jetzt > (int)(info_CurrentNext.current_zeit.startzeit + info_CurrentNext.current_zeit.dauer))
		{
			info_CurrentNext = getEPG(channel_id);
		}
#endif
		long long interval = 60000000; /* 60 seconds default update time */
		if ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_current)
		{
			if (jetzt < info_CurrentNext.current_zeit.startzeit)
				runningPercent = 0;
			else if (jetzt > (int)(info_CurrentNext.current_zeit.startzeit + info_CurrentNext.current_zeit.dauer))
				runningPercent = -2; /* overtime */
			else
			{
				runningPercent=MIN((jetzt-info_CurrentNext.current_zeit.startzeit) * 100 /
					            info_CurrentNext.current_zeit.dauer ,100);
				interval = info_CurrentNext.current_zeit.dauer * 1000LL * (1000/100); // update every percent
				if (is_visible && interval > 60000000)
					interval = 60000000;	// if infobar visible, update at least once per minute (radio mode)
				if (interval < 5000000)
					interval = 5000000;	// update only every 5 seconds
			}
		}
		if (interval != old_interval)
		{
			g_RCInput->killTimer(lcdUpdateTimer);
			lcdUpdateTimer = g_RCInput->addTimer(interval, false);
			//printf("lcdUpdateTimer: interval %lld old %lld\n",interval/1000000,old_interval/1000000);
			old_interval = interval;
		}
		CLCD::getInstance()->showPercentOver(runningPercent);
	}
}

void CInfoViewer::showEpgInfo()   //message on event change
{
	char nextStart[10];
	int mode = g_Zapit->getMode();
	struct tm *pnStartZeit = localtime(&info_CurrentNext.next_zeit.startzeit);
	sprintf((char*)&nextStart, "%02d:%02d", pnStartZeit->tm_hour, pnStartZeit->tm_min);

	/* show epg info only if we in TV- or Radio mode and current event is not the same like before */
	if ((eventname != info_CurrentNext.current_name) && (mode !=0))
	{
		eventname = info_CurrentNext.current_name;
		if (g_settings.infobar_show == 1)
		{
			if (eventname.length() !=0) // simple message
 			{
				std::string event = eventname + "\n" + g_Locale->getText(LOCALE_INFOVIEWER_MESSAGE_TO) + nextStart;
				std::string event_message =  ZapitTools::Latin1_to_UTF8(event.c_str());
				ShowHintUTF(LOCALE_INFOVIEWER_MESSAGE_NOW, event_message.c_str(), 420 , 6, "epginfo.raw"); 
			}
		}
		else if (g_settings.infobar_show == 2) // complex message, show infobar
		{
			g_RCInput->postMsg(NeutrinoMessages::SHOW_INFOBAR , 0);
		}
	}
}

//
//  -- InfoViewer Menu Handler Class
//  -- to be used for calls from Menue
//  -- (2004-03-06 rasc)
// 

int CInfoViewerHandler::exec(CMenuTarget* parent, const std::string &)
{
	int           res = menu_return::RETURN_EXIT_ALL;
	CChannelList  *channelList;
	CInfoViewer   *i;


	if (parent) {
		parent->hide();
	}

	i = new CInfoViewer;

	channelList = CNeutrinoApp::getInstance()->channelList;
	i->start();
	i->showTitle(channelList->getActiveChannelNumber(), channelList->getActiveChannelName(), channelList->getActiveSatellitePosition(), channelList->getActiveChannel_ChannelID()); // UTF-8
	delete i;

	return res;
}
