/*
	$Id: movieviewer.cpp,v 1.2 2007/09/07 00:43:09 guenther Exp $

	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Copyright (C) 2007 Guenther
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
	
	Purpose: Show info bar for movie playback. Displays further informations from *.xml if available.
		Adapted from infoviewer.h/.cpp
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/movieviewer.h>

#include <gui/widget/icons.h>
#include <gui/widget/hintbox.h>

#include <daemonc/remotecontrol.h>
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

#include <global.h>
#include <neutrino.h>

#include <string>
#include <system/settings.h>

#include <sys/timeb.h>
#include <time.h>
#include <sys/param.h>

#define COL_INFOBAR_BUTTONS            (COL_INFOBAR_SHADOW + 1)
#define COL_INFOBAR_BUTTONS_BACKGROUND (COL_INFOBAR_SHADOW_PLUS_1)

#define ICON_LARGE_WIDTH 26
#define ICON_SMALL_WIDTH 16

#define ICON_OFFSET (2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2)

#define BOTTOM_BAR_OFFSET 0
#define SHADOW_OFFSET 6

// in us
#define FADE_TIME 40000

/************************************************************************
 
************************************************************************/
CMovieViewer::CMovieViewer()
{
	frameBuffer		= CFrameBuffer::getInstance();

	BoxStartX		= BoxStartY = BoxEndX = BoxEndY = 0;
	is_visible		= false;
	isButtonBar		= true;
	isButtonBarExtended	= false;
	init();
}

/************************************************************************
 
************************************************************************/
CMovieViewer::~CMovieViewer()
{
}

/************************************************************************
 
************************************************************************/
void CMovieViewer::init()
{
	InfoHeightY = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getHeight()*9/8 +
		2*g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight() +
		25;
	InfoHeightY_Info = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight()+ 5;

	ChanWidth = 4* g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getRenderWidth(widest_number) + 10;
	ChanHeight = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getHeight()*9/8;

	time_height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getHeight()+5;
	time_left_width = 2* g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getRenderWidth(widest_number);
	time_dot_width = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getRenderWidth(":");
	time_width = time_left_width* 2+ time_dot_width;

	getData();
}


/************************************************************************
 
************************************************************************/
void CMovieViewer::setData(uint _aspectRatio, CMoviePlayerGui::state _player_mode, bool _is_ac3, bool _has_ac3, uint _apid_count, uint _runningPercent, const char* _current_name, const char* _next_name)
{
	aspectRatio = _aspectRatio;
	player_mode = _player_mode;
	is_ac3 = _is_ac3;
	has_ac3 = _has_ac3;
	apid_count = _apid_count;
	runningPercent = _runningPercent;
	if(_current_name != NULL)
	{
		strncpy(current_name,_current_name,sizeof(current_name));
		current_name[sizeof(current_name)-1] = 0;
		
		CMovieInfo mi;
		MI_MOVIE_INFO info;
		info.file.Name = current_name;
		if(mi.loadMovieInfo(&info) == true)
		{
			currentTitle = info.epgTitle;
			currentInfo1 = info.epgInfo1;
		}
		else
		{
			currentTitle = "";
			currentInfo1 = "";
		}
	}
	else
		current_name[0] = 0;
		
	if(_next_name != NULL)
	{
		strncpy(next_name, _next_name,sizeof(next_name));
		next_name[sizeof(next_name)-1] = 0;
	}
	else
		next_name[0] = 0;

}
/************************************************************************
 
************************************************************************/
void CMovieViewer::getData()
{
	//TODO get real values here?
	aspectRatio = 0;
	player_mode = CMoviePlayerGui::PLAY;
	is_ac3 = 0;
	has_ac3 = 0;
	apid_count = 0;
	runningPercent = 0;
	current_name[0] = 0;
	next_name[0] = 0;
}

