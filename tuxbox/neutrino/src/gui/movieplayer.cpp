/*
	Neutrino-GUI  -   DBoxII-Project

	Movieplayer (c) 2003 by giggo
	Based on code by Dirch, obi and the Metzler Bros. Thanks.

	Homepage: http://www.giggo.de/dbox

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

/* KNOWN ISSUES:
 * - PS Playback does not work on Nokia Cable as the performance is too bad; I left it in because it might work on other boxes; It will be replaced by a server-side ps2ts conversion anyway
 *
*/


/* TODOs / Release Plan:

 - always: fix bugs

(currently planned order)
 - read TS from socket (UDP?)
 - PES2TS on server side
 - DivX,XVid,AVI,MPG,etc.->TS on server side
 - Bookmarks

*/
#define MORE_THAN_TS 1
#include <config.h>
#if HAVE_DVB_API_VERSION >= 3
#undef _FILE_OFFSET_BITS
#include <global.h>
#include <neutrino.h>
#include <system/debug.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>
#include <system/settings.h>
#include <algorithm>
#include <sys/time.h>
#include <fstream>

#include "eventlist.h"
#include "movieplayer.h"
#include <transform.h>
#include "color.h"
#include "infoviewer.h"
#include "nfs.h"

#include "widget/menue.h"
#include "widget/messagebox.h"
#include "widget/hintbox.h"
#include "widget/stringinput.h"

#include <fcntl.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ADAP	"/dev/dvb/adapter0"
#define ADEC	ADAP "/audio0"
#define VDEC	ADAP "/video0"
#define DMX	ADAP "/demux0"
#define DVR	ADAP "/dvr0"

#define AVIA_AV_STREAM_TYPE_0           0x00
#define AVIA_AV_STREAM_TYPE_SPTS        0x01
#define AVIA_AV_STREAM_TYPE_PES         0x02
#define AVIA_AV_STREAM_TYPE_ES          0x03

#define ConnectLineBox_Width	15

static int playstate;
static bool isTS;
int speed = 1;


//------------------------------------------------------------------------

CMoviePlayerGui::CMoviePlayerGui()
{
	frameBuffer = CFrameBuffer::getInstance();

	visible = false;
	selected = 0;

	filebrowser = new CFileBrowser();
	filebrowser->Multi_Select = false;
	filebrowser->Dirs_Selectable = false;
	videofilefilter.addFilter("ts");
	videofilefilter.addFilter("ps");
        videofilefilter.addFilter("mpg");
	filebrowser->Filter = &videofilefilter;
   	if(strlen(g_settings.network_nfs_local_dir[0])!=0)
      		Path = g_settings.network_nfs_local_dir[0];
	else
      		Path = "/";
}

//------------------------------------------------------------------------

CMoviePlayerGui::~CMoviePlayerGui()
{
	delete filebrowser;
	g_Zapit->setStandby(false);
	g_Sectionsd->setPauseScanning(false);

}

//------------------------------------------------------------------------
int CMoviePlayerGui::exec(CMenuTarget* parent, std::string actionKey)
{
	m_state=STOP;
	current=-1;
	selected = 0;

	//define screen width
	width = 710;
	if((g_settings.screen_EndX- g_settings.screen_StartX) < width+ConnectLineBox_Width)
		width=(g_settings.screen_EndX- g_settings.screen_StartX)-ConnectLineBox_Width;

	//define screen height
	height = 570;
	if((g_settings.screen_EndY- g_settings.screen_StartY) < height)
		height=(g_settings.screen_EndY- g_settings.screen_StartY);
	buttonHeight = min(25,g_Fonts->infobar_small->getHeight());
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->menu->getHeight();
	sheight= g_Fonts->infobar_small->getHeight();
	title_height=fheight*2+20+sheight+4;
	info_height=fheight*2;
	listmaxshow = (height-info_height-title_height-theight-2*buttonHeight)/(fheight);
	height = theight+info_height+title_height+2*buttonHeight+listmaxshow*fheight;	// recalc height

	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-(width+ConnectLineBox_Width)) / 2) + g_settings.screen_StartX + ConnectLineBox_Width;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height)/ 2) + g_settings.screen_StartY;

	if(parent)
	{
		parent->hide();
	}

	// set zapit in standby mode
	g_Zapit->setStandby(true);

	// tell neutrino we're in ts_mode
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_ts );
	// remember last mode
	m_LastMode=(CNeutrinoApp::getInstance()->getLastMode() /*| NeutrinoMessages::norezap*/);

	// Stop sectionsd
	g_Sectionsd->setPauseScanning(true);


	show();

	//stop();
	hide();

	g_Zapit->setStandby(false);

	// Start Sectionsd
	g_Sectionsd->setPauseScanning(false);

	// Restore last mode
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , m_LastMode );

	// always exit all
	return menu_return::RETURN_EXIT_ALL;
}

