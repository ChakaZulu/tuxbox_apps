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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/mp3player.h>

#include <global.h>
#include <neutrino.h>

#include <driver/encoding.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <daemonc/remotecontrol.h>

#include <gui/eventlist.h>
#include <gui/color.h>
#include <gui/infoviewer.h>
#include <gui/nfs.h>

#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/menue.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>

#include <system/settings.h>

#include <id3tag.h>

#include <assert.h>
#include <algorithm>
#include <sys/time.h>
#include <fstream>

#if HAVE_DVB_API_VERSION >= 3
#include <linux/dvb/audio.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>
#define ADAP	"/dev/dvb/adapter0"
#define ADEC	ADAP "/audio0"
#define VDEC	ADAP "/video0"
#define DMX	ADAP "/demux0"
#define DVR	ADAP "/dvr0"
#endif

#ifdef ConnectLineBox_Width
#undef ConnectLineBox_Width
#endif
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
	mp3filter.addFilter("mp2");
	mp3filter.addFilter("m3u");
	filebrowser->Filter = &mp3filter;
	if(strlen(g_settings.network_nfs_mp3dir)!=0)
		Path = g_settings.network_nfs_mp3dir;
	else
		Path = "/";
}

//------------------------------------------------------------------------

CMP3PlayerGui::~CMP3PlayerGui()
{
	playlist.clear();
	g_Zapit->setStandby (false);
	g_Sectionsd->setPauseScanning (false);
	delete filebrowser;
}

//------------------------------------------------------------------------
int CMP3PlayerGui::exec(CMenuTarget* parent, const std::string & actionKey)
{
	CMP3Player::getInstance()->init();
	m_state=CMP3PlayerGui::STOP;
	current=-1;
	selected = 0;
	width = 710;
	if((g_settings.screen_EndX- g_settings.screen_StartX) < width+ConnectLineBox_Width)
		width=(g_settings.screen_EndX- g_settings.screen_StartX)-ConnectLineBox_Width;
	height = 570;
	if((g_settings.screen_EndY- g_settings.screen_StartY) < height)
		height=(g_settings.screen_EndY- g_settings.screen_StartY);
	sheight = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
	buttonHeight = std::min(25, sheight);
	theight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	fheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	title_height=fheight*2+20+sheight+4;
	info_height=fheight*2;
	listmaxshow = (height-info_height-title_height-theight-2*buttonHeight)/(fheight);
	height = theight+info_height+title_height+2*buttonHeight+listmaxshow*fheight;	// recalc height

	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-(width+ConnectLineBox_Width)) / 2) + g_settings.screen_StartX + ConnectLineBox_Width;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height)/ 2) + g_settings.screen_StartY;
	m_idletime=time(NULL);
	m_screensaver=false;

	if(parent)
	{
		parent->hide();
	}

	if(g_settings.video_Format != CControldClient::VIDEOFORMAT_4_3)
		g_Controld->setVideoFormat(CControldClient::VIDEOFORMAT_4_3);

	bool usedBackground = frameBuffer->getuseBackground();
	if (usedBackground)
		frameBuffer->saveBackgroundImage();
	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->loadBackground("radiomode.raw");
	frameBuffer->useBackground(true);
	frameBuffer->paintBackground();

	// set zapit in standby mode
	g_Zapit->setStandby(true);
	if(g_settings.audio_avs_Control == CControld::TYPE_OST)
	{
		m_vol_ost = true;
		g_settings.audio_avs_Control = CControld::TYPE_AVS;
	}
	else
		m_vol_ost = true;

	// tell neutrino we're in mp3_mode
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_mp3 );
	// remember last mode
	m_LastMode=(CNeutrinoApp::getInstance()->getLastMode() | NeutrinoMessages::norezap);

	// Stop sectionsd
	g_Sectionsd->setPauseScanning(true); 

	/*int ret =*/

	show();

	// Restore previous background
	if (usedBackground)
		frameBuffer->restoreBackgroundImage();
	frameBuffer->useBackground(usedBackground);
	frameBuffer->paintBackground();

	// Restore last mode
	//t_channel_id channel_id=CNeutrinoApp::getInstance()->channelList->getActiveChannel_ChannelID();
	//g_Zapit->zapTo_serviceID(channel_id);
	g_Zapit->setStandby(false);
	if(m_vol_ost)
	{
		g_Controld->setVolume(100, CControld::TYPE_AVS);
		g_settings.audio_avs_Control = CControld::TYPE_OST;
	}

	// Start Sectionsd
	g_Sectionsd->setPauseScanning(false);
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , m_LastMode );
	g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

	// always exit all	
	return menu_return::RETURN_EXIT_ALL;
}

