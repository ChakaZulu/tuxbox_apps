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

#include <assert.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>
#include <system/settings.h>
#include <algorithm>
#include <sys/time.h>
#include <fstream>

#include "eventlist.h"
#include "mp3player.h"
#include "color.h"
#include "infoviewer.h"
#include "nfs.h"

#include "widget/menue.h"
#include "widget/messagebox.h"
#include "widget/hintbox.h"
#include "widget/stringinput.h"

 #include <id3tag.h>

#define ConnectLineBox_Width	15

//------------------------------------------------------------------------
bool sortByIndex (const CMP3& a, const CMP3& b)
{
	return a.Index < b.Index ;
}
//------------------------------------------------------------------------

CMP3PlayerGui::CMP3PlayerGui()
{
	frameBuffer = CFrameBuffer::getInstance();

	visible = false;
	selected = 0;
	m_mp3info = "";

	filebrowser = new CFileBrowser();
	filebrowser->Multi_Select = true;
	filebrowser->Dirs_Selectable = true;
	mp3filter.addFilter("mp3");
	mp3filter.addFilter("m2a");
	mp3filter.addFilter("mpa");
	mp3filter.addFilter("m3u");
	filebrowser->Filter = &mp3filter;
   if(strlen(g_settings.network_nfs_local_dir[0])!=0)
      Path = g_settings.network_nfs_local_dir[0];
	else
      Path = "/";
}

//------------------------------------------------------------------------

CMP3PlayerGui::~CMP3PlayerGui()
{
	playlist.clear();
	delete filebrowser;
}

//------------------------------------------------------------------------
int CMP3PlayerGui::exec(CMenuTarget* parent, std::string actionKey)
{
	m_state=STOP;
	current=-1;
	selected = 0;
	width = 710;
	if((g_settings.screen_EndX- g_settings.screen_StartX) < width+ConnectLineBox_Width)
		width=(g_settings.screen_EndX- g_settings.screen_StartX)-ConnectLineBox_Width;
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

	bool usedBackground = frameBuffer->getuseBackground();
	if (usedBackground)
		frameBuffer->saveBackgroundImage();
	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->loadBackground("radiomode.raw");
	frameBuffer->useBackground(true);
	frameBuffer->paintBackground();

	// set zapit in standby mode
	g_Zapit->setStandby(true);

	// tell neutrino we're in mp3_mode
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_mp3 );
	// remember last mode
	m_LastMode=(CNeutrinoApp::getInstance()->getLastMode() /*| NeutrinoMessages::norezap*/);

	// Stop sectionsd
	g_Sectionsd->setPauseScanning(true); 

	/*int ret =*/

	show();

	// Restore previous background
	if (usedBackground)
		frameBuffer->restoreBackgroundImage();
	frameBuffer->useBackground(usedBackground);
	frameBuffer->paintBackground();

	// Start Sectionsd
	g_Sectionsd->setPauseScanning(false);

	// Restore last mode
	//t_channel_id channel_id=CNeutrinoApp::getInstance()->channelList->getActiveChannel_ChannelID();
	//g_Zapit->zapTo_serviceID(channel_id);
	g_Zapit->setStandby(false);
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , m_LastMode );
	//sleep(3); // zapit doesnt like fast zapping in the moment

	// always exit all	
	return menu_return::RETURN_EXIT_ALL;
}

//------------------------------------------------------------------------