/************************************************************************
 
************************************************************************/
void CMovieViewer::exec()
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	CMovieInfo cMovieInfo;

	bool fadeOut = false;
	bool fadeIn = ((g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS) || (g_info.box_Type == CControld::TUXBOX_MAKER_SAGEM)) && // eNX only
		g_settings.widget_fade && (!is_visible) && isButtonBar;
	int fadeValue;
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

	show();
	
	bool show_dot= true;
	bool hideIt = true;
	if ( fadeIn )
		fadeTimer = g_RCInput->addTimer( FADE_TIME, false );

	sec_timer_id = g_RCInput->addTimer(1000000, false);

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR_MOVIE]);

	int res = messages_return::none;

	while ( ! ( res & ( messages_return::cancel_info | messages_return::cancel_all ) ) )
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg == CRCInput::RC_help )
		{
			cMovieInfo.showMovieInfo(current_name);
			msg = CRCInput::RC_home;
			g_RCInput->postMsg( msg, data );
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
		else if (( msg == CRCInput::RC_ok ) ||
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
				res = messages_return::cancel_info;
			}
		}
		else if ( ( msg == NeutrinoMessages::EVT_TIMER ) && ( data == sec_timer_id ) )
		{
			paintTime( show_dot, false );
			show_dot = !show_dot;
		}
		else
		{
			// raus hier und im Hauptfenster behandeln...
			g_RCInput->postMsg(  msg, data );
			res = messages_return::cancel_info;
		}
	}

	if ( hideIt )
		kill();

	g_RCInput->killTimer(sec_timer_id);
	if ( fadeIn || fadeOut )
	{
		g_RCInput->killTimer(fadeTimer);
		frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
		frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
		frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(0) );
		frameBuffer->paletteSet();
	}
}

/************************************************************************
 
************************************************************************/
int CMovieViewer::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{

	if ( msg == NeutrinoMessages::EVT_TIMER )
	{
		// TODO return messages_return::handled;
	}

	return messages_return::unhandled;
}

/************************************************************************
 
************************************************************************/
void CMovieViewer::kill()
{
	if (is_visible )
	{
		is_visible = false;
		frameBuffer->paintBackgroundBox(BoxStartX, BoxStartY, BoxEndX+ SHADOW_OFFSET, BoxEndY+ SHADOW_OFFSET );
	}
}


/************************************************************************
 
************************************************************************/
void CMovieViewer::show()
{
	is_visible = true;

	BoxStartX = g_settings.screen_StartX+ 20;
	BoxEndX   = g_settings.screen_EndX- 20;
	BoxEndY   = g_settings.screen_EndY- 20;

	BoxEndInfoY = BoxEndY;
	if(isButtonBar)
		BoxEndInfoY -= InfoHeightY_Info;
	if(isButtonBarExtended)
		BoxEndInfoY -= 2*InfoHeightY_Info;
	
	BoxStartY = BoxEndInfoY- InfoHeightY;

	if ( !gotTime )
		gotTime = g_Sectionsd->getIsTimeSet();

	// kill linke seite
	frameBuffer->paintBackgroundBox(BoxStartX, BoxStartY+ ChanHeight, BoxStartX + (ChanWidth/3), BoxStartY+ ChanHeight+ InfoHeightY_Info+ 10);
	// kill progressbar
	frameBuffer->paintBackgroundBox(BoxEndX- 120, BoxStartY, BoxEndX, BoxStartY+ ChanHeight);

	int col_NumBox = COL_INFOBAR_PLUS_0;

	///////////////////////////
	// paint number box 
	frameBuffer->paintBoxRel(BoxStartX+10, BoxStartY+10, ChanWidth, ChanHeight, COL_INFOBAR_SHADOW_PLUS_0);
	frameBuffer->paintBoxRel(BoxStartX,    BoxStartY,    ChanWidth, ChanHeight, col_NumBox);

	int ChanNumYPos = BoxStartY + ChanHeight;

	//paint play state icon
	bool res;
	const char * dd_icon;
	if (  player_mode == CMoviePlayerGui::PAUSE )
		dd_icon = "pause.raw";
	else 
		dd_icon = "play.raw";
	res = frameBuffer->paintIcon(dd_icon, BoxStartX , BoxStartY );

	///////////////////////////
	//paint infobox
	int ChanNameX = BoxStartX + ChanWidth + 10;
	int ChanNameY = BoxStartY + (ChanHeight>>1)   + 5; //oberkante schatten?

	frameBuffer->paintBox(ChanNameX, ChanNameY, BoxEndX, BoxEndInfoY, COL_INFOBAR_PLUS_0);

	paintTime( false, true );

	///////////////////////////
	// paint channel name
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(ChanNameX+ 10, ChanNameY+ time_height, BoxEndX- (ChanNameX+ 20)- time_width- 15, g_Locale->getText(LOCALE_MOVIEPLAYER_HEAD), COL_INFOBAR, 0, true); // UTF-8

	ChanInfoX = BoxStartX + (ChanWidth / 3);
	int ChanInfoY = BoxStartY + ChanHeight+ 10;
	ButtonWidth = (BoxEndX- ChanInfoX- ICON_OFFSET)>> 2;

	frameBuffer->paintBox(ChanInfoX, ChanInfoY, ChanNameX, BoxEndInfoY, COL_INFOBAR_PLUS_0);

	///////////////////////////
	//paint button bar
	if ( isButtonBar )
		showButtonBar();

	if ( isButtonBarExtended )
		showButtonBarExtended();

	///////////////////////////
	//paint other stuff
	showData();
	showIcon_Audio();
	showIcon_16_9();
	showIcon_VTXT();
	showLcdPercentOver();
	// Schatten
	frameBuffer->paintBox(BoxEndX, ChanNameY+ SHADOW_OFFSET, BoxEndX+ SHADOW_OFFSET, BoxEndY, COL_INFOBAR_SHADOW_PLUS_0);
	frameBuffer->paintBox(ChanInfoX+ SHADOW_OFFSET, BoxEndY, BoxEndX+ SHADOW_OFFSET, BoxEndY+ SHADOW_OFFSET, COL_INFOBAR_SHADOW_PLUS_0);
}