//------------------------------------------------------------------------

int CMP3PlayerGui::show()
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = -1;

	CLCD::getInstance()->setMode(CLCD::MODE_MP3);

	bool loop=true;
	bool update=true;
	key_level=0;
	while(loop)
	{
		if(!m_screensaver)
		{
			updateTimes();
			updateMP3Infos();
		}

		if(CNeutrinoApp::getInstance()->getMode()!=NeutrinoMessages::mode_mp3)
		{
			// stop if mode was changed in another thread
			loop=false;
		}
		if ((m_state != CMP3PlayerGui::STOP) && 
		    (CMP3Player::getInstance()->state == CMP3Player::STOP) && 
		    (!playlist.empty()))
		{
			int next = getNext();
			if (next >= 0)
				play(next);
			else
				stop();
		}

		if (update)
		{
		  /* 
		   * let's try without hide(); to save some painting and prevent flickering
		   */
		  /*
			hide();
		  */
			update=false;
			paint();
		}
		g_RCInput->getMsg( &msg, &data, 10 ); // 1 sec timeout to update play/stop state display

		if( msg == CRCInput::RC_timeout  || msg == NeutrinoMessages::EVT_TIMER)
		{
			int timeout = time(NULL) - m_idletime;
			int screensaver_timeout = atoi(g_settings.mp3player_screensaver);
			if(screensaver_timeout !=0 && timeout > screensaver_timeout*60 && !m_screensaver)
				screensaver(true);
		}
		else
		{
			m_idletime=time(NULL);
			if(m_screensaver)
			{
				screensaver(false);
			}
		}
		if( msg == CRCInput::RC_timeout)
		{
			// nothing
		}
		else if( msg == CRCInput::RC_home)
		{ 
			if (m_state != CMP3PlayerGui::STOP)
				stop();        
			else
				loop=false;
		}
		else if( msg == CRCInput::RC_left)
		{
			if(key_level==1)
			{
				if(current==-1)
					stop();
				else if(current-1 > 0)
					play(current-1);
				else
					play(0);
			}
			else
			{
				if(selected >0 )
				{
					if ((int(selected)-int(listmaxshow))<0)
						selected=playlist.size()-1;
					else
						selected -= listmaxshow;
					liststart = (selected/listmaxshow)*listmaxshow;
					update=true;
				}
			}

		}
		else if( msg == CRCInput::RC_right)
		{
			if(key_level==1)
			{
				int next = getNext();
				if(next>=0)
					play(next);
				else
					stop();
			}
			else
			{
				if(selected!=playlist.size()-1 && !playlist.empty())
				{
					selected+=listmaxshow;
					if (selected>playlist.size()-1)
						selected=0;
					liststart = (selected/listmaxshow)*listmaxshow;
					update=true;
				}
			}
		}
		else if( msg == CRCInput::RC_up && !playlist.empty())
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
		else if( msg == CRCInput::RC_down && !playlist.empty() > 0)
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
		else if( msg == CRCInput::RC_ok && !playlist.empty())
		{
			// OK button
			play(selected);
		}
		else if(msg==CRCInput::RC_red )
		{
			if(key_level==0)
			{
				if (!playlist.empty())
				{
					CPlayList::iterator p = playlist.begin()+selected;
					playlist.erase(p);
					if((int)selected==current)
					{
						current--;
						//stop(); // Stop if song is deleted, next song will be startet automat.
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
							std::string sPath = files->Name.substr(0, files->Name.rfind('/'));
							std::ifstream infile;
							char cLine[256];
							infile.open(files->Name.c_str(), std::ifstream::in);
							while (infile.good())
							{
								infile.getline(cLine, 255);
								// remove CR
								if(cLine[strlen(cLine)-1]=='\r')
									cLine[strlen(cLine)-1]=0;
								if(strlen(cLine) > 0 && cLine[0]!='#') 
								{
									std::string filename = sPath;
									filename += '/';
									filename += cLine;
                           
									unsigned int pos;
									while((pos=filename.find('\\'))!=std::string::npos)
										filename[pos]='/';

									std::ifstream testfile;
									testfile.open(filename.c_str(), std::ifstream::in);
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
				//stop();
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
				if(m_state!=CMP3PlayerGui::STOP)
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
				if(m_state!=CMP3PlayerGui::STOP)
				{
					key_level=1;
					paintFoot();
				}
			}
		}
		else if ((msg >= CRCInput::RC_1) && (msg <= CRCInput::RC_9) && !(playlist.empty()))
		{ //numeric zap
			int x1=(g_settings.screen_EndX- g_settings.screen_StartX)/2 + g_settings.screen_StartX-50;
			int y1=(g_settings.screen_EndY- g_settings.screen_StartY)/2 + g_settings.screen_StartY;
			int val=0;
			char str[11];
			do
			{
				val = val * 10 + CRCInput::getNumericValue(msg);
				sprintf(str,"%d",val);
				int w = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getRenderWidth(str);
				int h = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getHeight();
				frameBuffer->paintBoxRel(x1 - 7, y1 - h - 5, w + 14, h + 10, COL_MENUCONTENT_PLUS_6);
				frameBuffer->paintBoxRel(x1 - 4, y1 - h - 3, w +  8, h +  6, COL_MENUCONTENTSELECTED_PLUS_0);
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->RenderString(x1,y1,w+1,str,COL_MENUCONTENTSELECTED,0);
				g_RCInput->getMsg( &msg, &data, 100 ); 
			} while (g_RCInput->isNumeric(msg) && val < 1000000);
			if (msg == CRCInput::RC_ok)
				selected = std::min((int)playlist.size(), val) - 1;
			update = true;
		}
		else if(msg == CRCInput::RC_0)
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
		else if(msg == NeutrinoMessages::EVT_TIMER)
		{
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
		}
		else
		{
			if( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
			}
			// update mute icon
			paintHead();
		}
	}
	hide();

	if(m_state != CMP3PlayerGui::STOP)
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
	int ypos = y + title_height + theight + pos*fheight;
	uint8_t    color;
	fb_pixel_t bgcolor;

	if ((pos + liststart) == selected)
	{
		if ((pos + liststart) == (unsigned)current)
		{
			color   = COL_MENUCONTENTSELECTED + 2;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_2;
		}
		else
		{
			color   = COL_MENUCONTENTSELECTED;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		}
	}
	else
		if (((pos + liststart) < playlist.size()) && (pos & 1))
		{
			if ((pos + liststart) == (unsigned)current)
			{
				color   = COL_MENUCONTENTDARK + 2;
				bgcolor = COL_MENUCONTENTDARK_PLUS_2;
			}
			else
			{
				color   = COL_MENUCONTENTDARK;
				bgcolor = COL_MENUCONTENTDARK_PLUS_0;
			}
		}
		else
		{
			if ((pos + liststart) == (unsigned)current)
			{
				color   = COL_MENUCONTENT + 2;
				bgcolor = COL_MENUCONTENT_PLUS_2;
			}
			else
			{
				color   = COL_MENUCONTENT;
				bgcolor = COL_MENUCONTENT_PLUS_0;
			}
		}

	frameBuffer->paintBoxRel(x, ypos, width - 15, fheight, bgcolor);

	if ((pos + liststart) < playlist.size())
	{
		if (playlist[pos + liststart].Title.empty())
		{
			// id3tag noch nicht geholt
			get_id3(&playlist[pos + liststart]);
			get_mp3info(&playlist[pos + liststart]);
			if(m_state!=CMP3PlayerGui::STOP && !g_settings.mp3player_highprio)
				usleep(100*1000);
		}
		char sNr[20];
		sprintf(sNr, "%2d : ", pos + liststart + 1);
		std::string tmp=sNr;
 		std::string artist="Artist?";
		std::string title="Title?";
		
		if (!playlist[pos + liststart].Artist.empty())
			artist = playlist[pos + liststart].Artist;
		if (!playlist[pos + liststart].Title.empty())
			title = playlist[pos + liststart].Title;
		if(g_settings.mp3player_display == TITLE_ARTIST)
		{
			tmp += title;
			tmp += ", ";
			tmp += artist;
		}
		else //if(g_settings.mp3player_display == ARTIST_TITLE)
		{
			tmp += artist;
			tmp += ", ";
			tmp += title;
		}

		if (!playlist[pos + liststart].Album.empty())
		{
			tmp += " (";
			tmp += playlist[pos + liststart].Album;
			tmp += ')';
		}
		
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(playlist[pos + liststart].Duration)+5;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10,ypos+fheight, width-30-w, tmp, color, fheight, true); // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+width-15-w,ypos+fheight, w, playlist[pos + liststart].Duration, color, fheight);
		
		if ((pos + liststart) == selected)
		{
			paintItemID3DetailsLine(pos);
			if (m_state == CMP3PlayerGui::STOP)
				CLCD::getInstance()->showMP3(playlist[pos + liststart].Artist, playlist[pos + liststart].Title, playlist[pos + liststart].Album);
		}
		
	}
}

//------------------------------------------------------------------------

void CMP3PlayerGui::paintHead()
{
//	printf("paintHead{\n");
	std::string strCaption = g_Locale->getText("mp3player.head");
	frameBuffer->paintBoxRel(x,y+title_height, width,theight, COL_MENUHEAD_PLUS_0);
	frameBuffer->paintIcon("mp3.raw",x+7,y+title_height+10);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+35,y+theight+title_height+0, width- 45, strCaption, COL_MENUHEAD, 0, true); // UTF-8
	int ypos=y+title_height;
	if(theight > 26)
		ypos = (theight-26) / 2 + y + title_height;
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_DBOX, x+ width- 30, ypos );
	if( CNeutrinoApp::getInstance()->isMuted() )
	{
		int xpos=x+width-75;
		ypos=y+title_height;
		if(theight > 32)
			ypos = (theight-32) / 2 + y + title_height;
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_MUTE, xpos, ypos);
	}
