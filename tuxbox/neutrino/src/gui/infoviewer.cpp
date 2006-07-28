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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/infoviewer.h>

#include <gui/widget/icons.h>
#include <gui/widget/hintbox.h>

#include <daemonc/remotecontrol.h>
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

#include <global.h>
#include <neutrino.h>

#include <string>

#include <sys/timeb.h>
#include <time.h>
#include <sys/param.h>

#define COL_INFOBAR_BUTTONS            (COL_INFOBAR_SHADOW + 1)
#define COL_INFOBAR_BUTTONS_BACKGROUND (COL_INFOBAR_SHADOW_PLUS_1)

#define ICON_LARGE_WIDTH 26
#define ICON_SMALL_WIDTH 16
#ifndef SKIP_CA_STATUS
#define ICON_OFFSET (2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2)
#else
#define ICON_OFFSET (2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2)
#endif
#define BOTTOM_BAR_OFFSET 0
#define SHADOW_OFFSET 6
#define borderwidth 4

// in us
#define FADE_TIME 40000

#define LCD_UPDATE_TIME_TV_MODE (60 * 1000 * 1000)

#ifndef TUXTXT_CFG_STANDALONE
extern "C" void tuxtxt_start(int tpid);
extern "C" int  tuxtxt_stop();
#endif

int time_left_width;
int time_dot_width;
int time_width;
int time_height;
char old_timestr[10];

extern CZapitClient::SatelliteList satList;

CInfoViewer::CInfoViewer()
{
	frameBuffer      = CFrameBuffer::getInstance();

	BoxStartX        = BoxStartY = BoxEndX = BoxEndY = 0;
	recordModeActive = false;
	is_visible       = false;
	showButtonBar    = false;
	gotTime          = g_Sectionsd->getIsTimeSet();
#ifndef SKIP_CA_STATUS
	CA_Status        = false;
#endif
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
	    int ChanNameY = BoxStartY + (ChanHeight>>1)   + 5; //oberkante schatten?

		char timestr[10];
		struct timeb tm;

		ftime(&tm);
		strftime((char*) &timestr, 20, "%H:%M", localtime(&tm.time) );

		if ( ( !firstPaint ) && ( strcmp( timestr, old_timestr ) == 0 ) )
		{
			if ( show_dot )
				frameBuffer->paintBoxRel(BoxEndX- time_width+ time_left_width- 10, ChanNameY, time_dot_width, time_height/2+2, COL_INFOBAR_PLUS_0);
			else
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(BoxEndX- time_width+ time_left_width- 10, ChanNameY+ time_height, time_dot_width, ":", COL_INFOBAR);
			strcpy( old_timestr, timestr );
		}
		else
		{
			strcpy( old_timestr, timestr );

			if ( !firstPaint )
			{
				frameBuffer->paintBoxRel(BoxEndX- time_width- 10, ChanNameY, time_width+ 10, time_height, COL_INFOBAR_PLUS_0);
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
		int ChanNameX = BoxStartX + ChanWidth + 20;
		if(show)
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, ChanNameX, BoxStartY+10 );
		}
		else
		{
			frameBuffer->paintBoxRel(ChanNameX, BoxStartY+10, 20, 20, 255);
		}
	}
}

