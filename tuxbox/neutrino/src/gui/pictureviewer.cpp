/*
	Neutrino-GUI  -   DBoxII-Project

	MP3Player by Dirch
	
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

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>
#include <system/settings.h>
#include <algorithm>
#include <sys/time.h>

#include "pictureviewer.h"

#include "widget/menue.h"
#include "widget/messagebox.h"
#include "widget/hintbox.h"
#include "widget/stringinput.h"


CPictureViewerGui::CPictureViewerGui()
{
	frameBuffer = CFrameBuffer::getInstance();

	visible = false;
	selected = 0;

	filebrowser = new CFileBrowser();
	filebrowser->Multi_Select = true;
	filebrowser->Dirs_Selectable = true;
	picture_filter.addFilter("png");
	picture_filter.addFilter("bmp");
	picture_filter.addFilter("jpg");
	picture_filter.addFilter("gif");
	filebrowser->Filter = &picture_filter;
	Path = "/";
}

//------------------------------------------------------------------------

CPictureViewerGui::~CPictureViewerGui()
{
	playlist.clear();
	delete filebrowser;
}

//------------------------------------------------------------------------
int CPictureViewerGui::exec(CMenuTarget* parent, string actionKey)
{
	m_state=STOP;
	current=-1;
	selected = 0;
	width = 710;
	if((g_settings.screen_EndX- g_settings.screen_StartX) < width)
		width=(g_settings.screen_EndX- g_settings.screen_StartX);
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

	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height)/ 2) + g_settings.screen_StartY;

	if(parent)
	{
		parent->hide();
	}

	// set radio mode background
//	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
//	frameBuffer->loadBackground("radiomode.raw");
/*	frameBuffer->loadPal("scan.pal", 37, COL_MAXFREE);
	frameBuffer->loadBackground("scan.raw");*/
	frameBuffer->setBackgroundColor(0);
	frameBuffer->paintBackgroundBox(0,0,720,576);
	frameBuffer->useBackground(true);
	frameBuffer->paintBackground();

	// set zapit in standby mode
	g_Zapit->setStandby(true);

	// tell neutrino we're in mp3_mode
	//CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_mp3 );
	// remember last mode
	//m_LastMode=(CNeutrinoApp::getInstance()->getLastMode() /*| NeutrinoMessages::norezap*/);

	// Stop sectionsd
	g_Sectionsd->setPauseScanning(true); 

	/*int ret =*/

	show();

	// Restore normal background
	if(frameBuffer->getActive())
		memset(frameBuffer->getFrameBufferPointer(), 255, frameBuffer->getStride()*576);
	frameBuffer->useBackground(false);

	// Start Sectionsd
	g_Sectionsd->setPauseScanning(false);

	// Restore last mode
	//t_channel_id channel_id=CNeutrinoApp::getInstance()->channelList->getActiveChannel_ChannelID();
	//g_Zapit->zapTo_serviceID(channel_id);
	g_Zapit->setStandby(false);
	//CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , m_LastMode );
	//sleep(3); // zapit doesnt like fast zapping in the moment

	// always exit all	
	return menu_return::RETURN_EXIT_ALL;
}

//------------------------------------------------------------------------