int CMP3PlayerGui::show()
{
	int res = -1;

	CLCD::getInstance()->setMode(CLCD::MODE_MP3);

	uint msg; uint64_t data;

	bool loop=true;
	bool update=true;
	key_level=0;
	while(loop)
	{
		updateTimes();
		updateMP3Infos();
		if(CNeutrinoApp::getInstance()->getMode()!=NeutrinoMessages::mode_mp3)
		{
			// stop if mode was changed in another thread
			loop=false;
		}
		if(m_state != STOP && CMP3Player::getInstance()->state==CMP3Player::STOP && playlist.size() >0)
		{
			int next = getNext();
			play(next);
		}

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
         if(key_level==1)
         {
            if(current-1 > 0)
               play(current-1);
            else
               play(0);
         }
         else
         {
            if ((int(selected)-int(listmaxshow))<0)
               selected=playlist.size()-1;
            else
               selected -= listmaxshow;
            liststart = (selected/listmaxshow)*listmaxshow;
            update=true;
         }

		}
		else if( msg == CRCInput::RC_right)
		{
         if(key_level==1)
         {
            int next = getNext();
            play(next);
         }
         else
         {
            selected+=listmaxshow;
            if (selected>playlist.size()-1)
               selected=0;
            liststart = (selected/listmaxshow)*listmaxshow;
            update=true;
         }
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
			play(selected);
		}
		else if(msg==CRCInput::RC_red )
		{
			if(key_level==0)
			{
				if (playlist.size() > 0)
				{
					CPlayList::iterator p = playlist.begin()+selected;
					playlist.erase(p);
					if((int)selected==current)
					{
						current--;
						stop(); // Stop if song is deleted, next song will be startet automat.
					}
					if(selected > playlist.size()-1)
						selected = playlist.size()-1;
					update=true;
				}
			}
			else
			{
				stop();
			}
		}
		else if(msg==CRCInput::RC_green)
		{
			if(key_level==0)
			{
				hide();
				if(filebrowser->exec(Path))
				{
					Path=filebrowser->getCurrentDir();
					CFileList::iterator files = filebrowser->getSelectedFiles()->begin();
					for(; files != filebrowser->getSelectedFiles()->end();files++)
					{
						if(files->getType() == CFile::FILE_MP3)
						{
							CMP3 mp3;
							mp3.Filename = files->Name;
							playlist.push_back(mp3);
						}
                  else if(files->getType() == CFile::FILE_MP3_PLAYLIST)
                  {
                     std::string sPath = files->Name.substr(0, files->Name.rfind("/"));
                     std::ifstream infile;
                     char cLine[256];
                     infile.open (files->Name.c_str(), ifstream::in);
                     while (infile.good())
                     {
                        infile.getline(cLine, 255);
                        // remove CR
                        if(cLine[strlen(cLine)-1]=='\r')
                           cLine[strlen(cLine)-1]=0;
                        if(strlen(cLine) > 0 && cLine[0]!='#') 
                        {
                           std::string filename = sPath + "/" + cLine;
                           
                           unsigned int pos;
                           while((pos=filename.find("\\"))!=std::string::npos)
                              filename[pos]='/';

                           std::ifstream testfile;
                           testfile.open(filename.c_str(), ifstream::in);
                           if(testfile.good())
                           {
                              // Check for duplicates and remove (playlist has higher prio)
                              CPlayList::iterator p=playlist.begin();
                              while(p!=playlist.end())
                              {
                                 if(p->Filename == filename)
                                    playlist.erase(p);
                                 else
                                    p++;
                              }
                              CMP3 mp3;
                              mp3.Filename = filename;
                              playlist.push_back(mp3);
                           }
                           testfile.close();
                        }
                     }
                     infile.close();
                  }
               }
				}
				CLCD::getInstance()->setMode(CLCD::MODE_MP3);
				paintLCD();
				update=true;
			}
			else
			{
				rev();
			}
		}
		else if(msg==CRCInput::RC_yellow)
		{
			if(key_level==0)
			{
				stop();
				playlist.clear();
				current=-1;
				selected=0;
				update=true;
			}
			else
			{
				pause();
			}
		}
		else if(msg==CRCInput::RC_blue)
		{
			if(key_level==0)
			{
				int i=0;
				srandom((unsigned int) time(NULL));
				for(CPlayList::iterator p=playlist.begin(); p!=playlist.end() ;p++)
				{
					p->Index = random();
					if(i==current)
					{
						p->Index=-1;
					}
					i++;
				}
				sort(playlist.begin(),playlist.end(),sortByIndex);
				selected=0;
				if(m_state!=STOP)
					current=0;
				else
					current=-1;
				update=true;
			}
			else
			{
				ff();
			}

		}
		else if(msg==CRCInput::RC_help)
		{
			if(key_level==1)
			{
				key_level=0;
				paintFoot();
			}
			else
			{
				if(m_state!=STOP)
				{
					key_level=1;
					paintFoot();
				}
			}
		}
		else if( ( msg >= CRCInput::RC_1 ) && ( msg <= CRCInput::RC_9 ) && playlist.size() > 0)
		{ //numeric zap
			int x1=(g_settings.screen_EndX- g_settings.screen_StartX)/2 + g_settings.screen_StartX-50;
			int y1=(g_settings.screen_EndY- g_settings.screen_StartY)/2 + g_settings.screen_StartY;
			std::string num;
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
			CNFSSmallMenu nfsMenu;
			nfsMenu.exec(this, "");
			CLCD::getInstance()->setMode(CLCD::MODE_MP3);
			paintLCD();
			update=true;
			//pushback key if...
			//g_RCInput->postMsg( msg, data );
			//loop=false;
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

	if(m_state != STOP)
		stop();

	return(res);
}

//------------------------------------------------------------------------

void CMP3PlayerGui::hide()
{
//	printf("hide(){\n");
	if(visible)
	{
		frameBuffer->paintBackgroundBoxRel(x-ConnectLineBox_Width-1, y+title_height-1, width+ConnectLineBox_Width+2, height+2-title_height);
		clearItemID3DetailsLine();
		frameBuffer->paintBackgroundBoxRel(x, y, width, title_height);
		visible = false;
	}
//	printf("hide()}\n");
}

//------------------------------------------------------------------------

void CMP3PlayerGui::paintItem(int pos)
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
		if (playlist[liststart+pos].Title.empty())
		{
			// id3tag noch nicht geholt
			get_id3(&playlist[liststart+pos]);
			get_mp3info(&playlist[liststart+pos]);
			if(m_state!=STOP)
				usleep(100*1000);
		}
		char sNr[20];
		sprintf(sNr, "%2d : ", liststart+pos+1);
		std::string tmp=sNr;
 		std::string artist="Aritst?";
      std::string title="Title?";
      std::string album="";
      
      if (!playlist[liststart+pos].Artist.empty())
         artist=playlist[liststart+pos].Artist;
      if (!playlist[liststart+pos].Title.empty())
			title= playlist[liststart+pos].Title;
      if (!playlist[liststart+pos].Album.empty())
         album=" (" + playlist[liststart+pos].Album + ")";
      if(g_settings.mp3player_display == TITLE_ARTIST)
         tmp += title + ", " + artist + album;
      else //if(g_settings.mp3player_display == ARTIST_TITLE)
         tmp += artist + ", " + title + album;

		int w=g_Fonts->menu->getRenderWidth(playlist[liststart+pos].Duration)+5;
		g_Fonts->menu->RenderString(x+10,ypos+fheight, width-30-w, tmp.c_str(), color, fheight, true); // UTF-8
		g_Fonts->menu->RenderString(x+width-15-w,ypos+fheight, w, playlist[liststart+pos].Duration, color, fheight);

		if(liststart+pos==selected)
			paintItemID3DetailsLine(pos);
		
		if(liststart+pos==selected && m_state==STOP)
			CLCD::getInstance()->showMP3(playlist[liststart+pos].Artist, playlist[liststart+pos].Title, playlist[liststart+pos].Album);

	}