//	printf("paintHead}\n");
}

//------------------------------------------------------------------------
const struct button_label MP3PlayerButtons[2][4] =
{
	{
		{ NEUTRINO_ICON_BUTTON_RED   , "mp3player.stop"        },
		{ NEUTRINO_ICON_BUTTON_GREEN , "mp3player.rewind"      },
		{ NEUTRINO_ICON_BUTTON_YELLOW, "mp3player.pause"       },
		{ NEUTRINO_ICON_BUTTON_BLUE  , "mp3player.fastforward" },
	},
	{
		{ NEUTRINO_ICON_BUTTON_RED   , "mp3player.delete"    },
		{ NEUTRINO_ICON_BUTTON_GREEN , "mp3player.add"       },
		{ NEUTRINO_ICON_BUTTON_YELLOW, "mp3player.deleteall" },
		{ NEUTRINO_ICON_BUTTON_BLUE  , "mp3player.shuffle"   }
	}
};

void CMP3PlayerGui::paintFoot()
{
//	printf("paintFoot{\n");
	if(m_state==CMP3PlayerGui::STOP) // insurance
		key_level=0;
	int ButtonWidth = (width-20) / 4;
	int ButtonWidth2 = (width-50) / 2;
	frameBuffer->paintBoxRel(x,y+(height-info_height-2*buttonHeight), width,2*buttonHeight, COL_MENUHEAD_PLUS_0);
	frameBuffer->paintHLine(x, x+width,  y+(height-info_height-2*buttonHeight), COL_INFOBAR_SHADOW_PLUS_0);

	if (!(playlist.empty()))
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, x + 1* ButtonWidth2 + 25, y+(height-info_height-buttonHeight)-3);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x + 1 * ButtonWidth2 + 53 , y+(height-info_height-buttonHeight)+24 - 4, 
						     ButtonWidth2- 28, g_Locale->getText("mp3player.play"), COL_INFOBAR, 0, true); // UTF-8
	}
	if(m_state!=CMP3PlayerGui::STOP)
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x+ 0* ButtonWidth + 25, y+(height-info_height-buttonHeight)-3);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+ 0* ButtonWidth +53 , y+(height-info_height-buttonHeight)+24 - 4, 
						     ButtonWidth2- 28, g_Locale->getText("mp3player.keylevel"), COL_INFOBAR, 0, true); // UTF-8
	}

	if (key_level == 0)
	{
		if (playlist.empty())
			::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + ButtonWidth + 10, y + (height - info_height - 2 * buttonHeight) + 4, ButtonWidth, 1, &(MP3PlayerButtons[1][1]));
		else
			::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, y + (height - info_height - 2 * buttonHeight) + 4, ButtonWidth, 4, MP3PlayerButtons[1]);
	}
	else
	{	
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, y + (height - info_height - 2 * buttonHeight) + 4, ButtonWidth, 4, MP3PlayerButtons[0]);
	}