void* Play_Thread( void* filename )
{
	bool failed = false;
	unsigned char buf[384*188*2];
	unsigned short pida = 0, pidv = 0;
	int done, fd = 0, dmxa = 0, dmxv = 0, dvr = 0, adec = 0, vdec = 0;
	struct dmx_pes_filter_params p;
	ssize_t wr = 0;
	ssize_t cache = sizeof(buf);
	size_t r = 0;
	ipack pack;

	if( (char *) filename == NULL ) {
	    	playstate = 0;
		pthread_exit(NULL);
	}

	if( (fd = open((char *) filename, O_RDONLY|O_LARGEFILE )) < 0 ) {
	    	playstate = 0;
		pthread_exit(NULL);
	}

	// todo: check if file is valid ts or pes
	if( isTS )
	{
		find_avpids( fd, &pidv, &pida );
		fprintf( stdout, "[movieplayer.cpp] found pida: 0x%04X ; pidv: 0x%04X\n", pida, pidv );
	}
	else
	{	// Play PES
		pida=0x900;
		pidv=0x8ff;
	}
	lseek( fd, 0L, SEEK_SET );

	if( (dmxa = open(DMX, O_RDWR)) < 0 || (dmxv = open(DMX, O_RDWR)) < 0 || (dvr  = open(DVR, O_WRONLY)) < 0 ||
	    (adec = open(ADEC, O_RDWR)) < 0 || (vdec = open(VDEC, O_RDWR)) < 0 )
	{
		failed = true;
	}

	p.input = DMX_IN_DVR;
	p.output = DMX_OUT_DECODER;
	p.flags = DMX_IMMEDIATE_START;

	p.pid = pida;
	p.pes_type = DMX_PES_AUDIO;

	if( ioctl(dmxa, DMX_SET_PES_FILTER, &p) < 0)
	{
		failed = true;
	}

	p.pid = pidv;
	p.pes_type = DMX_PES_VIDEO;
	if( ioctl(dmxv, DMX_SET_PES_FILTER, &p) < 0)
	{
		failed = true;
	}

	if( isTS && !failed )
	{
		while( (r = read(fd, buf, cache)) > 0 && playstate >= 1 )
		{
			done = 0;
			wr = 0;

			switch( playstate )
			{
				case 2:	// pause
					while (playstate == 2)
					{
						ioctl(dmxa, DMX_STOP);
					}
					break;

				case 3:	// ff
				case 4:	// rew
					ioctl(dmxa, DMX_STOP);
					lseek( fd, cache*speed, SEEK_CUR );
					break;

				case 99: // Softreset
					speed = 1;
					ioctl(vdec, VIDEO_STOP);
					ioctl(adec, AUDIO_STOP);
					ioctl(dmxv, DMX_STOP);
					ioctl(dmxa, DMX_STOP);
					ioctl(dmxv, DMX_START);
					ioctl(dmxa, DMX_START);
					ioctl(vdec, VIDEO_PLAY);
					ioctl(adec, AUDIO_PLAY);
					playstate = 1;
			}

			do
			{
				wr = write(dvr, &buf[done], r);
				if ( playstate > 1) break;
				if (!done ) cache = wr;
				done += wr;
				r -= wr;
			}
			while( r );
		}
	}
	else if( !failed )
	{
		ioctl(vdec, VIDEO_PLAY);
		ioctl(adec, AUDIO_PLAY);
		ioctl(dmxv, DMX_START);
		ioctl(dmxa, DMX_START);
		pes_to_ts2( fd, dvr, pida, pidv, &playstate );	// VERY bad performance!!!
	}

	ioctl(vdec, VIDEO_STOP);
	ioctl(adec, AUDIO_STOP);
	ioctl(dmxv, DMX_STOP);
	ioctl(dmxa, DMX_STOP);

	close(fd);
	close(dmxa);
	close(dmxv);
	close(dvr);
	close(adec);
	close(vdec);

	isTS = true;					// to let the fast exit also work in pes mode ;)
	g_RCInput->postMsg( CRCInput::RC_red, 0 );	// for faster exit in PlayStream()

	playstate = 0;

	pthread_exit(NULL);
}