void CInfoViewer::showTitle(const int ChanNum, const std::string & Channel, const t_satellite_position satellitePosition, const t_channel_id new_channel_id, const bool calledFromNumZap)
{
	channel_id = new_channel_id;
	showButtonBar = !calledFromNumZap;
	std::string ChannelName = Channel;

	bool fadeIn = ((g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS) || (g_info.box_Type == CControld::TUXBOX_MAKER_SAGEM)) && // eNX only
		g_settings.widget_fade &&
		(!is_visible) &&
		showButtonBar;
	bool fadeOut = false;
	int fadeValue;

	is_visible = true;

	BoxStartX = g_settings.screen_StartX+ 20;
	BoxEndX   = g_settings.screen_EndX- 20;
	BoxEndY   = g_settings.screen_EndY- 20;

	int BoxEndInfoY = showButtonBar?(BoxEndY- InfoHeightY_Info):(BoxEndY);
	BoxStartY = BoxEndInfoY- InfoHeightY;

	if ( !gotTime )
		gotTime = g_Sectionsd->getIsTimeSet();

	if ( fadeIn )
	{
		fadeValue = 100;
		frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(fadeValue) );
		frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(fadeValue) );
		frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(fadeValue) );
		frameBuffer->paletteSet();
	}
	else
		fadeValue= g_settings.infobar_alpha;

	// kill linke seite
	frameBuffer->paintBackgroundBox(BoxStartX, BoxStartY+ ChanHeight, BoxStartX + (ChanWidth/3), BoxStartY+ ChanHeight+ InfoHeightY_Info+ 10);
	// kill progressbar
	frameBuffer->paintBackgroundBox(BoxEndX- 120, BoxStartY, BoxEndX, BoxStartY+ ChanHeight);

	//number box
	frameBuffer->paintBoxRel(BoxStartX+10, BoxStartY+10, ChanWidth, ChanHeight, COL_INFOBAR_SHADOW_PLUS_0);
	frameBuffer->paintBoxRel(BoxStartX,    BoxStartY,    ChanWidth, ChanHeight, COL_INFOBAR_PLUS_0);

	int ChanNumYPos = BoxStartY + ChanHeight;

	if (g_settings.infobar_sat_display)
	{
		for (CZapitClient::SatelliteList::const_iterator satList_it = satList.begin(); satList_it != satList.end(); satList_it++)
			if (satList_it->satPosition == satellitePosition)
			{
				int satNameWidth = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(satList_it->satName);
				if (satNameWidth > ChanWidth)
					satNameWidth = ChanWidth;

				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxStartX + ((ChanWidth - satNameWidth)>>1), BoxStartY + 22, ChanWidth, satList_it->satName, COL_INFOBAR);

				ChanNumYPos += 10;

				break;
			}
	}

	char strChanNum[10];
	sprintf( (char*) strChanNum, "%d", ChanNum);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->RenderString(BoxStartX + ((ChanWidth - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getRenderWidth(strChanNum))>>1), ChanNumYPos, ChanWidth, strChanNum, COL_INFOBAR);

	//infobox
	int ChanNameX = BoxStartX + ChanWidth + 10;
	int ChanNameY = BoxStartY + (ChanHeight>>1)   + 5; //oberkante schatten?

	frameBuffer->paintBox(ChanNameX, ChanNameY, BoxEndX, BoxEndInfoY, COL_INFOBAR_PLUS_0);

	paintTime( false, true );

	// ... with channel name
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(ChanNameX+ 10, ChanNameY+ time_height, BoxEndX- (ChanNameX+ 20)- time_width- 15, ChannelName, COL_INFOBAR, 0, true); // UTF-8

	ChanInfoX = BoxStartX + (ChanWidth / 3);
	int ChanInfoY = BoxStartY + ChanHeight+ 10;
	ButtonWidth = (BoxEndX- ChanInfoX- ICON_OFFSET)>> 2;

	frameBuffer->paintBox(ChanInfoX, ChanInfoY, ChanNameX, BoxEndInfoY, COL_INFOBAR_PLUS_0);

	if ( showButtonBar )
	{
		sec_timer_id = g_RCInput->addTimer(1000000, false);

		if ( BOTTOM_BAR_OFFSET> 0 )
			frameBuffer->paintBackgroundBox(ChanInfoX, BoxEndInfoY, BoxEndX, BoxEndInfoY+ BOTTOM_BAR_OFFSET);

		frameBuffer->paintBox(ChanInfoX, BoxEndInfoY+ BOTTOM_BAR_OFFSET, BoxEndX, BoxEndY, COL_INFOBAR_BUTTONS_BACKGROUND);

		// blau
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE, BoxEndX- ICON_OFFSET- ButtonWidth + 2, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2), BoxEndY - 2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2 + 2), g_Locale->getText(LOCALE_INFOVIEWER_STREAMINFO), COL_INFOBAR_BUTTONS, 0, true); // UTF-8

		showButton_Audio();
		showButton_SubServices();
#ifndef SKIP_CA_STATUS
		showIcon_CA_Status();