//	printf("paintFoot}\n");
}
//------------------------------------------------------------------------
void CMP3PlayerGui::paintInfo()
{
	if(m_state==CMP3PlayerGui::STOP)
		frameBuffer->paintBackgroundBoxRel(x, y, width, title_height);
	else
	{
		frameBuffer->paintBoxRel(x,         y, width, title_height-10, COL_MENUCONTENT_PLUS_6);
		frameBuffer->paintBoxRel(x+2, y +2 , width-4, title_height-14, COL_MENUCONTENTSELECTED_PLUS_0);
		char sNr[20];
		sprintf(sNr, ": %2d", current+1);
		std::string tmp = g_Locale->getText("mp3player.playing");
		tmp += sNr ;
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp, true); // UTF-8
		int xstart=(width-w)/2;
		if(xstart < 10)
			xstart=10;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+xstart, y + 4 + 1*fheight, width- 20, tmp, COL_MENUCONTENTSELECTED, 0, true); // UTF-8
		if (curr_mp3.Title.empty())
			tmp = curr_mp3.Artist;
		else if (curr_mp3.Artist.empty())
			tmp = curr_mp3.Title;
		else if (g_settings.mp3player_display == TITLE_ARTIST)
		{
			tmp = curr_mp3.Title;
			tmp += " / ";
			tmp += curr_mp3.Artist;
		}
		else //if(g_settings.mp3player_display == ARTIST_TITLE)
		{
			tmp = curr_mp3.Artist;
			tmp += " / ";
			tmp += curr_mp3.Title;
		}

		w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp, true); // UTF-8
		xstart=(width-w)/2;
		if(xstart < 10)
			xstart=10;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+xstart, y +4+ 2*fheight, width- 20, tmp, COL_MENUCONTENTSELECTED, 0, true); // UTF-8