//	printf("paintItem}\n");
}

//------------------------------------------------------------------------

void CMP3PlayerGui::paintHead()
{
//	printf("paintHead{\n");
	std::string strCaption = g_Locale->getText("mp3player.head");
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

void CMP3PlayerGui::paintFoot()
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
void CMP3PlayerGui::paintInfo()
{
	if(m_state==STOP)
		frameBuffer->paintBackgroundBoxRel(x, y, width, title_height);
	else
	{
		unsigned char colFrame = COL_MENUCONTENT+6;
		frameBuffer->paintBoxRel(x,         y, width, title_height-10, colFrame);
		frameBuffer->paintBoxRel(x+2, y +2 , width-4, title_height-14, COL_MENUCONTENTSELECTED);
		char sNr[20];
		sprintf(sNr, ": %2d", current+1);
		std::string tmp=g_Locale->getText("mp3player.playing") + sNr ;
		int w=g_Fonts->menu->getRenderWidth(tmp, true); // UTF-8
		int xstart=(width-w)/2;
		if(xstart < 10)
			xstart=10;
		g_Fonts->menu->RenderString(x+xstart, y + 4 + 1*fheight, width- 20, tmp, COL_MENUCONTENTSELECTED, 0, true); // UTF-8
		if (playlist[current].Title.empty() || playlist[current].Artist.empty())
			tmp=playlist[current].Title + playlist[current].Artist;
      else if(g_settings.mp3player_display == TITLE_ARTIST)
			tmp=playlist[current].Title + " / " + playlist[current].Artist;
      else //if(g_settings.mp3player_display == ARTIST_TITLE)
			tmp=playlist[current].Artist + " / " + playlist[current].Title;

		w=g_Fonts->menu->getRenderWidth(tmp, true); // UTF-8
		xstart=(width-w)/2;
		if(xstart < 10)
			xstart=10;
		g_Fonts->menu->RenderString(x+xstart, y +4+ 2*fheight, width- 20, tmp, COL_MENUCONTENTSELECTED, 0, true); // UTF-8
		tmp = playlist[current].Bitrate + " / " + playlist[current].Samplerate + " / " + playlist[current].ChannelMode + 
			" / " + playlist[current].Layer;
		// reset so fields get painted always
      m_mp3info="";
      m_time_total="0:00";
      m_time_played="0:00";
      updateMP3Infos();
		updateTimes(true);
	}
}
//------------------------------------------------------------------------

void CMP3PlayerGui::paint()
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

//------------------------------------------------------------------------
#define BUFFER_SIZE 2100
void CMP3PlayerGui::get_mp3info(CMP3 *mp3)
{
//   printf("get_mp3info %s\n",mp3->Filename.c_str());
   FILE* in;
   struct mad_stream	Stream;
	struct mad_header	Header;
	unsigned char		InputBuffer[BUFFER_SIZE];
   int ReadSize;
   int filesize;
   in = fopen(mp3->Filename.c_str(),"r");
   if(in==NULL)
      return;

   ReadSize=fread(InputBuffer,1,BUFFER_SIZE,in);

	if(m_state!=STOP)
		usleep(15000);
	bool foundSyncmark=true;
	// Check for sync mark (some encoder produce data befor 1st frame in mp3 stream)
	if(InputBuffer[0]!=0xff || (InputBuffer[1]&0xe0)!=0xe0)
	{
      foundSyncmark=false;
		//skip to first sync mark
		int n=0,j=0;
		while((InputBuffer[n]!=0xff || (InputBuffer[n+1]&0xe0)!=0xe0) && ReadSize > 1)
		{
			n++;
			j++;
			if(n > ReadSize-2)
			{
				j--;
				n=0;
				fseek(in, -1, SEEK_CUR);
				ReadSize=fread(InputBuffer,1,BUFFER_SIZE,in);
				if(m_state!=STOP)
					usleep(15000);
			}
		}
		if(ReadSize > 1)
		{
			fseek(in, j, SEEK_SET);
			ReadSize=fread(InputBuffer,1,BUFFER_SIZE,in);
			if(m_state!=STOP)
				usleep(15000);
         foundSyncmark=true;
		}
	}
   if(foundSyncmark)
   {
//      printf("found syncmark...\n");
      mad_stream_init(&Stream);
      mad_stream_buffer(&Stream,InputBuffer,ReadSize);
      mad_header_decode(&Header,&Stream);

      mp3->VBR=false;

      if(m_state!=STOP)
         usleep(15000);
      mad_stream_finish(&Stream);
      // filesize
      fseek(in, 0, SEEK_END);
      filesize=ftell(in);
      fclose(in);

      char tmp[20];
      sprintf(tmp,"%lu kbps",Header.bitrate / 1000);
      mp3->Bitrate=tmp;
      sprintf(tmp,"%u kHz",Header.samplerate / 1000);
      mp3->Samplerate=tmp;
      sprintf(tmp, "%lu:%02lu", filesize*8/Header.bitrate/60, filesize*8/Header.bitrate%60);
      mp3->Duration=tmp;
      /* Convert the layer number to it's printed representation. */
      switch(Header.layer)
      {
         case MAD_LAYER_I:
            mp3->Layer="layer I";
            break;
         case MAD_LAYER_II:
            mp3->Layer="layer II";
            break;
         case MAD_LAYER_III:
            mp3->Layer="layer III";
            break;
      }
      /* Convert the audio mode to it's printed representation. */
      switch(Header.mode)
      {
         case MAD_MODE_SINGLE_CHANNEL:
            mp3->ChannelMode="single channel";
            break;
         case MAD_MODE_DUAL_CHANNEL:
            mp3->ChannelMode="dual channel";
            break;
         case MAD_MODE_JOINT_STEREO:
            mp3->ChannelMode="joint stereo";
            break;
         case MAD_MODE_STEREO:
            mp3->ChannelMode="normal stereo";
            break;
      }
   }
   else
   {
      mp3->Duration="?:??";
   }
}



std::string Latin1_to_UTF8(const std::string s)
{
	std::string r;
	
	for (std::string::const_iterator it = s.begin(); it != s.end(); it++)
	{
		unsigned char c = *it;
		if (c < 0x80)
			r += c;
		else
		{
			unsigned char d = 0xc0 | (c >> 6);
			r += d;
			d = 0x80 | (c & 0x3f);
			r += d;
		}
	}		
	return r;
}

//------------------------------------------------------------------------
void CMP3PlayerGui::get_id3(CMP3 *mp3)
{
	unsigned int i;
	struct id3_frame const *frame;
	id3_ucs4_t const *ucs4;
	id3_utf8_t *utf8;
	char const spaces[] = "          ";

	struct 
	{
		char const *id;
		char const *name;
	} const info[] = 
	{
		{ ID3_FRAME_TITLE,  "Title"},
		{ "TIT3",           0},	 /* Subtitle */
		{ "TCOP",           0,},  /* Copyright */
		{ "TPRO",           0,},  /* Produced */
		{ "TCOM",           "Composer"},
		{ ID3_FRAME_ARTIST, "Artist"},
		{ "TPE2",           "Orchestra"},
		{ "TPE3",           "Conductor"},
		{ "TEXT",           "Lyricist"},
		{ ID3_FRAME_ALBUM,  "Album"},
		{ ID3_FRAME_YEAR,   "Year"},
		{ ID3_FRAME_TRACK,  "Track"},
		{ "TPUB",           "Publisher"},
		{ ID3_FRAME_GENRE,  "Genre"},
		{ "TRSN",           "Station"},
		{ "TENC",           "Encoder"}
	};

	/* text information */

	struct id3_file *id3file = id3_file_open(mp3->Filename.c_str(), ID3_FILE_MODE_READONLY);
	if(id3file == 0)
		printf("error open id3 file\n");
	else
	{
		id3_tag *tag=id3_file_tag(id3file);
		if(tag)
		{
			for(i = 0; i < sizeof(info) / sizeof(info[0]); ++i)
			{
				union id3_field const *field;
				unsigned int nstrings, namelen, j;
				char const *name;

				frame = id3_tag_findframe(tag, info[i].id, 0);
				if(frame == 0)
					continue;

				field    = &frame->fields[1];
				nstrings = id3_field_getnstrings(field);

				name = info[i].name;
				namelen = name ? strlen(name) : 0;
				assert(namelen < sizeof(spaces));

				for(j = 0; j < nstrings; ++j)
				{
					ucs4 = id3_field_getstrings(field, j);
					assert(ucs4);

					if(strcmp(info[i].id, ID3_FRAME_GENRE) == 0)
						ucs4 = id3_genre_name(ucs4);

					utf8 = id3_ucs4_utf8duplicate(ucs4);
					if (utf8 == NULL)
						goto fail;

					if (j == 0 && name)
					{
						if(strcmp(name,"Title") == 0)
							mp3->Title = (char *) utf8;
						if(strcmp(name,"Artist") == 0)
							mp3->Artist = (char *) utf8;
						if(strcmp(name,"Year") == 0)
							mp3->Year = (char *) utf8;
						if(strcmp(name,"Album") == 0)
							mp3->Album = (char *) utf8;
						if(strcmp(name,"Genre") == 0)
							mp3->Genre = (char *) utf8;
						//printf("%s%s: %s\n", &spaces[namelen], name, latin1);
					}
					else
					{
						if(strcmp(info[i].id, "TCOP") == 0 || strcmp(info[i].id, "TPRO") == 0)
						{
							//printf("%s  %s %s\n", spaces, (info[i].id[1] == 'C') ? ("Copyright (C)") : ("Produced (P)"), latin1);
						}
						//else
						//printf("%s  %s\n", spaces, latin1);
					}

					free(utf8);
				}
			}

			/* comments */

			i = 0;
			while((frame = id3_tag_findframe(tag, ID3_FRAME_COMMENT, i++)))
			{
				id3_utf8_t *ptr, *newline;
				int first = 1;

				ucs4 = id3_field_getstring(&frame->fields[2]);
				assert(ucs4);

				if(*ucs4)
					continue;

				ucs4 = id3_field_getfullstring(&frame->fields[3]);
				assert(ucs4);

				utf8 = id3_ucs4_utf8duplicate(ucs4);
				if (utf8 == 0)
					goto fail;

				ptr = utf8;
				while(*ptr)
				{
					newline = (id3_utf8_t *) strchr((char*)ptr, '\n');
					if(newline)
						*newline = 0;

					if(strlen((char *)ptr) > 66)
					{
						id3_utf8_t *linebreak;

						linebreak = ptr + 66;

						while(linebreak > ptr && *linebreak != ' ')
							--linebreak;

						if(*linebreak == ' ')
						{
							if(newline)
								*newline = '\n';

							newline = linebreak;
							*newline = 0;
						}
					}

					if(first)
					{
						char const *name;
						unsigned int namelen;

						name    = "Comment";
						namelen = strlen(name);
						assert(namelen < sizeof(spaces));
						mp3->Comment = (char *) ptr;
						//printf("%s%s: %s\n", &spaces[namelen], name, ptr);
						first = 0;
					}
					else
						//printf("%s  %s\n", spaces, ptr);

						ptr += strlen((char *) ptr) + (newline ? 1 : 0);
				}

				free(utf8);
				break;
			}
			id3_tag_delete(tag);
		}
		else
			printf("error open id3 tag\n");

		id3_file_close(id3file);
	}

	if (mp3->Artist.empty() && mp3->Title.empty())
	{
		//Set from Filename
		std::string tmp = mp3->Filename.substr(mp3->Filename.rfind("/")+1);
		tmp = tmp.substr(0,tmp.length()-4);	//remove .mp3
		unsigned int i = tmp.rfind(" - ");
		if(i != std::string::npos)
		{ // Trennzeiche " - " gefunden
			mp3->Artist = tmp.substr(0, i);
			mp3->Title = tmp.substr(i+3);
		}
		else
		{
			i = tmp.rfind("-");
			if(i != std::string::npos)
			{ //Trennzeichen "-"
				mp3->Artist = tmp.substr(0, i);
				mp3->Title = tmp.substr(i+1);
			}
			else
				mp3->Title	= tmp;
		}
		mp3->Artist = Latin1_to_UTF8(mp3->Artist);
		mp3->Title = Latin1_to_UTF8(mp3->Title);
	}
	if(0)
	{
		fail:
		printf("id3: not enough memory to display tag");
	}
}

void CMP3PlayerGui::clearItemID3DetailsLine ()
{
	paintItemID3DetailsLine (-1);
}

void CMP3PlayerGui::paintItemID3DetailsLine (int pos)
{
	int xpos  = x - ConnectLineBox_Width;
	int ypos1 = y + title_height + theight+0 + pos*fheight;
	int ypos2 = y + (height-info_height);
	int ypos1a = ypos1 + (fheight/2)-2;
	int ypos2a = ypos2 + (info_height/2)-2;
	unsigned char col1 = COL_MENUCONTENT+6;
	unsigned char col2 = COL_MENUCONTENT+1;


	// Clear
	frameBuffer->paintBackgroundBoxRel(xpos,y, ConnectLineBox_Width, height - title_height);
	frameBuffer->paintBackgroundBoxRel(x,ypos2, width,info_height);

	// paint Line if detail info (and not valid list pos)
	if(playlist.size() > 0)
		if(pos >= 0)
		{
			// 1. col thick line
			frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos1, 4,fheight,     col1);
			frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos2, 4,info_height, col1);

			frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 4,ypos2a-ypos1a, col1);

			frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 12,4, col1);
			frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos2a, 12,4, col1);

			// 2. col small line
			frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos1, 1,fheight,     col2);
			frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-4, ypos2, 1,info_height, col2);

			frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 1,ypos2a-ypos1a+4, col2);

			frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-16, ypos1a, 12,1, col2);
			frameBuffer->paintBoxRel(xpos+ConnectLineBox_Width-12, ypos2a, 8,1, col2);

			// -- small Frame around infobox
			frameBuffer->paintBoxRel(x,         ypos2, width ,info_height, col1);
			// paint id3 infobox 
			frameBuffer->paintBoxRel(x+2, ypos2 +2 , width-4, info_height-4, COL_MENUCONTENTDARK);
			g_Fonts->menu->RenderString(x+10, ypos2 + 2 + 1*fheight, width- 80, playlist[selected].Title.c_str(), COL_MENUCONTENTDARK, 0, true); // UTF-8
			std::string tmp;
			if (playlist[selected].Genre.empty() || playlist[selected].Year.empty())
				tmp = playlist[selected].Genre + playlist[selected].Year;
			else
				tmp = playlist[selected].Genre + " / " + playlist[selected].Year;
			int w=g_Fonts->menu->getRenderWidth(tmp, true) + 10; // UTF-8
			g_Fonts->menu->RenderString(x+width-w-5, ypos2 + 2 + 1*fheight, w, tmp, COL_MENUCONTENTDARK, 0, true); // UTF-8
 			tmp = playlist[selected].Artist;
			if (!(playlist[selected].Album.empty()))
				tmp += " (" + playlist[selected].Album + ")";
			g_Fonts->menu->RenderString(x+10, ypos2 + 2*fheight-2, width- 20, tmp, COL_MENUCONTENTDARK, 0, true); // UTF-8
		}
}