void CMoviePlayerGui::PlayStream( void )
{
	uint msg, data;
	bool update_lcd = true, open_filebrowser = true, start_play = false, exit = false;

	playstate = 0;

	/* playstate == 0 : stopped
	 * playstate == 1 : playing
	 * playstate == 2 : pause-mode
	 * playstate == 3 : fast-forward
	 * playstate == 4 : rewind
	 * playstate == 99: softreset without clearing buffer (playstate toggle to 1)
	 */

	do
	{
		if( exit )
		{
			exit = false;

			if( playstate >= 1 )
			{
					playstate = 0;
					break;
			}
		}

		if( open_filebrowser )
		{
			open_filebrowser = false;
			filename = NULL;

			if( filebrowser->exec(Path) )
			{
				Path = filebrowser->getCurrentDir();

				if( (filename = filebrowser->getSelectedFile()->Name.c_str()) != NULL )
				{
					update_lcd = true;
					start_play = true;
				}
			}
			else
			{
				if( playstate == 0 )
					break;
			}

			CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
		}

		if( start_play )
		{
			start_play = false;

			if( playstate >= 1 )
			{
				playstate = 0;
				pthread_join( rct, NULL );
			}
			playstate = 99;

			if( pthread_create(&rct, 0, Play_Thread, (void *) filename) != 0 )
			{
				playstate = 0;
				break;
			}
		}

		if( update_lcd )
		{
			update_lcd = false;

			if( playstate == 1 )
				CLCD::getInstance()->showServicename(filebrowser->getSelectedFile()->getFileName());
			else
				CLCD::getInstance()->showServicename("("+filebrowser->getSelectedFile()->getFileName()+")");
		}

		g_RCInput->getMsg( &msg, &data, 100 );	// 10 secs..

		if( msg == CRCInput::RC_red ||
		    msg == CRCInput::RC_home )
		{
			//exit play
			exit = true;
		}
		else if( msg == CRCInput::RC_yellow )
		{
		  	if( playstate != 2 )
		  	{
		  		// pause play
		  		update_lcd = true;
		  		playstate = 2;
		  	}
		  	else
		  	{
		  		// resume play
		  		update_lcd = true;
		  		playstate = 99;
		  	}
		}
		else if( msg == CRCInput::RC_left )
		{
			// rewind
			if ( speed > 1) speed = 1;
			speed *= -2;
			speed *= (speed > 1 ? -1 : 1);
			playstate = 3;
		}
		else if( msg == CRCInput::RC_right )
		{
			// fast-forward
			if ( speed < 1) speed = 1;
			speed *= 2;
			playstate = 4;
		}
		else if( msg == CRCInput::RC_up ||
			 msg == CRCInput::RC_down )
		{
			// todo: next/prev file
		}
		else if( msg == CRCInput::RC_help )
		{
			// todo: infobar
		}
		else if( msg == CRCInput::RC_ok )
		{
			if (playstate > 1)
			{
				playstate = 99;
			}
			else
				open_filebrowser = true;
		}
		else if( msg == NeutrinoMessages::RECORD_START ||
			 msg == NeutrinoMessages::ZAPTO ||
			 msg == NeutrinoMessages::STANDBY_ON ||
			 msg == NeutrinoMessages::SHUTDOWN ||
			 msg == NeutrinoMessages::SLEEPTIMER )
		{
			// Exit for Record/Zapto Timers
			isTS = true;	// also exit in PES Mode
			exit = true;
			g_RCInput->postMsg( msg, data );
		}
		else if( CNeutrinoApp::getInstance()->handleMsg( msg, data ) == messages_return::cancel_all )
		{
			isTS = true;	// also exit in PES Mode
			exit = true;
		}
	} while( playstate >= 1 );

	pthread_join( rct, NULL );
}

int CMoviePlayerGui::show()
{
	int res = -1;
	uint msg, data;
	bool loop = true, update = true;

	key_level = 0;

	while(loop)
	{
		if(CNeutrinoApp::getInstance()->getMode()!=NeutrinoMessages::mode_ts)
		{
			// stop if mode was changed in another thread
			loop = false;
		}

		if(update)
		{
			hide();
			update = false;
			paint();
		}

		// Check Remote Control

		g_RCInput->getMsg( &msg, &data, 10 ); // 1 sec timeout to update play/stop state display

		if( msg == CRCInput::RC_home )
		{ //Exit after cancel key
			loop = false;
		}
		else if( msg == CRCInput::RC_timeout )
		{
			// do nothing
		}
//------------ GREEN --------------------
		else if( msg==CRCInput::RC_green )
		{
			hide();
			isTS = true;
			PlayStream();
			paint();
		}
//------------ YELLOW --------------------
#ifdef MORE_THAN_TS
		else if(msg==CRCInput::RC_yellow)
		{
			hide();
			isTS = false;
			PlayStream();
			paint();
		}
#endif
		else if(msg == NeutrinoMessages::CHANGEMODE)
		{
			if((data & NeutrinoMessages::mode_mask) !=NeutrinoMessages::mode_ts)
			{
				loop = false;
				m_LastMode=data;
			}
		}
		else if(msg == NeutrinoMessages::RECORD_START ||
				  msg == NeutrinoMessages::ZAPTO ||
				  msg == NeutrinoMessages::STANDBY_ON ||
				  msg == NeutrinoMessages::SHUTDOWN ||
				  msg == NeutrinoMessages::SLEEPTIMER)
		{
			// Exit for Record/Zapto Timers
			// Add bookmark
			loop = false;
			g_RCInput->postMsg(msg, data);
		}
		else
		{
			if( CNeutrinoApp::getInstance()->handleMsg( msg, data ) == messages_return::cancel_all )
			{
				loop = false;
			}
			// update mute icon
			paintHead();
		}
	}
	hide();

	if(m_state != STOP) {
		//stop();
        }

	return(res);
}