/************************************************************************
 
************************************************************************/
void CMovieViewer::showData( )
{
	char runningStart[10]= {0};
	char runningRest[20]= {0};

	char nextStart[10] = {0};
	char nextDuration[10] = "";

	const char* txt;

	if ( is_visible )
	{
		// runningPercent bar
		int posy = BoxStartY+12;
		int height2= 20;

		frameBuffer->paintBoxRel(BoxEndX-114, posy,   2+100+2, height2, COL_INFOBAR_SHADOW_PLUS_0); //border
		frameBuffer->paintBoxRel(BoxEndX-112, posy+2, runningPercent+2, height2-4, COL_INFOBAR_PLUS_7);//fill(active)
		frameBuffer->paintBoxRel(BoxEndX-112+runningPercent, posy+2, 100-runningPercent, height2-4, COL_INFOBAR_PLUS_3);//fill passive

		int height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getHeight()/3;
		int ChanInfoY = BoxStartY + ChanHeight+ 15; //+10

		height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight();
		int xStart= BoxStartX + ChanWidth;
		frameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR_PLUS_0);

		int duration1Width   = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(runningRest);
		int duration1TextPos = BoxEndX- duration1Width- 10;
		int duration2Width   = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(nextDuration);
		int duration2TextPos = BoxEndX- duration2Width- 10;

		if(currentTitle.empty())
			txt = current_name;
		else
			txt = currentTitle.c_str();
				
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, runningStart, COL_INFOBAR,0,true);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(xStart,  ChanInfoY+height, duration1TextPos- xStart- 5, txt, COL_INFOBAR,0,true);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(duration1TextPos,            ChanInfoY+height, duration1Width, runningRest, COL_INFOBAR,0,true);

		ChanInfoY += height;

		//info next
		frameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR_PLUS_0);

		// print Info1 if there is no other file to be played
		if (  next_name[0] == 0  )
		{
			txt = currentInfo1.c_str();
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR,0,true);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(xStart,  ChanInfoY+height, duration2TextPos- xStart- 5, txt, COL_INFOBAR,0,true);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR,0,true);
		}
		else
		{
			txt = next_name;
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR,0,true);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(xStart,  ChanInfoY+height, duration2TextPos- xStart- 5, txt, COL_INFOBAR,0,true);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR,0,true);
		}
	}
}

/************************************************************************
 
************************************************************************/
void CMovieViewer::showLcdPercentOver()
{
	if (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME] != 1)
	{
		CLCD::getInstance()->showPercentOver(runningPercent);
	}
}

/************************************************************************
 
************************************************************************/
void CMovieViewer::paintTime( bool show_dot, bool firstPaint )
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