#endif
		showIcon_16_9();
		showIcon_VTXT();
	}

	info_CurrentNext = getEPG(channel_id);

	if ( !( info_CurrentNext.flags & ( CSectionsdClient::epgflags::has_later | CSectionsdClient::epgflags::has_current |  CSectionsdClient::epgflags::not_broadcast ) ) )
	{
		// nicht gefunden / noch nicht geladen
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(ChanNameX+ 10, ChanInfoY+ 2* g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight()+ 5, BoxEndX- (ChanNameX+ 20), g_Locale->getText(gotTime?(showButtonBar ? LOCALE_INFOVIEWER_EPGWAIT : LOCALE_INFOVIEWER_EPGNOTLOAD) : LOCALE_INFOVIEWER_WAITTIME), COL_INFOBAR, 0, true); // UTF-8
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

	// Schatten
	frameBuffer->paintBox(BoxEndX, ChanNameY+ SHADOW_OFFSET, BoxEndX+ SHADOW_OFFSET, BoxEndY, COL_INFOBAR_SHADOW_PLUS_0);
	frameBuffer->paintBox(ChanInfoX+ SHADOW_OFFSET, BoxEndY, BoxEndX+ SHADOW_OFFSET, BoxEndY+ SHADOW_OFFSET, COL_INFOBAR_SHADOW_PLUS_0);

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	CNeutrinoApp *neutrino = CNeutrinoApp::getInstance();

	if ( !calledFromNumZap )
	{
		bool show_dot= true;
		if ( fadeIn )
			fadeTimer = g_RCInput->addTimer( FADE_TIME, false );

		bool hideIt = true;
		bool virtual_zap_mode = false;
		unsigned long long timeoutEnd = (neutrino->getMode() != 2) ?  CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR]) : CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR_RADIO]);

		int res = messages_return::none;
		time_t ta=0,tb=0;

		while ( ! ( res & ( messages_return::cancel_info | messages_return::cancel_all ) ) )
		{
			g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
			//printf(" g_RCInput->getMsgAbsoluteTimeout %x %x\n", msg, data);

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
						fadeIn = false;
						frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(0) );
					}
					else
						frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(fadeValue) );
				}

				frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(fadeValue) );
				frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(fadeValue) );
				frameBuffer->paletteSet();
			}
			else if ( ( msg == CRCInput::RC_ok ) ||
					( msg == CRCInput::RC_home ) ||
						( msg == CRCInput::RC_timeout ) )
			{
				if ( fadeIn )
				{
					g_RCInput->killTimer(fadeTimer);
					fadeIn = false;
				}
				if (((g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS) || (g_info.box_Type == CControld::TUXBOX_MAKER_SAGEM)) && // eNX only
					(!fadeOut) && g_settings.widget_fade )
				{
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
			else if ( ( msg == (neutrino_msg_t)g_settings.key_quickzap_up  ) ||
				    ( msg == (neutrino_msg_t)g_settings.key_quickzap_down) ||
				    ( msg == CRCInput::RC_0 ) ||
				    ( msg == NeutrinoMessages::SHOW_INFOBAR ) )
			{
				hideIt = false;
				g_RCInput->postMsg( msg, data );
				res = messages_return::cancel_info;
			}
			else if ( msg == NeutrinoMessages::EVT_TIMESET )
			{
				// Handle anyway!
				neutrino->handleMsg(msg, data);
				g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );
				hideIt = false;
				res = messages_return::cancel_all;
			}
			else if ( ( msg == NeutrinoMessages::EVT_TIMER ) && ( data == sec_timer_id ) )
			{
				paintTime( show_dot, false );
				showRecordIcon(show_dot);
				show_dot = !show_dot;
			}
			else if ( g_settings.virtual_zap_mode && ((msg == CRCInput::RC_right) || msg == CRCInput::RC_left ))
			{
				virtual_zap_mode = true;
				res = messages_return::cancel_all;
				hideIt = true;
			}
			else
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
	CFrameBuffer *frameBuffer = CFrameBuffer::getInstance();
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

		frameBuffer->paintBoxRel(x, y, dx, dy, COL_MENUCONTENT_PLUS_0);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(x+10, y+ 30, dx-20, text, COL_MENUCONTENT, 0, subChannelNameIsUTF); // UTF-8

		if ( g_RemoteControl->director_mode )
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, x+ 8, y+ dy- 20 );
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+ 30, y+ dy- 2, dx- 40, g_Locale->getText(LOCALE_NVODSELECTOR_DIRECTORMODE), COL_MENUCONTENT, 0, true); // UTF-8
		}

		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( 2 );
		int res = messages_return::none;

		neutrino_msg_t      msg;
		neutrino_msg_data_t data;

		while ( ! ( res & ( messages_return::cancel_info | messages_return::cancel_all ) ) )
		{
			g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

			if ( msg == CRCInput::RC_timeout )
			{
				res = messages_return::cancel_info;
			}
			else
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

		frameBuffer->RestoreScreen(x- borderwidth, y- borderwidth, dx+ 2* borderwidth, dy+ 2* borderwidth, pixbuf);

	}
	else
	{
		g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );
	}
}