//------------------------------------------------------------------------

void CMoviePlayerGui::hide()
{
	if(visible)
	{
		frameBuffer->paintBackgroundBoxRel(x-ConnectLineBox_Width-1, y+title_height-1, width+ConnectLineBox_Width+2, height+2-title_height);
		frameBuffer->paintBackgroundBoxRel(x, y, width, title_height);
		frameBuffer->ClearFrameBuffer();
		visible = false;
	}
}

//------------------------------------------------------------------------

void CMoviePlayerGui::paintHead()
{
//	printf("paintHead{\n");
	std::string strCaption = g_Locale->getText("movieplayer.head");
	frameBuffer->paintBoxRel(x,y+title_height, width,theight, COL_MENUHEAD);
	frameBuffer->paintIcon("movie.raw",x+7,y+title_height+10);
	g_Fonts->menu_title->RenderString(x+35,y+theight+title_height+0, width- 45, strCaption.c_str(), COL_MENUHEAD, 0, true); // UTF-8
	int ypos=y+title_height;
	if(theight > 26)
		ypos = (theight-26) / 2 + y + title_height;
	frameBuffer->paintIcon("dbox.raw", x+ width- 30, ypos );
	if( CNeutrinoApp::getInstance()->isMuted() )
	{
		int xpos=x+width-75;
		ypos=y+title_height;
		if(theight > 32)
			ypos = (theight-32) / 2 + y + title_height;
		frameBuffer->paintIcon("mute.raw", xpos, ypos);
	}
	visible = true;

}

//------------------------------------------------------------------------

void CMoviePlayerGui::paintImg()
{
	// TODO: find better image
	frameBuffer->paintBoxRel(x,y+title_height+theight, width,height-info_height-2*buttonHeight-title_height-theight, COL_BACKGROUND);
	frameBuffer->paintIcon("movieplayer.raw",x+25,y+15+title_height+theight);
}

//------------------------------------------------------------------------
void CMoviePlayerGui::paintFoot()
{
	if(m_state==STOP) // insurance
		key_level=0;
	int ButtonWidth = (width-20) / 4;
	frameBuffer->paintBoxRel(x,y+(height-info_height-2*buttonHeight), width,2*buttonHeight, COL_MENUHEAD);
	frameBuffer->paintHLine(x, x+width-x,  y+(height-info_height-2*buttonHeight), COL_INFOBAR_SHADOW);

#ifdef MORE_THAN_TS
	frameBuffer->paintIcon("rot.raw", x+ 0* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x + 0* ButtonWidth + 30, y+(height-info_height-2*buttonHeight)+24 - 1,
						     ButtonWidth- 20, g_Locale->getText("movieplayer.bookmark").c_str(), COL_INFOBAR, 0, true); // UTF-8
#endif

		frameBuffer->paintIcon("gruen.raw", x+ 1* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x+ 1* ButtonWidth +30, y+(height-info_height-2*buttonHeight)+24 - 1,
						     ButtonWidth- 20, g_Locale->getText("movieplayer.choosets").c_str(), COL_INFOBAR, 0, true); // UTF-8

#ifdef MORE_THAN_TS
		frameBuffer->paintIcon("gelb.raw", x+ 2* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x+ 2* ButtonWidth + 30, y+(height-info_height-2*buttonHeight)+24 - 1,
						     ButtonWidth- 20, g_Locale->getText("movieplayer.chooseps").c_str(), COL_INFOBAR, 0, true); // UTF-8
#endif
}

void CMoviePlayerGui::paint()
{
	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
	CLCD::getInstance()->showServicename("Movieplayer");
	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->loadBackground("radiomode.raw");
	frameBuffer->useBackground(true);
	frameBuffer->paintBackground();

	paintHead();
	paintImg();
	paintFoot();

	visible = true;
}

#endif