/************************************************************************
 
************************************************************************/
void CMovieViewer::showRecordIcon(const bool show)
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

/************************************************************************
 
************************************************************************/
void CMovieViewer::showIcon_Audio()
{
	const char * dd_icon;
	if (  is_ac3 )
		dd_icon = "dd.raw";
	else if ( has_ac3 )
		dd_icon = "dd_avail.raw";
	else
		dd_icon = "dd_gray.raw";

	frameBuffer->paintIcon(dd_icon, BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2), BoxEndInfoY + InfoHeightY_Info - ((InfoHeightY_Info + 16) >> 1));
}

/************************************************************************
 
************************************************************************/
void CMovieViewer::showIcon_16_9() const
{
	frameBuffer->paintIcon((aspectRatio != 0) ? "16_9.raw" : "16_9_gray.raw", BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2), BoxEndInfoY + InfoHeightY_Info - ((InfoHeightY_Info + 16) >> 1));
}

/************************************************************************
 
************************************************************************/
void CMovieViewer::showIcon_VTXT() const
{
	int vtpid=g_RemoteControl->current_PIDs.PIDs.vtxtpid;
	frameBuffer->paintIcon((vtpid != 0) ? "vtxt.raw" : "vtxt_gray.raw", BoxEndX - (ICON_SMALL_WIDTH + 2), BoxEndInfoY + InfoHeightY_Info - ((InfoHeightY_Info + 16) >> 1));
}


/************************************************************************
 
************************************************************************/
void CMovieViewer::showButtonBar()
{
	if ( BOTTOM_BAR_OFFSET> 0 )
		frameBuffer->paintBackgroundBox(ChanInfoX, BoxEndInfoY, BoxEndX, BoxEndInfoY+ BOTTOM_BAR_OFFSET);

	frameBuffer->paintBox(ChanInfoX, BoxEndInfoY+ BOTTOM_BAR_OFFSET, BoxEndX, BoxEndInfoY + InfoHeightY_Info, COL_INFOBAR_BUTTONS_BACKGROUND);

	const char* txt= NULL;
	//////////////////
	// show red button
	txt = g_Locale->getText(LOCALE_TIMERLIST_PLUGIN);
	if ( txt != NULL )
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, BoxEndX- ICON_OFFSET- 4* ButtonWidth + 2, BoxEndInfoY + InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) );
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- 4* ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2), BoxEndInfoY + InfoHeightY_Info - 2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + 2) + 8, txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
	}
	//////////////////
	// show green button
	if( apid_count > 1 )	
		txt = g_Locale->getText(LOCALE_INFOVIEWER_LANGUAGES);
	else
		txt = NULL;
	if ( txt != NULL )
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, BoxEndX- ICON_OFFSET- 3* ButtonWidth + 2 + 8, BoxEndInfoY + InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) );
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- 3* ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2) + 8, BoxEndInfoY + InfoHeightY_Info - 2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2 + 2), txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
	};
	//////////////////
	// show yellow button
	if( player_mode == CMoviePlayerGui::PAUSE )
		txt = "Play";
	else if( player_mode == CMoviePlayerGui::PLAY )	
		txt = "Pause";
	if ( txt != NULL )
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, BoxEndX- ICON_OFFSET- 2* ButtonWidth + 2, BoxEndInfoY + InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) );
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- 2* ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2), BoxEndInfoY + InfoHeightY_Info - 2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2 + 2), txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
	}
	//////////////////
	// show blue button
	txt = g_Locale->getText(LOCALE_MOVIEBROWSER_BOOK_HEAD);
	if ( txt != NULL )
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE, BoxEndX- ICON_OFFSET- ButtonWidth + 2, BoxEndInfoY + InfoHeightY_Info - ((InfoHeightY_Info+ 16)>>1) );
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2), BoxEndInfoY + InfoHeightY_Info - 2, ButtonWidth - (NEUTRINO_ICON_BUTTON_BLUE_WIDTH-10), txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
	}

}