#ifdef INCLUDE_UNUSED_STUFF
		tmp = curr_mp3.Bitrate + " / " + curr_mp3.Samplerate + " / " + curr_mp3.ChannelMode + " / " + curr_mp3.Layer;
#endif
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
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT_PLUS_1);

	int sbc= ((playlist.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT_PLUS_3);

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

	if(m_state!=CMP3PlayerGui::STOP && !g_settings.mp3player_highprio)
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
				if(m_state!=CMP3PlayerGui::STOP)
					usleep(15000 && !g_settings.mp3player_highprio);
			}
		}
		if(ReadSize > 1)
		{
			fseek(in, j, SEEK_SET);
			ReadSize=fread(InputBuffer,1,BUFFER_SIZE,in);
			if(m_state!=CMP3PlayerGui::STOP)
				usleep(15000 && !g_settings.mp3player_highprio);
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

		if(m_state!=CMP3PlayerGui::STOP && !g_settings.mp3player_highprio)
			usleep(15000);
		mad_stream_finish(&Stream);
		// filesize
		fseek(in, 0, SEEK_END);
		filesize=ftell(in);
		fclose(in);

		char tmp[20];
#ifdef INCLUDE_UNUSED_STUFF
		sprintf(tmp,"%lu kbps",Header.bitrate / 1000);
		mp3->Bitrate=tmp;
		sprintf(tmp,"%u kHz",Header.samplerate / 1000);
		mp3->Samplerate=tmp;
#endif
		sprintf(tmp, "%lu:%02lu", filesize*8/Header.bitrate/60, filesize*8/Header.bitrate%60);
		mp3->Duration=tmp;
#ifdef INCLUDE_UNUSED_STUFF
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
#endif
	}
	else
	{
		mp3->Duration="?:??";
	}
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