void CInfoViewer::showIcon_16_9() const
{
	frameBuffer->paintIcon((aspectRatio != 0) ? "16_9.raw" : "16_9_gray.raw", BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2), BoxEndY - ((InfoHeightY_Info + 16) >> 1));
}

void CInfoViewer::showIcon_VTXT() const
{
	int vtpid=g_RemoteControl->current_PIDs.PIDs.vtxtpid;
	frameBuffer->paintIcon((vtpid != 0) ? "vtxt.raw" : "vtxt_gray.raw", BoxEndX - (ICON_SMALL_WIDTH + 2), BoxEndY - ((InfoHeightY_Info + 16) >> 1));
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

int CInfoViewer::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	if ((msg == NeutrinoMessages::EVT_CURRENTNEXT_EPG) ||
	    (msg == NeutrinoMessages::EVT_NEXTPROGRAM    ))
	{
		if ((*(t_channel_id *)data) == channel_id)
		{
			CSectionsdClient::CurrentNextInfo info = getEPG( *(t_channel_id *)data );
			info_CurrentNext = info;
			if ( is_visible )
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
			showLcdPercentOver();
			if ( is_visible )
				show_Data( true );
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
		}
	    return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_GOTPIDS )
	{
		if ((*(t_channel_id *)data) == channel_id)
		{
			if ( is_visible && showButtonBar )
				showIcon_VTXT();
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
	else if (msg == NeutrinoMessages::EVT_ZAP_SUB_COMPLETE)
	{
		//if ((*(t_channel_id *)data) == channel_id)
		{
			if ( is_visible && showButtonBar &&  ( !g_RemoteControl->are_subchannels ) )
				show_Data( true );
		}
		showLcdPercentOver();
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
#ifndef SKIP_CA_STATUS
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
#endif
	
   return messages_return::unhandled;
}


void CInfoViewer::showButton_SubServices()
{
	if (!(g_RemoteControl->subChannels.empty()))
	{
		// yellow button for subservices / NVODs
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, BoxEndX- ICON_OFFSET- 2* ButtonWidth + 2, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );

		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- 2* ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2), BoxEndY - 2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2 + 2), g_Locale->getText((g_RemoteControl->are_subchannels) ? LOCALE_INFOVIEWER_SUBSERVICE : LOCALE_INFOVIEWER_SELECTTIME), COL_INFOBAR_BUTTONS, 0, true); // UTF-8
	}
}

CSectionsdClient::CurrentNextInfo CInfoViewer::getEPG(const t_channel_id for_channel_id)
{
	CSectionsdClient::CurrentNextInfo info;

	g_Sectionsd->getCurrentNextServiceKey(for_channel_id, info );

	if ( info.flags & ( CSectionsdClient::epgflags::has_current | CSectionsdClient::epgflags::has_next ) )
	{
		CSectionsdClient::CurrentNextInfo*	_info = new CSectionsdClient::CurrentNextInfo;
		*_info = info;
		g_RCInput->postMsg( ( info.flags & ( CSectionsdClient::epgflags::has_current ) )? NeutrinoMessages::EVT_CURRENTEPG : NeutrinoMessages::EVT_NEXTEPG, (unsigned) _info, false );
	}
	else
	{
		t_channel_id * p = new t_channel_id;
		*p = for_channel_id;
		g_RCInput->postMsg(NeutrinoMessages::EVT_NOEPG_YET, (const neutrino_msg_data_t)p, false); // data is pointer to allocated memory
	}

	return info;
}


