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

 - always: fix bugs and be nice to neutrino (state handling, background handling, interruptable etc.)
 
(currently planned order)
 - make sure that zapit works after movieplayer has been quit
 - read TS from socket (UDP?)
 - PES2TS on server side
 - DivX,XVid,AVI,MPG,etc.->TS on server side
 - Pause/Resume (removed due to performance penalty)
 - Add Error handling
 - Bookmarks
 - LCD support

*/

#include <config.h>
#if HAVE_DVB_API_VERSION >= 3
#include <global.h>
#include <neutrino.h>
#include <zapit/debug.h>

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
	t_channel_id serviceId = g_Zapit->getCurrentServiceID();
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
	// Start Sectionsd
	g_Sectionsd->setPauseScanning(false);
	// Restore last mode
	//t_channel_id channel_id=CNeutrinoApp::getInstance()->channelList->getActiveChannel_ChannelID();
	//g_Zapit->zapTo_serviceID(channel_id);
	g_Zapit->setStandby(false);

	/*g_Zapit->setStandby(true);
	g_Sectionsd->setPauseScanning(true);
	g_Sectionsd->setPauseScanning(false);
	g_Zapit->setStandby(false);*/
	g_Zapit->reinitChannels();
	g_Zapit->startPlayBack();
	g_Zapit->zapTo(serviceId);
	//g_Zapit->startPlayBack(serviceId);
	//g_Zapit->zapTo_serviceID(serviceId);

	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , m_LastMode );
	sleep(3); // zapit doesnt like fast zapping in the moment
        g_Zapit->zapTo_serviceID(serviceId);
	// always exit all
	return menu_return::RETURN_EXIT_ALL;
}