int CPictureViewerGui::show()
{
	int res = -1;

	//CLCD::getInstance()->setMode(CLCD::MODE_MP3);

	uint msg; uint data;

	bool loop=true;
	bool update=true;
	
	while(loop)
	{
		if(update)
		{
			hide();
			update=false;
			paint();
		}
		g_RCInput->getMsg( &msg, &data, 10 ); // 1 sec timeout to update play/stop state display

		if( msg == CRCInput::RC_home)
		{ //Exit after cancel key
			loop=false;
		}
		else if( msg == CRCInput::RC_timeout )
		{
			// do nothing
		}
		else if( msg == CRCInput::RC_left)
		{

		}
		else if( msg == CRCInput::RC_right)
		{

		}
		else if( msg == CRCInput::RC_up && playlist.size() > 0)
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = playlist.size()-1;
			}
			else
				selected--;
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				update=true;
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if( msg == CRCInput::RC_down && playlist.size() > 0)
		{
			int prevselected=selected;
			selected = (selected+1)%playlist.size();
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				update=true;
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if( msg == CRCInput::RC_ok && playlist.size() > 0)
		{
			// OK button
			view(selected);
			update=true;
		}
		else if(msg==CRCInput::RC_red )
		{
		}
		else if(msg==CRCInput::RC_green)
		{
			hide();
			if(filebrowser->exec(Path))
			{
				Path=filebrowser->getCurrentDir();
				CFileList::iterator files = filebrowser->getSelectedFiles()->begin();
				for(; files != filebrowser->getSelectedFiles()->end();files++)
				{
					if(files->getType() == CFile::FILE_PICTURE)
					{
						CPicture pic;
						pic.Filename = files->Name;
						playlist.push_back(pic);
					}
					else
						printf("Wrong Filetype %d\n",files->getType());
				}
			}
//				CLCD::getInstance()->setMode(CLCD::MODE_MP3);
			update=true;
		}
		else if(msg==CRCInput::RC_yellow)
		{
			playlist.clear();
			current=-1;
			selected=0;
			update=true;
		}
		else if(msg==CRCInput::RC_blue)
		{
		}
		else if(msg==CRCInput::RC_help)
		{
		}
		else if( ( msg >= CRCInput::RC_1 ) && ( msg <= CRCInput::RC_9 ) && playlist.size() > 0)
		{ //numeric zap
			int x1=(g_settings.screen_EndX- g_settings.screen_StartX)/2 + g_settings.screen_StartX-50;
			int y1=(g_settings.screen_EndY- g_settings.screen_StartY)/2 + g_settings.screen_StartY;
			string num;
			int val=0;
			char str[11];
			do
			{
				val = val * 10 + CRCInput::getNumericValue(msg);
				sprintf(str,"%d",val);
				int w=g_Fonts->channel_num_zap->getRenderWidth(str);
				frameBuffer->paintBoxRel(x1-7, y1-g_Fonts->channel_num_zap->getHeight()-5, w+14, 
												 g_Fonts->channel_num_zap->getHeight()+10, COL_MENUCONTENT+6);
				frameBuffer->paintBoxRel(x1-4, y1-g_Fonts->channel_num_zap->getHeight()-3, w+8, 
												 g_Fonts->channel_num_zap->getHeight()+6, COL_MENUCONTENTSELECTED);
				g_Fonts->channel_num_zap->RenderString(x1,y1,w+1,str,COL_MENUCONTENTSELECTED,0);
				g_RCInput->getMsg( &msg, &data, 100 ); 
			} while (g_RCInput->isNumeric(msg) && val < 1000000);
			if(msg==CRCInput::RC_ok)
				selected=min((int)playlist.size(), val)-1;
			update=true;
		}
		else if(msg==CRCInput::RC_0)
		{
			if(current>=0)
			{
				selected=current;
				update=true;
			}
		}
		else if(msg==CRCInput::RC_setup)
		{
		}
		else if(msg == NeutrinoMessages::CHANGEMODE)
		{
			if((data & NeutrinoMessages::mode_mask) !=NeutrinoMessages::mode_mp3)
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

	return(res);
}

//------------------------------------------------------------------------

void CPictureViewerGui::hide()
{
//	printf("hide(){\n");
	if(visible)
	{
		frameBuffer->paintBackgroundBoxRel(x-1, y+title_height-1, width+2, height+2-title_height);
		frameBuffer->paintBackgroundBoxRel(x, y, width, title_height);
		visible = false;
	}
//	printf("hide()}\n");
}

//------------------------------------------------------------------------

void CPictureViewerGui::paintItem(int pos)
{
//	printf("paintItem{\n");
	int ypos = y+ +title_height+theight+0 + pos*fheight;
	int color;
	if( (liststart+pos < playlist.size()) && (pos % 2) )
		color = COL_MENUCONTENTDARK;
	else
		color	= COL_MENUCONTENT;

	if(liststart+pos==selected)
		color = COL_MENUCONTENTSELECTED;

	if(liststart+pos==(unsigned)current)
		color = color+2;

	frameBuffer->paintBoxRel(x,ypos, width-15, fheight, color);
	if(liststart+pos<playlist.size())
	{
		string tmp;
		tmp=playlist[liststart+pos].Filename;
//		int w=g_Fonts->menu->getRenderWidth(playlist[liststart+pos].Duration)+5;
		g_Fonts->menu->RenderString(x+10,ypos+fheight, width-30/*-w*/, tmp.c_str(), color, fheight);
//		g_Fonts->menu->RenderString(x+width-15-w,ypos+fheight, w, playlist[liststart+pos].Duration, color, fheight);

	}
//	printf("paintItem}\n");
}

//------------------------------------------------------------------------

void CPictureViewerGui::paintHead()
{
//	printf("paintHead{\n");
	string strCaption = g_Locale->getText("mp3player.head");
	frameBuffer->paintBoxRel(x,y+title_height, width,theight, COL_MENUHEAD);
	frameBuffer->paintIcon("mp3.raw",x+7,y+title_height+10);
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
//	printf("paintHead}\n");
}

//------------------------------------------------------------------------

void CPictureViewerGui::paintFoot()
{
//	printf("paintFoot{\n");
	if(m_state==STOP) // insurance
		key_level=0;
	int ButtonWidth = (width-20) / 4;
	int ButtonWidth2 = (width-50) / 2;
	frameBuffer->paintBoxRel(x,y+(height-info_height-2*buttonHeight), width,2*buttonHeight, COL_MENUHEAD);
	frameBuffer->paintHLine(x, x+width,  y+(height-info_height-2*buttonHeight), COL_INFOBAR_SHADOW);

	if(playlist.size()>0)
	{
		frameBuffer->paintIcon("ok.raw", x + 1* ButtonWidth2 + 25, y+(height-info_height-buttonHeight)-3);
		g_Fonts->infobar_small->RenderString(x + 1 * ButtonWidth2 + 53 , y+(height-info_height-buttonHeight)+24 - 4, 
						     ButtonWidth2- 28, g_Locale->getText("mp3player.play").c_str(), COL_INFOBAR, 0, true); // UTF-8
	}
	if(m_state!=STOP)
	{
		frameBuffer->paintIcon("help.raw", x+ 0* ButtonWidth + 25, y+(height-info_height-buttonHeight)-3);
		g_Fonts->infobar_small->RenderString(x+ 0* ButtonWidth +53 , y+(height-info_height-buttonHeight)+24 - 4, 
						     ButtonWidth2- 28, g_Locale->getText("mp3player.keylevel").c_str(), COL_INFOBAR, 0, true); // UTF-8
	}

	if(key_level==0)
	{
		if(playlist.size()>0)
		{
			frameBuffer->paintIcon("rot.raw", x+ 0* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
			g_Fonts->infobar_small->RenderString(x + 0* ButtonWidth + 30, y+(height-info_height-2*buttonHeight)+24 - 1, 
							     ButtonWidth- 20, g_Locale->getText("mp3player.delete").c_str(), COL_INFOBAR, 0, true); // UTF-8

			frameBuffer->paintIcon("gelb.raw", x+ 2* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
			g_Fonts->infobar_small->RenderString(x+ 2* ButtonWidth + 30, y+(height-info_height-2*buttonHeight)+24 - 1, 
							     ButtonWidth- 20, g_Locale->getText("mp3player.deleteall").c_str(), COL_INFOBAR, 0, true); // UTF-8

			frameBuffer->paintIcon("blau.raw", x+ 3* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
			g_Fonts->infobar_small->RenderString(x+ 3* ButtonWidth +30 , y+(height-info_height-2*buttonHeight)+24 - 1, 
							     ButtonWidth- 20, g_Locale->getText("mp3player.shuffle").c_str(), COL_INFOBAR, 0, true); // UTF-8
		}

		frameBuffer->paintIcon("gruen.raw", x+ 1* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x+ 1* ButtonWidth +30, y+(height-info_height-2*buttonHeight)+24 - 1, 
						     ButtonWidth- 20, g_Locale->getText("mp3player.add").c_str(), COL_INFOBAR, 0, true); // UTF-8
	}
	else
	{

		frameBuffer->paintIcon("rot.raw", x+ 0* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x + 0* ButtonWidth + 30, y+(height-info_height-2*buttonHeight)+24 - 1, 
						     ButtonWidth- 20, g_Locale->getText("mp3player.stop").c_str(), COL_INFOBAR, 0, true); // UTF-8

		frameBuffer->paintIcon("gruen.raw", x+ 1* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x+ 1* ButtonWidth +30, y+(height-info_height-2*buttonHeight)+24 - 1, 
						     ButtonWidth- 20, g_Locale->getText("mp3player.rewind").c_str(), COL_INFOBAR, 0, true); // UTF-8
		
		frameBuffer->paintIcon("gelb.raw", x+ 2* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x+ 2* ButtonWidth + 30, y+(height-info_height-2*buttonHeight)+24 - 1, 
						     ButtonWidth- 20, g_Locale->getText("mp3player.pause").c_str(), COL_INFOBAR, 0, true); // UTF-8

		frameBuffer->paintIcon("blau.raw", x+ 3* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x+ 3* ButtonWidth +30 , y+(height-info_height-2*buttonHeight)+24 - 1, 
						     ButtonWidth- 20, g_Locale->getText("mp3player.fastforward").c_str(), COL_INFOBAR, 0, true); // UTF-8
	}
//	printf("paintFoot}\n");
}
//------------------------------------------------------------------------
void CPictureViewerGui::paintInfo()
{
}
//------------------------------------------------------------------------

void CPictureViewerGui::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

	paintHead();
	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int ypos = y+title_height+ theight;
	int sb = fheight* listmaxshow;
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT+ 1);

	int sbc= ((playlist.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);

	paintFoot();
	paintInfo();
	
	visible = true;
}

void CPictureViewerGui::view(unsigned int index)
{
	uint msg; uint data;
	frameBuffer->setMode(720,576,16);
	CPictureViewer viewer;
	viewer.ShowImage(playlist[index].Filename);
	g_RCInput->getMsg( &msg, &data, 10000 ); // 1 sec timeout to update play/stop state display
	frameBuffer->setMode(720,576,8);
	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
}