/************************************************************************
 
************************************************************************/
void CMovieViewer::showButtonBarExtended()
{
	frameBuffer->paintBox(ChanInfoX, BoxEndInfoY + BOTTOM_BAR_OFFSET + 1*InfoHeightY_Info, BoxEndX, BoxEndInfoY + 3*InfoHeightY_Info, COL_INFOBAR_BUTTONS_BACKGROUND);

	const char* txt= "";
	// Bar 2
	//"(OK) Playlist   (home) Stop     (dbox) Time     (?) Movie info";
	if(1)
	{
		txt = g_Locale->getText(LOCALE_MOVIEBROWSER_HEAD);
		if ( txt != NULL )
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, BoxEndX- ICON_OFFSET- ButtonWidth + 2- 7, BoxEndInfoY + 2*InfoHeightY_Info - ((InfoHeightY_Info+ 16)>>1) - 2*3);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2), BoxEndInfoY + 2*InfoHeightY_Info - 2*2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_BLUE_WIDTH + 2 + 2), txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
		}
		txt = g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP1);
		if ( txt != NULL )
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HOME, BoxEndX- ICON_OFFSET- 4* ButtonWidth + 2- 5, BoxEndInfoY + 2*InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) - 2*3);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- 4* ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2), BoxEndInfoY + 2*InfoHeightY_Info - 2*2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + 2) + 8, txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
		}
		txt = g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP5);
		if ( txt != NULL )
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_DBOX, BoxEndX- ICON_OFFSET- 2* ButtonWidth + 2- 7, BoxEndInfoY + 2*InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) - 2*3);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- 2* ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2), BoxEndInfoY + 2*InfoHeightY_Info - 2*2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2 + 2), txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
		}
		txt = g_Locale->getText(LOCALE_MOVIEBROWSER_MENU_MAIN_MOVIEINFO);
		if ( txt != NULL )
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, BoxEndX- ICON_OFFSET- 3* ButtonWidth + 2 - 1, BoxEndInfoY + 2*InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) - 2*3);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- 3* ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2) + 8, BoxEndInfoY + 2*InfoHeightY_Info - 2*2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_GREEN_WIDTH + 2 + 2), txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
		};
	}	
	// Bar 3
	//txt = "(1),(4),(7) << 1,5,10 min       (3),(6),(9) >> 1,5,10 min       (0) Re-Sync";
	if(1)
	{
		txt = "<< 1/5/10 min";
		if ( txt != NULL )
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_1, BoxEndX- ICON_OFFSET- 4* ButtonWidth + 2- 5, BoxEndInfoY + 3*InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) - 3*3);
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_4, BoxEndX- ICON_OFFSET- 4* ButtonWidth + 2- 5+20, BoxEndInfoY + 3*InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) - 3*3);
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_7, BoxEndX- ICON_OFFSET- 4* ButtonWidth + 2- 5+40, BoxEndInfoY + 3*InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) - 3*3);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- 4* ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2) +40, BoxEndInfoY + 3*InfoHeightY_Info - 3*2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_RED_WIDTH + 2 + 2) + 8+40, txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
		}
		txt = ">> 1/5/10 min";
		if ( txt != NULL )
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_3, BoxEndX- ICON_OFFSET- 2* ButtonWidth + 2- 7, BoxEndInfoY + 3*InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) - 3*3);
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_6, BoxEndX- ICON_OFFSET- 2* ButtonWidth + 2- 7+20, BoxEndInfoY + 3*InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) - 3*3);
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_9, BoxEndX- ICON_OFFSET- 2* ButtonWidth + 2- 7+40, BoxEndInfoY + 3*InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) - 3*3);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- 2* ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2)+40, BoxEndInfoY + 3*InfoHeightY_Info - 3*2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2 + 2)+8+40, txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
		}
		txt = g_Locale->getText(LOCALE_MOVIEPLAYER_TSHELP16);
		if ( txt != NULL )
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_0, BoxEndX- ICON_OFFSET- 0* ButtonWidth + 2- 7, BoxEndInfoY + 3*InfoHeightY_Info- ((InfoHeightY_Info+ 16)>>1) - 3*3);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- ICON_OFFSET- 0* ButtonWidth + (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2), BoxEndInfoY + 3*InfoHeightY_Info - 3*2, ButtonWidth - (2 + NEUTRINO_ICON_BUTTON_YELLOW_WIDTH + 2 + 2), txt, COL_INFOBAR_BUTTONS, 0, true); // UTF-8
		}
	}	
}