int CMoviePlayerGui::show()
{
	int res = -1;

	//TODO: Own LCD support
	CLCD::getInstance()->setMode(CLCD::MODE_MP3);

	uint msg; uint data;

	bool loop=true;
	bool update=true;
	key_level=0;

	while(loop)
	{
		if(CNeutrinoApp::getInstance()->getMode()!=NeutrinoMessages::mode_ts)
		{
			// stop if mode was changed in another thread
			loop=false;
		}

		if(update)
		{
			hide();
			update=false;
			paint();
		}

		// Check Remote Control

		g_RCInput->getMsg( &msg, &data, 10 ); // 1 sec timeout to update play/stop state display

		if( msg == CRCInput::RC_home)
		{ //Exit after cancel key
			loop=false;
		}
		else if( msg == CRCInput::RC_timeout )
		{
			// do nothing
		}
//------------ GREEN --------------------
		else if(msg==CRCInput::RC_green)
		{
		//INFO("Green (Play TS File");
		hide();
		const char *tsfilename;
		if(filebrowser->exec(Path))
			{
			Path=filebrowser->getCurrentDir();
			tsfilename = filebrowser->getSelectedFile()->Name.c_str();
			//INFO("TS Filename %s",tsfilename);
			}
		unsigned char buf[384*188];
		unsigned short pida, pidv;
		int dmxa, dmxv, dvr, adec, vdec, ts;
		struct dmx_pes_filter_params p;
		ssize_t wr;
		size_t r;
		int done;

		int fd;

		if ((fd = open(tsfilename,O_RDONLY|O_LARGEFILE)) < 0){
			//handle error
		}
		find_avpids(fd, &pidv, &pida);
		INFO("pida: %i",pida);
		INFO("pidv: %i",pidv);

		if ((dmxa = open(DMX, O_RDWR)) < 0) {
		 //handle error
		}


		if ((dmxv = open(DMX, O_RDWR)) < 0) {
		 //handle error
		}

		if ((dvr = open(DVR, O_WRONLY)) < 0) {
		 //handle error
		}

		if ((adec = open(ADEC, O_RDWR)) < 0) {
		 //handle error
		}

		if ((vdec = open(VDEC, O_RDWR)) < 0) {
		 //handle error
		}

		if ((ts = open(tsfilename, O_RDONLY)) < 0) {
		 //handle error
		}

		p.pid = pida;
		p.input = DMX_IN_DVR;
		p.output = DMX_OUT_DECODER;
		p.pes_type = DMX_PES_AUDIO;
		p.flags = DMX_IMMEDIATE_START;

		if (ioctl(dmxa, DMX_SET_PES_FILTER, &p) < 0) {
		 //handle error
		}

		if (ioctl(adec, AUDIO_STOP) < 0) {
			//perror("AUDIO_STOP");
			//return 1;
		}

		if (ioctl(vdec, VIDEO_STOP) < 0) {
			//perror("VIDEO_STOP");
			//return 1;
		}

	    	p.pid = pidv;
		p.input = DMX_IN_DVR;
		p.output = DMX_OUT_DECODER;
		p.pes_type = DMX_PES_VIDEO;
		p.flags = DMX_IMMEDIATE_START;

		if (ioctl(dmxv, DMX_SET_PES_FILTER, &p) < 0) {
		 //handle error
		}

		if (ioctl(adec, AUDIO_PLAY) < 0) {
		 //handle error
		}

		if (ioctl(vdec, VIDEO_PLAY) < 0) {
		 //handle error
		}

		while ((r = read(ts, buf, sizeof(buf))) > 0) {
			done = 0;
			while (r > 0) {
				if ((wr = write(dvr, &buf[done], r)) <= 0)
					continue;
				r -= wr;
				done += wr;

			}
			
			// TODO: re-add Pause/Resume; currently removed due to performance problems with newest drivers
			/*g_RCInput->getMsg( &msg, &data, 1 ); // 1/10 sec timeout to update play/stop state display
			if( msg == CRCInput::RC_red || msg == CRCInput::RC_home) {
				loop=false;
				break;
			}
			else if (msg == CRCInput::RC_yellow) {
				while (true) {
					g_RCInput->getMsg (&msg2, &data,10);
					if (msg2 == CRCInput::RC_yellow || msg2 == CRCInput::RC_home)
						break;
				}
			}*/
			

		}

		ioctl(vdec, VIDEO_STOP);
		ioctl(adec, AUDIO_STOP);
		ioctl(dmxv, DMX_STOP);
		ioctl(dmxa, DMX_STOP);

		close(ts);
		close(vdec);
		close(adec);
		close(dvr);
		close(dmxv);
		close(dmxa);
		paint();
		}
//------------ YELLOW --------------------
		else if(msg==CRCInput::RC_yellow)
		{
		//INFO("Yellow: Play PS file");
		hide();
		const char *psfilename;
		if(filebrowser->exec(Path))
			{
			Path=filebrowser->getCurrentDir();
			psfilename = filebrowser->getSelectedFile()->Name.c_str();
			//INFO("PS Filename %s",psfilename);
			}
		unsigned short pida, pidv;
		int dmxa, dmxv, dvr, adec, vdec, ps;
		struct dmx_pes_filter_params p;

		//define a pida and pidv however you want
		pida=0x900;
		pidv=0x8ff;
		int fd;

		if ((fd = open(psfilename,O_RDONLY|O_LARGEFILE)) < 0){
			//handle error
		}
		//find_avpids(fd, &pidv, &pida);
		INFO("preset pida: %i",pida);
		INFO("preset pidv: %i",pidv);

		if ((dmxa = open(DMX, O_RDWR)) < 0) {
		 //handle error
		}


		if ((dmxv = open(DMX, O_RDWR)) < 0) {
		 //handle error
		}

		if ((dvr = open(DVR, O_WRONLY)) < 0) {
		 //handle error
		}

		if ((adec = open(ADEC, O_RDWR)) < 0) {
		 //handle error
		}

		if ((vdec = open(VDEC, O_RDWR)) < 0) {
		 //handle error
		}

		if ((ps = open(psfilename, O_RDONLY)) < 0) {
		 //handle error
		}

		p.pid = pida;
		p.input = DMX_IN_DVR;
		p.output = DMX_OUT_DECODER;
		p.pes_type = DMX_PES_AUDIO;
		p.flags = DMX_IMMEDIATE_START;

		if (ioctl(dmxa, DMX_SET_PES_FILTER, &p) < 0) {
		 //handle error
		}

		if (ioctl(adec, AUDIO_STOP) < 0) {
			//perror("AUDIO_STOP");
			//return 1;
		}

		if (ioctl(vdec, VIDEO_STOP) < 0) {
			//perror("VIDEO_STOP");
			//return 1;
		}

	    	p.pid = pidv;
		p.input = DMX_IN_DVR;
		p.output = DMX_OUT_DECODER;
		p.pes_type = DMX_PES_VIDEO;
		p.flags = DMX_IMMEDIATE_START;

		if (ioctl(dmxv, DMX_SET_PES_FILTER, &p) < 0) {
		 //handle error
		}

		if (ioctl(adec, AUDIO_PLAY) < 0) {
		 //handle error
		}

		if (ioctl(vdec, VIDEO_PLAY) < 0) {
		 //handle error
		}
		//INFO("Writing to fd %i",dvr);
                // Use the pida and pidv as defined above
                pes_to_ts2( ps, dvr, 0x900, 0x8ff);
                // ^ VERY bad performance!!!

		ioctl(vdec, VIDEO_STOP);
		ioctl(adec, AUDIO_STOP);
		ioctl(dmxv, DMX_STOP);
		ioctl(dmxa, DMX_STOP);

		close(ps);
		close(vdec);
		close(adec);
		close(dvr);
		close(dmxv);
		close(dmxa);
		paint();
		}
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

		frameBuffer->paintIcon("rot.raw", x+ 0* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x + 0* ButtonWidth + 30, y+(height-info_height-2*buttonHeight)+24 - 1,
						     ButtonWidth- 20, g_Locale->getText("movieplayer.bookmark").c_str(), COL_INFOBAR, 0, true); // UTF-8


		frameBuffer->paintIcon("gruen.raw", x+ 1* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x+ 1* ButtonWidth +30, y+(height-info_height-2*buttonHeight)+24 - 1,
						     ButtonWidth- 20, g_Locale->getText("movieplayer.choosets").c_str(), COL_INFOBAR, 0, true); // UTF-8

		frameBuffer->paintIcon("gelb.raw", x+ 2* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x+ 2* ButtonWidth + 30, y+(height-info_height-2*buttonHeight)+24 - 1,
						     ButtonWidth- 20, g_Locale->getText("movieplayer.chooseps").c_str(), COL_INFOBAR, 0, true); // UTF-8

}
//------------------------------------------------------------------------
void CMoviePlayerGui::paintInfo()
{
}
//------------------------------------------------------------------------

void CMoviePlayerGui::paint()
{
        frameBuffer->loadPal("movieplayerbg.pal", 18, COL_MAXFREE);
	frameBuffer->loadBackground("movieplayerbg.raw");
	frameBuffer->useBackground(true);
	frameBuffer->paintBackground();

	paintHead();
	paintImg();
	paintFoot();

	visible = true;
}

//------------------------------------------------------------------------

void CMoviePlayerGui::paintLCD()
{
}
#endif
