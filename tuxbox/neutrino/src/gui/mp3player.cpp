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
bool sortByIndex (const CAudiofile& a, const CAudiofile& b)
{
	return a.Index < b.Index ;
}
//------------------------------------------------------------------------

CMP3PlayerGui::CMP3PlayerGui()
{
	frameBuffer = CFrameBuffer::getInstance();

	visible = false;
	selected = 0;
	m_metainfo = "";

	filebrowser = new CFileBrowser();
	filebrowser->Multi_Select = true;
	filebrowser->Dirs_Selectable = true;
	audiofilefilter.addFilter("mp3");
	audiofilefilter.addFilter("m2a");
	audiofilefilter.addFilter("mpa");
	audiofilefilter.addFilter("mp2");
	audiofilefilter.addFilter("m3u");
	audiofilefilter.addFilter("url");
	audiofilefilter.addFilter("ogg");
	filebrowser->Filter = &audiofilefilter;
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
	CAudioPlayer::getInstance()->init();
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
		m_vol_ost = false;

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
			updateMetaData();
		}

		if(CNeutrinoApp::getInstance()->getMode()!=NeutrinoMessages::mode_mp3)
		{
			// stop if mode was changed in another thread
			loop=false;
		}
		if ((m_state != CMP3PlayerGui::STOP) && 
		    (CAudioPlayer::getInstance()->getState() == CBaseDec::STOP) && 
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
						if(files->getType() == CFile::FILE_MP3 )
						{
							CAudiofile mp3;
							mp3.Filename = files->Name;
							mp3.FileType = CFile::FILE_MP3;
							playlist.push_back(mp3);
						}
						if(files->getType() == CFile::FILE_OGG)
						{
							CAudiofile mp3;
							mp3.Filename = files->Name;
							mp3.FileType = CFile::FILE_OGG;
							playlist.push_back(mp3);
						}
						if(files->getType() == CFile::STREAM_MP3)
						{
							CAudiofile mp3;
							mp3.FileType = CFile::STREAM_MP3;
							mp3.Filename = files->Name;
							mp3.Artist = "Shoutcast";
							std::string tmp = mp3.Filename.substr(mp3.Filename.rfind('/')+1);
							tmp = tmp.substr(0,tmp.length()-4);	//remove .url
							mp3.Title = tmp;
							char url[80];
							FILE* f=fopen(files->Name.c_str(), "r");
							if(f!=NULL)
							{
								fgets(url, 80, f);
								if(url[strlen(url)-1] == '\n') url[strlen(url)-1]=0;
								if(url[strlen(url)-1] == '\r') url[strlen(url)-1]=0;
								mp3.Album = url;
								playlist.push_back(mp3);
								fclose(f);
							}
						}
						else if(files->getType() == CFile::FILE_PLAYLIST)
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
										CAudiofile mp3;
										mp3.FileType = CFile::FILE_MP3;
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
				if(curr_audiofile.FileType != CFile::STREAM_MP3)
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
				if(curr_audiofile.FileType != CFile::STREAM_MP3)
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
			GetMetaData(&playlist[pos + liststart]);
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
		
		char dura[9];
		snprintf(dura, 8, "%ld:%02ld", playlist[pos + liststart].Duration / 60, playlist[pos + liststart].Duration % 60);
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(dura)+5;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10,ypos+fheight, width-30-w, tmp, color, fheight, true); // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+width-15-w,ypos+fheight, w, dura, color, fheight);
		
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
	std::string strCaption = g_Locale->getText(LOCALE_MP3PLAYER_HEAD);
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
		{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_MP3PLAYER_STOP        },
		{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_MP3PLAYER_REWIND      },
		{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_MP3PLAYER_PAUSE       },
		{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_MP3PLAYER_FASTFORWARD },
	},
	{
		{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_MP3PLAYER_DELETE      },
		{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_MP3PLAYER_ADD         },
		{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_MP3PLAYER_DELETEALL   },
		{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_MP3PLAYER_SHUFFLE     }
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
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x + 1 * ButtonWidth2 + 53 , y+(height-info_height-buttonHeight)+24 - 4, ButtonWidth2- 28, g_Locale->getText(LOCALE_MP3PLAYER_PLAY), COL_INFOBAR, 0, true); // UTF-8
	}
	if(m_state!=CMP3PlayerGui::STOP)
	{
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x+ 0* ButtonWidth + 25, y+(height-info_height-buttonHeight)-3);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+ 0* ButtonWidth +53 , y+(height-info_height-buttonHeight)+24 - 4, ButtonWidth2- 28, g_Locale->getText(LOCALE_MP3PLAYER_KEYLEVEL), COL_INFOBAR, 0, true); // UTF-8
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
		std::string tmp = g_Locale->getText(LOCALE_MP3PLAYER_PLAYING);
		tmp += sNr ;
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp, true); // UTF-8
		int xstart=(width-w)/2;
		if(xstart < 10)
			xstart=10;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+xstart, y + 4 + 1*fheight, width- 20, tmp, COL_MENUCONTENTSELECTED, 0, true); // UTF-8
		if (curr_audiofile.Title.empty())
			tmp = curr_audiofile.Artist;
		else if (curr_audiofile.Artist.empty())
			tmp = curr_audiofile.Title;
		else if (g_settings.mp3player_display == TITLE_ARTIST)
		{
			tmp = curr_audiofile.Title;
			tmp += " / ";
			tmp += curr_audiofile.Artist;
		}
		else //if(g_settings.mp3player_display == ARTIST_TITLE)
		{
			tmp = curr_audiofile.Artist;
			tmp += " / ";
			tmp += curr_audiofile.Title;
		}

		w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp, true); // UTF-8
		xstart=(width-w)/2;
		if(xstart < 10)
			xstart=10;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+xstart, y +4+ 2*fheight, width- 20, tmp, COL_MENUCONTENTSELECTED, 0, true); // UTF-8