#ifdef INCLUDE_UNUSED_STUFF
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
#endif
			id3_tag_delete(tag);
		}
		else
			printf("error open id3 tag\n");

		id3_file_close(id3file);
	}

	if (mp3->Artist.empty() && mp3->Title.empty())
	{
		//Set from Filename
		std::string tmp = mp3->Filename.substr(mp3->Filename.rfind('/')+1);
		tmp = tmp.substr(0,tmp.length()-4);	//remove .mp3
		unsigned int i = tmp.rfind(" - ");
		if(i != std::string::npos)
		{ // Trennzeiche " - " gefunden
			mp3->Artist = tmp.substr(0, i);
			mp3->Title = tmp.substr(i+3);
		}
		else
		{
			i = tmp.rfind('-');
			if(i != std::string::npos)
			{ //Trennzeichen "-"
				mp3->Artist = tmp.substr(0, i);
				mp3->Title = tmp.substr(i+1);
			}
			else
				mp3->Title	= tmp;
		}
#ifdef FILESYSTEM_IS_ISO8859_1_ENCODED
		mp3->Artist = Latin1_to_UTF8(mp3->Artist);
		mp3->Title = Latin1_to_UTF8(mp3->Title);
#endif
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
	fb_pixel_t col1 = COL_MENUCONTENT_PLUS_6;
	fb_pixel_t col2 = COL_MENUCONTENT_PLUS_1;


	// Clear
	frameBuffer->paintBackgroundBoxRel(xpos - 1, y + title_height, ConnectLineBox_Width + 1, height - title_height);

	// paint Line if detail info (and not valid list pos)
	if (!playlist.empty() && (pos >= 0))
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
		frameBuffer->paintBoxRel(x, ypos2, width, 2, col1);
		frameBuffer->paintBoxRel(x, ypos2 + 2, 2, info_height - 4, col1);
		frameBuffer->paintBoxRel(x + width - 2, ypos2 + 2, 2, info_height - 4, col1);
		frameBuffer->paintBoxRel(x, ypos2 + info_height - 2, width, 2, col1);
//		frameBuffer->paintBoxRel(x, ypos2, width, info_height, col1);

		// paint id3 infobox 
		frameBuffer->paintBoxRel(x+2, ypos2 +2 , width-4, info_height-4, COL_MENUCONTENTDARK_PLUS_0);
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10, ypos2 + 2 + 1*fheight, width- 80, playlist[selected].Title, COL_MENUCONTENTDARK, 0, true); // UTF-8
		std::string tmp;
		if (playlist[selected].Genre.empty())
			tmp = playlist[selected].Year;
		else if (playlist[selected].Year.empty())
			tmp = playlist[selected].Genre;
		else
		{
			tmp = playlist[selected].Genre;
			tmp += " / ";
			tmp += playlist[selected].Year;
		}
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp, true) + 10; // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+width-w-5, ypos2 + 2 + 1*fheight, w, tmp, COL_MENUCONTENTDARK, 0, true); // UTF-8
		tmp = playlist[selected].Artist;
		if (!(playlist[selected].Album.empty()))
		{
			tmp += " (";
			tmp += playlist[selected].Album;
			tmp += ')';
		}
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10, ypos2 + 2*fheight-2, width- 20, tmp, COL_MENUCONTENTDARK, 0, true); // UTF-8
	}
	else
	{
		frameBuffer->paintBackgroundBoxRel(x, ypos2, width, info_height);
	}
}