void CMP3PlayerGui::stop()
{
	m_state=STOP;
	current=-1;
	//LCD
	paintLCD();
	//Display
	paintInfo();
	key_level=0;
	paintFoot();
	//MP3
	if(CMP3Player::getInstance()->state != CMP3Player::STOP)
		CMP3Player::getInstance()->stop();
}

void CMP3PlayerGui::pause()
{
	if(m_state==PLAY || m_state==FF || m_state==REV)
	{
		m_state=PAUSE;
		CMP3Player::getInstance()->pause();
	}
	else if(m_state==PAUSE)
   {
		m_state=PLAY;
		CMP3Player::getInstance()->pause();
	}
	paintLCD();
}

void CMP3PlayerGui::ff()
{
	if(m_state==FF)
	{
		m_state=PLAY;
		CMP3Player::getInstance()->ff();
	}
	else if(m_state==PLAY || m_state==PAUSE || m_state==REV)
	{
		m_state=FF;
		CMP3Player::getInstance()->ff();
	}
	paintLCD();
}

void CMP3PlayerGui::rev()
{
	if(m_state==REV)
	{
		m_state=PLAY;
		CMP3Player::getInstance()->rev();
	}
	else if(m_state==PLAY || m_state==PAUSE || m_state==FF)
	{
		m_state=FF;
		CMP3Player::getInstance()->rev();
	}
	paintLCD();
}