void CInfoViewer::show_Data( bool calledFromEvent)
{
	char runningStart[10];
	char runningRest[20];
	char runningPercent = 0;

	char nextStart[10];
	char nextDuration[10];

	int is_nvod= false;

	if ( is_visible )
	{

       	if ( ( g_RemoteControl->current_channel_id == channel_id) &&
       		 ( g_RemoteControl->subChannels.size()> 0 ) && ( !g_RemoteControl->are_subchannels ) )
       	{
			is_nvod = true;
 			info_CurrentNext.current_zeit.startzeit = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].startzeit;
 			info_CurrentNext.current_zeit.dauer = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].dauer;
	}
	else
	{
		if ( ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_current) &&
		     ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_next) &&
		     ( showButtonBar ) )
		{
			if ( (uint) info_CurrentNext.next_zeit.startzeit < ( info_CurrentNext.current_zeit.startzeit+ info_CurrentNext.current_zeit.dauer ) )
			{
				is_nvod = true;
			}
		}
	}

	time_t jetzt=time(NULL);

	if ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_current)
	{
		int seit = (jetzt - info_CurrentNext.current_zeit.startzeit + 30) / 60;
		int rest = ( info_CurrentNext.current_zeit.dauer / 60) - seit ;
		if ( seit< 0 )
		{
			runningPercent= 0;
			sprintf( (char*)&runningRest, "in %d min", -seit);
		}
		else
		{
			runningPercent=(unsigned)((float)(jetzt-info_CurrentNext.current_zeit.startzeit)/(float)info_CurrentNext.current_zeit.dauer*100.);
			sprintf( (char*)&runningRest, "%d / %d min", seit, rest);
		}

		struct tm *pStartZeit = localtime(&info_CurrentNext.current_zeit.startzeit);
		sprintf( (char*)&runningStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min );


	}

	if ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_next)
	{
		unsigned dauer = info_CurrentNext.next_zeit.dauer/ 60;
		sprintf( (char*)&nextDuration, "%d min", dauer);
		struct tm *pStartZeit = localtime(&info_CurrentNext.next_zeit.startzeit);
		sprintf( (char*)&nextStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
	}

	int height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getHeight()/3;
	int ChanInfoY = BoxStartY + ChanHeight+ 15; //+10


	if ( showButtonBar )
	{
		int posy = BoxStartY+12;
		int height2= 20;
		//percent
		if ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_current)
		{
			frameBuffer->paintBoxRel(BoxEndX-114, posy,   2+100+2, height2, COL_INFOBAR_SHADOW_PLUS_0); //border
			frameBuffer->paintBoxRel(BoxEndX-112, posy+2, runningPercent+2, height2-4, COL_INFOBAR_PLUS_7);//fill(active)
			frameBuffer->paintBoxRel(BoxEndX-112+runningPercent, posy+2, 100-runningPercent, height2-4, COL_INFOBAR_PLUS_3);//fill passive
		}
		else
			frameBuffer->paintBackgroundBoxRel(BoxEndX-114, posy,   2+100+2, height2);

			if ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_anything )
			{
				frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, BoxEndX- ICON_OFFSET- 4* ButtonWidth + 2, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- 4* ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2), BoxEndY - 2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + 2) + 8, g_Locale->getText(LOCALE_INFOVIEWER_EVENTLIST), COL_INFOBAR_BUTTONS, 0, true); // UTF-8
			}
		}

		height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight();
		int xStart= BoxStartX + ChanWidth;

		frameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR_PLUS_0);

		if ( ( info_CurrentNext.flags & CSectionsdClient::epgflags::not_broadcast ) ||
			 ( ( calledFromEvent ) && !( info_CurrentNext.flags & ( CSectionsdClient::epgflags::has_next | CSectionsdClient::epgflags::has_current ) ) ) )
		{
			// no EPG available
			ChanInfoY += height;
			frameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height, COL_INFOBAR_PLUS_0);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(BoxStartX + ChanWidth + 20,  ChanInfoY+height, BoxEndX- (BoxStartX + ChanWidth + 20), g_Locale->getText(gotTime ? LOCALE_INFOVIEWER_NOEPG : LOCALE_INFOVIEWER_WAITTIME), COL_INFOBAR, 0, true); // UTF-8
		}
		else
		{
			// irgendein EPG gefunden
			int duration1Width   = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(runningRest);
			int duration1TextPos = BoxEndX- duration1Width- 10;

			int duration2Width   = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(nextDuration);
			int duration2TextPos = BoxEndX- duration2Width- 10;

			if ( ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_next ) && ( !( info_CurrentNext.flags & CSectionsdClient::epgflags::has_current )) )
			{
				// there are later events available - yet no current
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(xStart,  ChanInfoY+height, BoxEndX- xStart, g_Locale->getText(LOCALE_INFOVIEWER_NOCURRENT), COL_INFOBAR, 0, true); // UTF-8

				ChanInfoY += height;

				//info next
				frameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR_PLUS_0);

				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR);
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(xStart,  ChanInfoY+height, duration2TextPos- xStart- 5, info_CurrentNext.next_name, COL_INFOBAR);
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR);
			}
			else
			{
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, runningStart, COL_INFOBAR);
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(xStart,  ChanInfoY+height, duration1TextPos- xStart- 5, info_CurrentNext.current_name, COL_INFOBAR);
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(duration1TextPos,            ChanInfoY+height, duration1Width, runningRest, COL_INFOBAR);

				ChanInfoY += height;

				//info next
				frameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR_PLUS_0);

				if ( ( !is_nvod ) && ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_next ) )
				{
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR);
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(xStart,  ChanInfoY+height, duration2TextPos- xStart- 5, info_CurrentNext.next_name, COL_INFOBAR);
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR);
				}
			}
		}
	}
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
	if ( g_settings.audio_left_right_selectable || count > 1 )
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, BoxEndX- ICON_OFFSET- 3* ButtonWidth + 2 + 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- 3* ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2) + 8, BoxEndY - 2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2 + 2), g_Locale->getText(LOCALE_INFOVIEWER_LANGUAGES), COL_INFOBAR_BUTTONS, 0, true); // UTF-8
	};

	const char * dd_icon;
	if ( ( g_RemoteControl->current_PIDs.PIDs.selected_apid < count ) &&
	     ( g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].is_ac3 ) )
		dd_icon = "dd.raw";
	else if ( g_RemoteControl->has_ac3 )
		dd_icon = "dd_avail.raw";
	else
		dd_icon = "dd_gray.raw";

	frameBuffer->paintIcon(dd_icon, BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2), BoxEndY - ((InfoHeightY_Info + 16) >> 1));
}