void CMP3PlayerGui::stop()
{
	m_state=CMP3PlayerGui::STOP;
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
	if(m_state==CMP3PlayerGui::PLAY || m_state==CMP3PlayerGui::FF || m_state==CMP3PlayerGui::REV)
	{
		m_state=CMP3PlayerGui::PAUSE;
		CMP3Player::getInstance()->pause();
	}
	else if(m_state==CMP3PlayerGui::PAUSE)
	{
		m_state=CMP3PlayerGui::PLAY;
		CMP3Player::getInstance()->pause();
	}
	paintLCD();
}

void CMP3PlayerGui::ff()
{
	if(m_state==CMP3PlayerGui::FF)
	{
		m_state=CMP3PlayerGui::PLAY;
		CMP3Player::getInstance()->ff();
	}
	else if(m_state==CMP3PlayerGui::PLAY || m_state==CMP3PlayerGui::PAUSE || m_state==CMP3PlayerGui::REV)
	{
		m_state=CMP3PlayerGui::FF;
		CMP3Player::getInstance()->ff();
	}
	paintLCD();
}

void CMP3PlayerGui::rev()
{
	if(m_state==CMP3PlayerGui::REV)
	{
		m_state=CMP3PlayerGui::PLAY;
		CMP3Player::getInstance()->rev();
	}
	else if(m_state==CMP3PlayerGui::PLAY || m_state==CMP3PlayerGui::PAUSE || m_state==CMP3PlayerGui::FF)
	{
		m_state=CMP3PlayerGui::FF;
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
		if(!m_screensaver)
			paint();
	}
	else if(liststart - selected < 0 && g_settings.mp3player_follow)
	{
		liststart=selected-listmaxshow+1;
		if(!m_screensaver)
			paint();
	}
	else
	{
		if(old_current - liststart >=0 && old_current - liststart < listmaxshow)
		{
			if(!m_screensaver)
				paintItem(old_current - liststart);
		}
		if(pos - liststart >=0 && pos - liststart < listmaxshow)
		{
			if(!m_screensaver)
				paintItem(pos - liststart);
		}
		if(g_settings.mp3player_follow)
		{
			if(old_selected - liststart >=0 && old_selected - liststart < listmaxshow)
				if(!m_screensaver)
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
	m_state=CMP3PlayerGui::PLAY;
	curr_mp3 = playlist[current];
	// Play
	CMP3Player::getInstance()->play(curr_mp3.Filename.c_str(), g_settings.mp3player_highprio==1); 
	//LCD
	paintLCD();
	// Display
	if(!m_screensaver)
		paintInfo();
	key_level=1;
	if(!m_screensaver)
		paintFoot();
}

int CMP3PlayerGui::getNext()
{
	int ret=current+1;
	if(playlist.empty())
		return -1;
	if((unsigned)ret+1 > playlist.size())
		ret=0;
	return ret;
}
void CMP3PlayerGui::updateMP3Infos()
{
	if(m_state!=CMP3PlayerGui::STOP)
	{
		if(m_mp3info!=CMP3Player::getInstance()->getMp3Info())
		{
			m_mp3info=CMP3Player::getInstance()->getMp3Info();
			frameBuffer->paintBoxRel(x + 10, y+ 4 + 2*fheight, width-20, sheight, COL_MENUCONTENTSELECTED_PLUS_0);
			int xstart = ((width - 20 - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth( m_mp3info ))/2)+10;
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+ xstart, y+4 + 2*fheight+sheight, width- 2*xstart, m_mp3info, COL_MENUCONTENTSELECTED);
		}
	}
}

void CMP3PlayerGui::updateTimes(const bool force)
{
	if (m_state != CMP3PlayerGui::STOP)
	{
		bool updateTotal = force;
		bool updatePlayed = force;

		if (m_time_total != CMP3Player::getInstance()->getTimeTotal())
		{
			m_time_total = CMP3Player::getInstance()->getTimeTotal();
			if (curr_mp3.Duration != CMP3Player::getInstance()->getTimeTotal())
			{
				curr_mp3.Duration = CMP3Player::getInstance()->getTimeTotal();
				if(current >=0)
					playlist[current].Duration = CMP3Player::getInstance()->getTimeTotal();
			}
			updateTotal = true;
		}
		if ((m_time_played != CMP3Player::getInstance()->getTimePlayed()))
		{
			m_time_played = CMP3Player::getInstance()->getTimePlayed();
			updatePlayed = true;
		}
		std::string time_tmp = m_time_played.substr(0,m_time_played.find(':')+1);
		time_tmp += "00";

		int w1 = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(" / " + m_time_total);
		int w2 = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(time_tmp);

		if (updateTotal)
		{
			frameBuffer->paintBoxRel(x+width-w1-10, y+4, w1+4, fheight, COL_MENUCONTENTSELECTED_PLUS_0);
			g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+width-w1-10, y+4 + fheight, w1, " / " + m_time_total, COL_MENUCONTENTSELECTED);
		}
		if (updatePlayed || (m_state == CMP3PlayerGui::PAUSE))
		{
			frameBuffer->paintBoxRel(x+width-w1-w2-15, y+4, w2+4, fheight, COL_MENUCONTENTSELECTED_PLUS_0);
			struct timeval tv;
			gettimeofday(&tv, NULL);
			if ((m_state != CMP3PlayerGui::PAUSE) || (tv.tv_sec & 1))
			{
				g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+width-w1-w2-11, y+4 + fheight, w2, m_time_played, COL_MENUCONTENTSELECTED);
			}
		}
	}
}