void CMP3PlayerGui::play(int pos)
{
	//printf("MP3Playlist: play %d/%d\n",pos,playlist.size());
	unsigned int old_current=current;
   unsigned int old_selected=selected;

	current=pos;
	if(g_settings.mp3player_follow)
      selected=pos;

   if(selected - liststart >= listmaxshow && g_settings.mp3player_follow)
   {
      liststart=selected;
      paint();
   }
   else if(liststart - selected < 0 && g_settings.mp3player_follow)
   {
      liststart=selected-listmaxshow+1;
      paint();
   }
   else
   {
      if(old_current - liststart >=0 && old_current - liststart < listmaxshow)
         paintItem(old_current - liststart);
      if(pos - liststart >=0 && pos - liststart < listmaxshow)
         paintItem(pos - liststart);
      if(g_settings.mp3player_follow)
      {
         if(old_selected - liststart >=0 && old_selected - liststart < listmaxshow)
            paintItem(old_selected - liststart);
      }
   }

	if (playlist[pos].Artist.empty())
	{
		// id3tag noch nicht geholt
		get_id3(&playlist[pos]);
		get_mp3info(&playlist[pos]);
	}
	m_mp3info="";
	m_time_played="0:00";
	m_time_total=playlist[current].Duration;
	m_state=PLAY;
	// Play
	CMP3Player::getInstance()->play(playlist[current].Filename.c_str()); 
	//LCD
	paintLCD();
	// Display
	paintInfo();
	key_level=1;
	paintFoot();
}