#ifdef INCLUDE_UNUSED_STUFF
		tmp = curr_audiofile.Bitrate + " / " + curr_audiofile.Samplerate + " / " + curr_audiofile.ChannelMode + " / " + curr_audiofile.Layer;
#endif
		// reset so fields get painted always
		m_metainfo="";
		m_time_total=0;
		m_time_played=0;
		updateMetaData();
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
	if(CAudioPlayer::getInstance()->getState() != CBaseDec::STOP)
		CAudioPlayer::getInstance()->stop();
}

void CMP3PlayerGui::pause()
{
	if(m_state==CMP3PlayerGui::PLAY || m_state==CMP3PlayerGui::FF || m_state==CMP3PlayerGui::REV)
	{
		m_state=CMP3PlayerGui::PAUSE;
		CAudioPlayer::getInstance()->pause();
	}
	else if(m_state==CMP3PlayerGui::PAUSE)
	{
		m_state=CMP3PlayerGui::PLAY;
		CAudioPlayer::getInstance()->pause();
	}
	paintLCD();
}

void CMP3PlayerGui::ff()
{
	if(m_state==CMP3PlayerGui::FF)
	{
		m_state=CMP3PlayerGui::PLAY;
		CAudioPlayer::getInstance()->ff();
	}
	else if(m_state==CMP3PlayerGui::PLAY || m_state==CMP3PlayerGui::PAUSE || m_state==CMP3PlayerGui::REV)
	{
		m_state=CMP3PlayerGui::FF;
		CAudioPlayer::getInstance()->ff();
	}
	paintLCD();
}