void CMP3PlayerGui::paintLCD()
{
	switch(m_state)
	{
	case CMP3PlayerGui::STOP:
		CLCD::getInstance()->showMP3Play(CLCD::MP3_STOP);
		break;
	case CMP3PlayerGui::PLAY:
		CLCD::getInstance()->showMP3Play(CLCD::MP3_PLAY);
		CLCD::getInstance()->showMP3(curr_mp3.Artist, curr_mp3.Title, curr_mp3.Album);
		break;
	case CMP3PlayerGui::PAUSE:
		CLCD::getInstance()->showMP3Play(CLCD::MP3_PAUSE);
		CLCD::getInstance()->showMP3(curr_mp3.Artist, curr_mp3.Title, curr_mp3.Album);
		break;
	case CMP3PlayerGui::FF:
		CLCD::getInstance()->showMP3Play(CLCD::MP3_FF);
		CLCD::getInstance()->showMP3(curr_mp3.Artist, curr_mp3.Title, curr_mp3.Album);
		break;
	case CMP3PlayerGui::REV:
		break;
	}
}

void CMP3PlayerGui::screensaver(bool on)
{
	if(on)
	{
		m_screensaver=true;
		frameBuffer->ClearFrameBuffer();
	}
	else
	{
		m_screensaver=false;
		frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
		frameBuffer->loadBackground("radiomode.raw");
		frameBuffer->useBackground(true);
		frameBuffer->paintBackground();
		paint();
		m_idletime=time(NULL);
	}
}