int CMP3PlayerGui::getNext()
{
	int ret=current+1;
	if((unsigned)ret+1 > playlist.size())
		ret=0;
	return ret;
}
void CMP3PlayerGui::updateMP3Infos()
{
	if(m_state!=STOP)
	{
		if(m_mp3info!=CMP3Player::getInstance()->getMp3Info())
		{
			m_mp3info=CMP3Player::getInstance()->getMp3Info();
			frameBuffer->paintBoxRel(x + 10, y+ 4 + 2*fheight, width-20, sheight, COL_MENUCONTENTSELECTED);
			int xstart = ((width - 20 - g_Fonts->infobar_small->getRenderWidth( m_mp3info ))/2)+10;
			g_Fonts->infobar_small->RenderString(x+ xstart, y+4 + 2*fheight+sheight, width- 2*xstart, m_mp3info.c_str(), COL_MENUCONTENTSELECTED);
		}
	}
}

void CMP3PlayerGui::updateTimes(bool force)
{
	if(m_state!=STOP)
	{
		bool updateTotal=false,updatePlayed=false;
		if(m_time_total!=CMP3Player::getInstance()->getTimeTotal())
		{
			m_time_total=CMP3Player::getInstance()->getTimeTotal();
			if(playlist[current].Duration!=CMP3Player::getInstance()->getTimeTotal())
			{
				playlist[current].Duration=CMP3Player::getInstance()->getTimeTotal();
			}
			updateTotal=true;
		}
		if(m_time_played!=CMP3Player::getInstance()->getTimePlayed())
		{
			m_time_played=CMP3Player::getInstance()->getTimePlayed();
			updatePlayed=true;
		}
		std::string time_tmp=m_time_played.substr(0,m_time_played.find(":")+1) + "00";
		int w1=g_Fonts->menu->getRenderWidth(" / " + m_time_total);
		int w2=g_Fonts->menu->getRenderWidth(time_tmp);

		if(updateTotal || force)
		{
			frameBuffer->paintBoxRel(x+width-w1-10, y+4, w1+4, 1*fheight, COL_MENUCONTENTSELECTED);
			g_Fonts->menu->RenderString(x+width-w1-10, y+4 + 1*fheight, w1, " / " + m_time_total,
												 COL_MENUCONTENTSELECTED);
		}
		if(updatePlayed || force || m_state==PAUSE)
		{
			frameBuffer->paintBoxRel(x+width-w1-w2-15, y+4, w2+4, 1*fheight, COL_MENUCONTENTSELECTED);
         struct timeval tv;
         gettimeofday(&tv, NULL);
			if(m_state != PAUSE || (tv.tv_sec % 2) == 0)
         {
            g_Fonts->menu->RenderString(x+width-w1-w2-11, y+4 + 1*fheight, w2, m_time_played, COL_MENUCONTENTSELECTED);
         }
		}
	}
}

void CMP3PlayerGui::paintLCD()
{
	switch(m_state)
	{
		case STOP:
			CLCD::getInstance()->showMP3Play(CLCD::MP3_STOP);
			break;
		case PLAY:
			CLCD::getInstance()->showMP3Play(CLCD::MP3_PLAY);
			CLCD::getInstance()->showMP3(playlist[current].Artist, playlist[current].Title, playlist[current].Album);
			break;
		case PAUSE:
			CLCD::getInstance()->showMP3Play(CLCD::MP3_PAUSE);
			CLCD::getInstance()->showMP3(playlist[current].Artist, playlist[current].Title, playlist[current].Album);
			break;
		case FF:
			CLCD::getInstance()->showMP3Play(CLCD::MP3_FF);
			CLCD::getInstance()->showMP3(playlist[current].Artist, playlist[current].Title, playlist[current].Album);
			break;
		case REV:
			break;
	}
}