void CMP3PlayerGui::rev()
{
	if(m_state==CMP3PlayerGui::REV)
	{
		m_state=CMP3PlayerGui::PLAY;
		CAudioPlayer::getInstance()->rev();
	}
	else if(m_state==CMP3PlayerGui::PLAY || m_state==CMP3PlayerGui::PAUSE || m_state==CMP3PlayerGui::FF)
	{
		m_state=CMP3PlayerGui::FF;
		CAudioPlayer::getInstance()->rev();
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

	if (playlist[pos].Title.empty())
	{
		// id3tag noch nicht geholt
		GetMetaData(&playlist[pos]);
	}
	m_metainfo="";
	m_time_played=0;
	m_time_total=playlist[current].Duration;
	m_state=CMP3PlayerGui::PLAY;
	curr_audiofile = playlist[current];
	// Play
	CAudioPlayer::getInstance()->play(curr_audiofile.Filename.c_str(), g_settings.mp3player_highprio==1); 
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
void CMP3PlayerGui::updateMetaData()
{
	bool updateMeta=false;
	bool updateLcd=false;
	bool updateScreen=false;

	if(m_state!=CMP3PlayerGui::STOP)
	{
		CAudioMetaData metaData = CAudioPlayer::getInstance()->getMetaData();
		if(metaData.changed || m_metainfo.empty())
		{
			std::string info = metaData.type_info;
			if(metaData.bitrate > 0)
			{
				info += " / " ;
				if(metaData.vbr)
					info += "VBR ";
				char rate[31];
				snprintf(rate, 30, "%ukbs", metaData.bitrate/1000);
				info += rate;
			}
			if(metaData.samplerate > 0)
			{
				info += " / " ;
				char rate[31];
				snprintf(rate, 30, "%.1fKHz", (float)metaData.samplerate/1000);
				info += rate;
			}
			if(m_metainfo!=info)
			{
				m_metainfo=info;
				updateMeta=true;
			}
			
			if (!metaData.artist.empty()  &&
				 metaData.artist != curr_audiofile.Artist)
			{
				curr_audiofile.Artist = metaData.artist;
				updateScreen=true;
				updateLcd=true;
			}
			if (!metaData.title.empty() &&
				 metaData.title != curr_audiofile.Title)
			{
				curr_audiofile.Title = metaData.title;
				updateScreen=true;
				updateLcd=true;
			}
			if (!metaData.sc_station.empty()  &&
				 metaData.sc_station != curr_audiofile.Album)
			{
				curr_audiofile.Album = metaData.sc_station;
				updateLcd=true;
			}
		}
		if (CAudioPlayer::getInstance()->getScBuffered()!=0)
		{
			char perc[4];
			snprintf(perc,3,"%d",(int)CAudioPlayer::getInstance()->getScBuffered() * 100 / 65535);
			curr_audiofile.Album = perc;
			curr_audiofile.Album += "% ";
			curr_audiofile.Album += metaData.sc_station;
			updateLcd=true;
		}
		if(updateLcd)
			paintLCD();
		if(updateScreen)
			paintInfo();
		if(updateMeta || updateScreen)
		{
			frameBuffer->paintBoxRel(x + 10, y+ 4 + 2*fheight, width-20, sheight, COL_MENUCONTENTSELECTED_PLUS_0);
			int xstart = ((width - 20 - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth( m_metainfo ))/2)+10;
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+ xstart, y+4 + 2*fheight+sheight, width- 2*xstart, m_metainfo, COL_MENUCONTENTSELECTED);
		}
	}

}

void CMP3PlayerGui::updateTimes(const bool force)
{
	if (m_state != CMP3PlayerGui::STOP)
	{
		bool updateTotal = force;
		bool updatePlayed = force;

		if (m_time_total != CAudioPlayer::getInstance()->getTimeTotal())
		{
			m_time_total = CAudioPlayer::getInstance()->getTimeTotal();
			if (curr_audiofile.Duration != CAudioPlayer::getInstance()->getTimeTotal())
			{
				curr_audiofile.Duration = CAudioPlayer::getInstance()->getTimeTotal();
				if(current >=0)
					playlist[current].Duration = CAudioPlayer::getInstance()->getTimeTotal();
			}
			updateTotal = true;
		}
		if ((m_time_played != CAudioPlayer::getInstance()->getTimePlayed()))
		{
			m_time_played = CAudioPlayer::getInstance()->getTimePlayed();
			updatePlayed = true;
		}
		char tot_time[11];
		snprintf(tot_time, 10, " / %ld:%02ld", m_time_total / 60, m_time_total % 60);
		char tmp_time[8];
		snprintf(tmp_time, 7, "%ld:00", m_time_total / 60);
		char play_time[8];
		snprintf(play_time, 7, "%ld:%02ld", m_time_played / 60, m_time_played % 60);

		int w1 = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tot_time);
		int w2 = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp_time);

		if (updateTotal)
		{
			frameBuffer->paintBoxRel(x+width-w1-10, y+4, w1+4, fheight, COL_MENUCONTENTSELECTED_PLUS_0);
			if(m_time_total > 0)
				g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+width-w1-10, y+4 + fheight, w1, tot_time, COL_MENUCONTENTSELECTED);
		}
		if (updatePlayed || (m_state == CMP3PlayerGui::PAUSE))
		{
			frameBuffer->paintBoxRel(x+width-w1-w2-15, y+4, w2+4, fheight, COL_MENUCONTENTSELECTED_PLUS_0);
			struct timeval tv;
			gettimeofday(&tv, NULL);
			if ((m_state != CMP3PlayerGui::PAUSE) || (tv.tv_sec & 1))
			{
				g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+width-w1-w2-11, y+4 + fheight, w2, play_time, COL_MENUCONTENTSELECTED);
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
		CLCD::getInstance()->showMP3(curr_audiofile.Artist, curr_audiofile.Title, curr_audiofile.Album);
		break;
	case CMP3PlayerGui::PAUSE:
		CLCD::getInstance()->showMP3Play(CLCD::MP3_PAUSE);
		CLCD::getInstance()->showMP3(curr_audiofile.Artist, curr_audiofile.Title, curr_audiofile.Album);
		break;
	case CMP3PlayerGui::FF:
		CLCD::getInstance()->showMP3Play(CLCD::MP3_FF);
		CLCD::getInstance()->showMP3(curr_audiofile.Artist, curr_audiofile.Title, curr_audiofile.Album);
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

void CMP3PlayerGui::GetMetaData(CAudiofile *File)
{
	CAudioMetaData m=CAudioPlayer::getInstance()->readMetaData(File->Filename.c_str(), 
																				  m_state!=CMP3PlayerGui::STOP && 
																				  !g_settings.mp3player_highprio);

	File->Title = m.title;
	File->Artist = m.artist;
	File->Album = m.album;
	File->Year = m.date;
	File->Duration = m.total_time;
	File->Genre = m.genre;
	
	if (File->Artist.empty() && File->Title.empty())
	{
		//Set from Filename
		std::string tmp = File->Filename.substr(File->Filename.rfind('/')+1);
		tmp = tmp.substr(0,tmp.length()-4);	//remove extension (.mp3)
		unsigned int i = tmp.rfind(" - ");
		if(i != std::string::npos)
		{ // Trennzeiche " - " gefunden
			File->Artist = tmp.substr(0, i);
			File->Title = tmp.substr(i+3);
		}
		else
		{
			i = tmp.rfind('-');
			if(i != std::string::npos)
			{ //Trennzeichen "-"
				File->Artist = tmp.substr(0, i);
				File->Title = tmp.substr(i+1);
			}
			else
				File->Title	= tmp;
		}
#ifdef FILESYSTEM_IS_ISO8859_1_ENCODED
		File->Artist = Latin1_to_UTF8(File->Artist);
		File->Title = Latin1_to_UTF8(File->Title);
#endif
	}
}