void CInfoViewer::killTitle()
{
	if (is_visible )
	{
		is_visible = false;
		frameBuffer->paintBackgroundBox(BoxStartX, BoxStartY, BoxEndX+ SHADOW_OFFSET, BoxEndY+ SHADOW_OFFSET );
	}
}

#ifndef SKIP_CA_STATUS
void CInfoViewer::showIcon_CA_Status() const
{
	frameBuffer->paintIcon( ( CA_Status)?"ca.raw":"fta.raw", BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2), BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
}

void CInfoViewer::Set_CA_Status(int Status)
{
	CA_Status = Status;
	if ( is_visible && showButtonBar )
		showIcon_CA_Status();
}
#endif

void CInfoViewer::showLcdPercentOver()
{
	if (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME] != 1)
	{
		int runningPercent=-1;
		time_t jetzt=time(NULL);
		if ( ! (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current) ||
		     jetzt > (int)(info_CurrentNext.current_zeit.startzeit + info_CurrentNext.current_zeit.dauer))
		{
			info_CurrentNext = getEPG(channel_id);
		}
		if ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_current)
		{
			if (jetzt < info_CurrentNext.current_zeit.startzeit)
				runningPercent = 0;
			else
				runningPercent=MIN((unsigned)((float)(jetzt-info_CurrentNext.current_zeit.startzeit)/
														(float)info_CurrentNext.current_zeit.dauer*100.),100);
		}
		CLCD::getInstance()->showPercentOver(runningPercent);
	}
}





//
//  -- InfoViewer Menu Handler Class
//  -- to be used for calls from Menue
//  -- (2004-03-06 rasc)
// 

int CInfoViewerHandler::exec(CMenuTarget* parent, const std::string &actionkey)
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
