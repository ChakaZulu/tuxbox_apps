/*
  Neutrino-GUI  -   DBoxII-Project

  AudioPlayer by Dirch,Zwen
	
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

#include <gui/audioplayer.h>

#include <global.h>
#include <neutrino.h>

#include <driver/encoding.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/audioplay.h>
#include <driver/audiometadata.h>
#define DBOX 1
#ifdef DBOX
#include <driver/aviaext.h>
#endif

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
#include <gui/widget/stringinput_ext.h>

#include <system/settings.h>

#include <algorithm>
#include <sys/time.h>
#include <fstream>
#include <iostream>
#include <sstream>

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

#define AUDIOPLAYERGUI_SMSKEY_TIMEOUT 1000
#define SHOW_FILE_LOAD_LIMIT 50

//#define AUDIOPLAYER_TIME_DEBUG

// check if files to be added are already in the playlist
#define AUDIOPLAYER_CHECK_FOR_DUPLICATES

CAudiofileExt::CAudiofileExt()
	: CAudiofile(), firstChar('\0')
{
}

CAudiofileExt::CAudiofileExt(std::string name, CFile::FileType type)
	: CAudiofile(name, type), firstChar('\0')
{
}

CAudiofileExt::CAudiofileExt(const CAudiofileExt& src)
	: CAudiofile(src), firstChar(src.firstChar)
{
}

void CAudiofileExt::operator=(const CAudiofileExt& src)
{
	if (&src == this)
		return;
	CAudiofile::operator=(src);
	firstChar = src.firstChar;	
}


//------------------------------------------------------------------------

CAudioPlayerGui::CAudioPlayerGui()
{
	m_frameBuffer = CFrameBuffer::getInstance();

	m_visible = false;
	m_selected = 0;
	m_metainfo.clear();

	m_select_title_by_name = g_settings.audioplayer_select_title_by_name==1;

	if(strlen(g_settings.network_nfs_audioplayerdir)!=0)
		m_Path = g_settings.network_nfs_audioplayerdir;
	else
		m_Path = "/";

	audiofilefilter.addFilter("cdr");
	audiofilefilter.addFilter("mp3");
	audiofilefilter.addFilter("m2a");
	audiofilefilter.addFilter("mpa");
	audiofilefilter.addFilter("mp2");
	audiofilefilter.addFilter("m3u");
	audiofilefilter.addFilter("ogg");
	audiofilefilter.addFilter("url");
	audiofilefilter.addFilter("wav");
	m_SMSKeyInput.setTimeout(AUDIOPLAYERGUI_SMSKEY_TIMEOUT);
}

//------------------------------------------------------------------------

CAudioPlayerGui::~CAudioPlayerGui()
{
	m_playlist.clear();
	m_title2Pos.clear();
	g_Zapit->setStandby (false);
	g_Sectionsd->setPauseScanning (false);
}

//------------------------------------------------------------------------
int CAudioPlayerGui::exec(CMenuTarget* parent, const std::string & actionKey)
{
	CAudioPlayer::getInstance()->init();
	m_state = CAudioPlayerGui::STOP;

	m_show_playlist = g_settings.audioplayer_show_playlist==1;
	
	if (m_select_title_by_name != (g_settings.audioplayer_select_title_by_name==1))
	{
		if ((g_settings.audioplayer_select_title_by_name == 1)
			&& m_playlistHasChanged)
		{
			buildSearchTree();
		}
		m_select_title_by_name = g_settings.audioplayer_select_title_by_name;
	}

	if (m_playlist.empty())
		m_current = -1;
	else
		m_current = 0;
	
	m_selected = 0;
	m_width = 710;
	if((g_settings.screen_EndX - g_settings.screen_StartX) < m_width+ConnectLineBox_Width)
		m_width=(g_settings.screen_EndX - g_settings.screen_StartX) - ConnectLineBox_Width;
	m_height = 570;
	if((g_settings.screen_EndY - g_settings.screen_StartY) < m_height)
		m_height = (g_settings.screen_EndY - g_settings.screen_StartY);
	m_sheight = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
	m_buttonHeight = std::min(25, m_sheight);
	m_theight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	m_fheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	m_title_height = m_fheight*2 + 20 + m_sheight + 4;
	m_info_height = m_fheight*2;
	m_listmaxshow = (m_height - m_info_height - m_title_height - m_theight - 2*m_buttonHeight) / (m_fheight);
	m_height = m_theight + m_info_height + m_title_height + 2*m_buttonHeight + m_listmaxshow * m_fheight; // recalc height

	m_x = (((g_settings.screen_EndX - g_settings.screen_StartX) - (m_width + ConnectLineBox_Width)) / 2) 
		+ g_settings.screen_StartX + ConnectLineBox_Width;
	m_y = (((g_settings.screen_EndY- g_settings.screen_StartY) - m_height)/ 2) + g_settings.screen_StartY;
	m_idletime=time(NULL);
	m_screensaver=false;

	if(parent)
	{
		parent->hide();
	}

	if(g_settings.video_Format != CControldClient::VIDEOFORMAT_4_3)
		g_Controld->setVideoFormat(CControldClient::VIDEOFORMAT_4_3);

	bool usedBackground = m_frameBuffer->getuseBackground();
	if (usedBackground)
		m_frameBuffer->saveBackgroundImage();
	m_frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	m_frameBuffer->loadBackground("radiomode.raw");
	m_frameBuffer->useBackground(true);
	m_frameBuffer->paintBackground();

	// set zapit in standby mode
	g_Zapit->setStandby(true);
  
	// If Audiomode is OST then save setting and switch to AVS-Mode
	if(g_settings.audio_avs_Control == CControld::TYPE_OST)
	{
		m_vol_ost = true;
		g_settings.audio_avs_Control = CControld::TYPE_AVS;
	}
	else
		m_vol_ost = false;

	// tell neutrino we're in audio mode
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_audio );
	// remember last mode
	m_LastMode=(CNeutrinoApp::getInstance()->getLastMode() | NeutrinoMessages::norezap);

	// Stop sectionsd
	g_Sectionsd->setPauseScanning(true); 

#ifdef DBOX
	// disable iec aka digi out
	CAViAExt::getInstance()->iecOff();
#endif
	
	/*int ret =*/

	show();

	// Restore previous background
	if (usedBackground)
		m_frameBuffer->restoreBackgroundImage();
	m_frameBuffer->useBackground(usedBackground);
	m_frameBuffer->paintBackground();

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

#ifdef DBOX
	// enable iec aka digi out
	CAViAExt::getInstance()->iecOn();
#endif

	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , m_LastMode );
	g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

	// always exit all	
	return menu_return::RETURN_EXIT_ALL;
}

//------------------------------------------------------------------------

int CAudioPlayerGui::show()
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = -1;

	CLCD::getInstance()->setMode(CLCD::MODE_AUDIO);
	paintLCD();

	bool loop = true;
	bool update = true;
	bool clear_before_update = false;
	m_key_level = 0;

	while(loop)
	{
		if(!m_screensaver)
		{
			updateMetaData();
		}
		updateTimes();

		if(CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_audio)
		{
			// stop if mode was changed in another thread
			loop = false;
		}
		if ((m_state != CAudioPlayerGui::STOP) && 
		    (CAudioPlayer::getInstance()->getState() == CBaseDec::STOP) && 
		    (!m_playlist.empty()))
		{
			int next = getNext();
			if (next >= 0)
				play(next);
			else
				stop();
		}

		if (update)
		{
			if(clear_before_update)
			{
				hide();
				clear_before_update = false;
			}
			update = false;
			paint();
		}
		g_RCInput->getMsg(&msg, &data, 10); // 1 sec timeout to update play/stop state display

		if( msg == CRCInput::RC_timeout  || msg == NeutrinoMessages::EVT_TIMER)
		{
			int timeout = time(NULL) - m_idletime;
			int screensaver_timeout = atoi(g_settings.audioplayer_screensaver);
			if(screensaver_timeout !=0 && timeout > screensaver_timeout*60 && !m_screensaver)
				screensaver(true);
		}
		else
		{
			m_idletime = time(NULL);
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
			if (m_state != CAudioPlayerGui::STOP)
				stop();        
			else
				loop=false;
		}
		else if( msg == CRCInput::RC_left)
		{
			if(m_key_level == 1)
			{
				if(m_current == -1)
					stop();
				else if(m_current-1 > 0)
					play(m_current-1);
				else
					play(0);
			}
			else
			{
				if (!m_show_playlist)
				{
					m_current--;
					if (m_current < 0)
						m_current = m_playlist.size()-1;
					update = true;
				}
				else if(m_selected > 0 )
				{
					if (int(m_selected) > int(m_listmaxshow))
						m_selected = m_playlist.size()-1;
					else
						m_selected -= m_listmaxshow;
					m_liststart = (m_selected/m_listmaxshow)*m_listmaxshow;
					update = true;
				}
			}

		}
		else if( msg == CRCInput::RC_right)
		{
			if(m_key_level == 1)
			{
				int next = getNext();
				if(next >= 0)
					play(next);
				else
					stop();
			}
			else
			{
				if (!m_show_playlist)
				{
					m_current++;
					if (m_current >= (int)m_playlist.size())
						m_current = 0;
					update = true;
				}
				else if(m_selected != m_playlist.size()-1 && !m_playlist.empty())
				{
					m_selected += m_listmaxshow;
					if (m_selected >= m_playlist.size())
						m_selected = 0;
					m_liststart = (m_selected / m_listmaxshow) * m_listmaxshow;
					update = true;
				}
			}
		}
		else if( msg == CRCInput::RC_up && !m_playlist.empty() && m_show_playlist)
		{
			int prevselected = m_selected;
			if(m_selected == 0)
			{
				m_selected = m_playlist.size()-1;
			}
			else
				m_selected--;
				paintItem(prevselected - m_liststart);
			unsigned int oldliststart = m_liststart;
			m_liststart = (m_selected/m_listmaxshow)*m_listmaxshow;
			if(oldliststart != m_liststart)
			{
				update = true;
			}
			else
			{
				paintItem(m_selected - m_liststart);
			}
		}
		else if( msg == CRCInput::RC_down && !m_playlist.empty() && m_show_playlist)
		{
			int prevselected = m_selected;
			m_selected = (m_selected + 1) % m_playlist.size();
			paintItem(prevselected - m_liststart);
			unsigned int oldliststart = m_liststart;
			m_liststart = (m_selected/m_listmaxshow)*m_listmaxshow;
			if(oldliststart != m_liststart)
			{
				update = true;
			}
			else
			{
				paintItem(m_selected - m_liststart);
			}
		}
		else if (msg == CRCInput::RC_ok)
		{
			if (!m_playlist.empty())
				if (!m_show_playlist)
					play(m_current);
				else
					play(m_selected);
		}
		else if (msg == CRCInput::RC_red)
		{
			if(m_key_level == 0)
			{
				if (!m_playlist.empty())
				{
					//xx CPlayList::iterator p = m_playlist.begin()+selected;
					removeFromPlaylist(m_selected);
					if((int)m_selected == m_current)
					{
						m_current--;
						//stop(); // Stop if song is deleted, next song will be startet automat.
					}
					if(m_selected >= m_playlist.size())
						m_selected = m_playlist.size() == 0 ? m_playlist.size() : m_playlist.size() - 1;
					update = true;
				}
			}
			else if(m_key_level == 1)
			{
				stop();
			} else
			{
				// key_level==2
			}

		}
		else if(msg == CRCInput::RC_green)
		{
			if (m_key_level == 0)
			{
				CFileBrowser filebrowser((g_settings.filebrowser_denydirectoryleave) 
										 ? g_settings.network_nfs_audioplayerdir : "");

				filebrowser.Multi_Select    = true;
				filebrowser.Dirs_Selectable = true;
				filebrowser.Filter          = &audiofilefilter;

				hide();

				if (filebrowser.exec(m_Path.c_str()))
				{
#ifdef AUDIOPLAYER_TIME_DEBUG
					timeval start;
					gettimeofday(&start,NULL);
#endif
					CProgressWindow progress;
					long maxProgress = filebrowser.getSelectedFiles().size()-1;
					long currentProgress = -1;
					if (maxProgress > SHOW_FILE_LOAD_LIMIT)
					{
						progress.setTitle(LOCALE_AUDIOPLAYER_READING_FILES);
						progress.exec(this,"");
					}

					m_Path = filebrowser.getCurrentDir();
					CFileList::const_iterator files = filebrowser.getSelectedFiles().begin();
					for(; files != filebrowser.getSelectedFiles().end();files++)
					{
						if (maxProgress > SHOW_FILE_LOAD_LIMIT)
						{
							currentProgress++;
							progress.showGlobalStatus(100*currentProgress/maxProgress);
							progress.showStatusMessageUTF(files->Name);
						}
						if ((files->getType() == CFile::FILE_CDR) ||
						    (files->getType() == CFile::FILE_OGG) ||
						    (files->getType() == CFile::FILE_MP3) ||
						    (files->getType() == CFile::FILE_WAV))
						{
							CAudiofileExt audiofile(files->Name,
												  files->getType());
							addToPlaylist(audiofile);
						}
						if(files->getType() == CFile::STREAM_AUDIO)
						{
							CAudiofileExt mp3( files->Name, CFile::STREAM_AUDIO );
							mp3.MetaData.artist = "Shoutcast";
							std::string tmp = mp3.Filename.substr(mp3.Filename.rfind('/')+1);
							tmp = tmp.substr(0,tmp.length()-4);	//remove .url
							mp3.MetaData.title = tmp;
							mp3.MetaData.total_time = 0;
							addToPlaylist(mp3);
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
#ifdef AUDIOPLAYER_CHECK_FOR_DUPLICATES
                                        // Check for duplicates and remove (new entry has higher prio)
										// this really needs some time :(
										for (unsigned long i=0;i<m_playlist.size();i++)
										{
											if(m_playlist[i].Filename == filename)
												removeFromPlaylist(i);
										}
#endif
										if(strcasecmp(filename.substr(filename.length()-3,3).c_str(), "url")==0)
										{
											CAudiofileExt mp3( filename, CFile::STREAM_AUDIO );
											mp3.MetaData.artist = "Shoutcast";
											std::string tmp = mp3.Filename.substr(mp3.Filename.rfind('/')+1);
											tmp = tmp.substr(0,tmp.length()-4);	//remove .url
											mp3.MetaData.title = tmp;
											mp3.MetaData.total_time = 0;												
											addToPlaylist(mp3);
										}
										else
										{
											CFile playlistItem;
											playlistItem.Name = filename;											
											CFile::FileType fileType = playlistItem.getType();
											if (fileType == CFile::FILE_CDR
												|| fileType == CFile::FILE_MP3 
												|| fileType == CFile::FILE_OGG
												|| fileType == CFile::FILE_WAV) 
											{
												CAudiofileExt audioFile(filename,fileType);
												addToPlaylist(audioFile);
											} else
											{
												printf("Audioplayer: file type (%d) is *not* supported in playlists\n(%s)\n",
													   fileType, filename.c_str()); 
											}
										}
									}
									testfile.close();
								}
							}
							infile.close();
						}
					}
					if (m_select_title_by_name)
					{
						buildSearchTree();
					}
#ifdef AUDIOPLAYER_TIME_DEBUG
					timeval end;
					gettimeofday(&end,NULL);
					printf("adding %ld files took: ",maxProgress+1);
					printTimevalDiff(start,end);
#endif
				}
				CLCD::getInstance()->setMode(CLCD::MODE_AUDIO);
				paintLCD();
				update=true;
				// if playlist is turned off -> start playing immediately
				if (!m_show_playlist && !m_playlist.empty())
					play(m_selected);
			}
			else if (m_key_level == 1)
			{
				if(m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
					rev();
			} else { // key_level == 2
				
 				if(m_state == CAudioPlayerGui::STOP)
 				{
 					if (!m_playlist.empty()) 
 					{
 						savePlaylist();
 						CLCD::getInstance()->setMode(CLCD::MODE_AUDIO);
 						paintLCD();
 						update = true;
 					}
 				} else
				{
					// keylevel 2 can only be reached if the currently played file
					// is no stream, so we do not have to test for this case
					long seconds=0;
					CIntInput secondsInput(LOCALE_AUDIOPLAYER_JUMP_DIALOG_TITLE,
										   seconds,
										   5,
										   LOCALE_AUDIOPLAYER_JUMP_DIALOG_HINT1,
										   LOCALE_AUDIOPLAYER_JUMP_DIALOG_HINT2);
					int res = secondsInput.exec(NULL,"");
					if (seconds != 0 && res!= menu_return::RETURN_EXIT_ALL)
						rev(seconds);
					update=true;
				}
			}
		}
		else if(msg == CRCInput::RC_yellow)
		{
			if(m_key_level == 0)
			{
				//stop();
				m_playlist.clear();
				m_current = -1;
				m_selected = 0;
				clear_before_update = true;
				update = true;
				m_title2Pos.clear();
			}
			else if(m_key_level == 1)
			{
				pause();
			} else 
			{ // key_level==2
				m_select_title_by_name =! m_select_title_by_name;
				if (m_select_title_by_name && m_playlistHasChanged)
					buildSearchTree();
				paint();
			}
		}
		else if(msg == CRCInput::RC_blue)
		{
			if (m_key_level == 0)
			{
				if (!(m_playlist.empty()))
				{
					if (m_current > 0)
					{
						std::swap(m_playlist[0], m_playlist[m_current]);
						m_current = 0;
					}
				
					std::random_shuffle((m_current != 0) ? m_playlist.begin() : m_playlist.begin() + 1, m_playlist.end());
					if (m_select_title_by_name)
					{
						buildSearchTree();
					}
					m_playlistHasChanged = true;
					m_selected = 0;

					update = true;
				}
			}
			else if (m_key_level == 1)
			{
				if(m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
					ff();
			} else // key_level == 2
			{
				if (m_state != CAudioPlayerGui::STOP)
				{
					// keylevel 2 can only be reached if the currently played file
					// is no stream, so we do not have to test for this case
					long seconds=0;
					CIntInput secondsInput(LOCALE_AUDIOPLAYER_JUMP_DIALOG_TITLE,
										   seconds,
										   5,
										   LOCALE_AUDIOPLAYER_JUMP_DIALOG_HINT1,
										   LOCALE_AUDIOPLAYER_JUMP_DIALOG_HINT2);
					int res = secondsInput.exec(NULL,"");
					if (seconds != 0 && res!= menu_return::RETURN_EXIT_ALL)
					  ff(seconds);
					update = true;
				}
			}
		}
		else if(msg == CRCInput::RC_help)
		{
			if (m_key_level == 2)
				m_key_level = 0;
			else
				m_key_level++;

			if (m_state != CAudioPlayerGui::STOP)
			{
				// jumping in streams not supported
				if (m_key_level == 2 &&
					m_curr_audiofile.FileType == CFile::STREAM_AUDIO)
				{
					m_key_level = 0;
				}
			}
			// there are only two keylevels in the "STOP"-case
			else if(m_key_level == 1)
			{
				m_key_level = 2;
			}
			paintFoot();
		}
		else if(msg == CRCInput::RC_0)
		{
			if(m_current >= 0)
			{
				m_selected = m_current;
				update = true;
			}
		}
		else if ((msg >= CRCInput::RC_1) && (msg <= CRCInput::RC_9) && !(m_playlist.empty()))
		{ //numeric zap or SMS zap
			if (m_select_title_by_name)
			{
				//printf("select by name\n");
				unsigned char smsKey = 0;				
				do 
				{
					smsKey = m_SMSKeyInput.handleMsg(msg);
					//printf("  new key: %c", smsKey);
					g_RCInput->getMsg_ms(&msg, &data, AUDIOPLAYERGUI_SMSKEY_TIMEOUT - 200);
					

					/* show a hint box with current char (too slow at the moment?)*/
// 					char selectedKey[1];
// 					sprintf(selectedKey,"%c",smsKey);
// 					int x1=(g_settings.screen_EndX- g_settings.screen_StartX)/2 + g_settings.screen_StartX-50;
// 					int y1=(g_settings.screen_EndY- g_settings.screen_StartY)/2 + g_settings.screen_StartY;
// 					int h = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getHeight();
// 					int w = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getRenderWidth(selectedKey);
// 					m_frameBuffer->paintBoxRel(x1 - 7, y1 - h - 5, w + 14, h + 10, COL_MENUCONTENT_PLUS_6);
// 					m_frameBuffer->paintBoxRel(x1 - 4, y1 - h - 3, w +  8, h +  6, COL_MENUCONTENTSELECTED_PLUS_0);
// 					g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]
// 						->RenderString(x1,y1,w+1,selectedKey,COL_MENUCONTENTSELECTED,0);


				} while ((msg >= CRCInput::RC_1) && (msg <= CRCInput::RC_9) && !(m_playlist.empty()));
				
				if (msg == CRCInput::RC_timeout
				    || msg == CRCInput::RC_nokey)
				{
					//printf("selected key: %c\n",smsKey);
					selectTitle(smsKey);
					update = true;
				}
				m_SMSKeyInput.resetOldKey();
			} else 
			{
				//printf("numeric zap\n");
				int val = 0;
				if (getNumericInput(msg,val)) 
					m_selected = std::min((int)m_playlist.size(), val) - 1;
				update = true;
			}

		}

		else if(msg == CRCInput::RC_setup)
		{
			CNFSSmallMenu nfsMenu;
			nfsMenu.exec(this, "");
			CLCD::getInstance()->setMode(CLCD::MODE_AUDIO);
			paintLCD();
			update = true;
			//pushback key if...
			//g_RCInput->postMsg( msg, data );
			//loop = false;
		}
		else if(msg == NeutrinoMessages::CHANGEMODE)
		{
			if((data & NeutrinoMessages::mode_mask) != NeutrinoMessages::mode_audio)
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
			paintLCD();
		}
	}
	hide();

	if(m_state != CAudioPlayerGui::STOP)
		stop();

	return(res);
}

//------------------------------------------------------------------------


void CAudioPlayerGui::hide()
{
//	printf("hide(){\n");
	if(m_visible)
	{
		m_frameBuffer->paintBackgroundBoxRel(m_x - ConnectLineBox_Width-1, m_y + m_title_height - 1,
											 m_width + ConnectLineBox_Width+2, m_height + 2 - m_title_height);
		clearItemID3DetailsLine();
		m_frameBuffer->paintBackgroundBoxRel(m_x, m_y, m_width, m_title_height);
		m_visible = false;
	}
}

//------------------------------------------------------------------------

void CAudioPlayerGui::paintItem(int pos)
{
	if (!m_show_playlist)
		return;
  
	int ypos = m_y + m_title_height + m_theight + pos*m_fheight;
	uint8_t    color;
	fb_pixel_t bgcolor;

	if ((pos + m_liststart) == m_selected)
	{
		if ((pos + m_liststart) == (unsigned)m_current)
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
		if (((pos + m_liststart) < m_playlist.size()) && (pos & 1))
		{
			if ((pos + m_liststart) == (unsigned)m_current)
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
			if ((pos + m_liststart) == (unsigned)m_current)
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

	m_frameBuffer->paintBoxRel(m_x, ypos, m_width - 15, m_fheight, bgcolor);

	if ((pos + m_liststart) < m_playlist.size())
	{
		if (m_playlist[pos + m_liststart].FileType != CFile::STREAM_AUDIO &&
			 !m_playlist[pos + m_liststart].MetaData.bitrate)
		{
			// id3tag noch nicht geholt
			GetMetaData(m_playlist[pos + m_liststart]);
			if(m_state != CAudioPlayerGui::STOP && !g_settings.audioplayer_highprio)
				usleep(100*1000);
		}
		char sNr[20];
		sprintf(sNr, "%2d : ", pos + m_liststart + 1);
		std::string tmp = sNr;
		getFileInfoToDisplay(tmp,m_playlist[pos + m_liststart]);
		
		char dura[9];
		snprintf(dura, 8, "%ld:%02ld", m_playlist[pos + m_liststart].MetaData.total_time / 60,
				 m_playlist[pos + m_liststart].MetaData.total_time % 60);
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(dura) + 5;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x + 10, ypos + m_fheight, m_width - 30 - w,
																tmp, color, m_fheight, true); // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x + m_width - 15 - w, ypos + m_fheight,
																w, dura, color, m_fheight);
		if ((pos + m_liststart) == m_selected)
		{
			paintItemID3DetailsLine(pos);
			if (m_state == CAudioPlayerGui::STOP)
				CLCD::getInstance()->showAudioTrack(m_playlist[pos + m_liststart].MetaData.artist, 
													m_playlist[pos + m_liststart].MetaData.title,
													m_playlist[pos + m_liststart].MetaData.album);
		}
	}
}

//------------------------------------------------------------------------

void CAudioPlayerGui::paintHead()
{
	if (!m_show_playlist)
		return;

	std::string strCaption = g_Locale->getText(LOCALE_AUDIOPLAYER_HEAD);
	m_frameBuffer->paintBoxRel(m_x, m_y + m_title_height, m_width, m_theight, COL_MENUHEAD_PLUS_0);
	m_frameBuffer->paintIcon("mp3.raw",m_x + 7, m_y + m_title_height + 10);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(m_x + 35, m_y + m_theight + m_title_height + 0,
																  m_width - 45, strCaption, COL_MENUHEAD, 0, true); // UTF-8
	int ypos = m_y + m_title_height;
	if(m_theight > 26)
		ypos = (m_theight - 26) / 2 + m_y + m_title_height;
	m_frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_DBOX, m_x + m_width - 30, ypos);
	if( CNeutrinoApp::getInstance()->isMuted() )
	{
		int xpos = m_x + m_width - 75;
		ypos = m_y + m_title_height;
		if(m_theight > 32)
			ypos = (m_theight - 32) / 2 + m_y + m_title_height;
		m_frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_MUTE, xpos, ypos);
	}
}

//------------------------------------------------------------------------
const struct button_label AudioPlayerButtons[7][4] =
{
	{
		{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_AUDIOPLAYER_STOP                        },
		{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_AUDIOPLAYER_REWIND                      },
		{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_AUDIOPLAYER_PAUSE                       },
		{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_AUDIOPLAYER_FASTFORWARD                 },
	},
	{
		{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_AUDIOPLAYER_DELETE                      },
		{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_AUDIOPLAYER_ADD                         },
		{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_AUDIOPLAYER_DELETEALL                   },
		{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_AUDIOPLAYER_SHUFFLE                     }
	},
	{
		{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_AUDIOPLAYER_JUMP_BACKWARDS              },
		{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_AUDIOPLAYER_JUMP_FORWARDS               }
	},
	{
		{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_AUDIOPLAYER_JUMP_BACKWARDS              },
		{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_AUDIOPLAYER_JUMP_FORWARDS               }
	},
	{
		{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_AUDIOPLAYER_SAVE_PLAYLIST               },
		{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_AUDIOPLAYER_BUTTON_SELECT_TITLE_BY_ID   },			
	},
	{
		{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_AUDIOPLAYER_SAVE_PLAYLIST               },
		{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_AUDIOPLAYER_BUTTON_SELECT_TITLE_BY_NAME },			
	},
	{
		{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_AUDIOPLAYER_STOP                        },
		{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_AUDIOPLAYER_PAUSE                       },
	},
};
//------------------------------------------------------------------------

void CAudioPlayerGui::paintFoot()
{
//	printf("paintFoot{\n");
	int top;
	if (m_show_playlist)
		top = m_y + (m_height - m_info_height - 2 * m_buttonHeight);
	else
		top = m_y + (m_height - 2 * m_buttonHeight);

	int ButtonWidth = (m_width - 20) / 4;
	int ButtonWidth2 = (m_width - 50) / 2;
	m_frameBuffer->paintBoxRel(m_x, top, m_width, 2 * m_buttonHeight, COL_MENUHEAD_PLUS_0);
	m_frameBuffer->paintHLine(m_x, m_x + m_width, top, COL_INFOBAR_SHADOW_PLUS_0);

	if (!m_playlist.empty())
	{
		// play
		m_frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_OKAY, m_x + 1 * ButtonWidth2 + 25, top + m_buttonHeight - 3);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]
			->RenderString(m_x + 1 * ButtonWidth2 + 53, top + m_buttonHeight + 24 - 4, ButtonWidth2 - 28,
						   g_Locale->getText(LOCALE_AUDIOPLAYER_PLAY), COL_INFOBAR, 0, true); // UTF-8		
		// keylevel switch
		m_frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, m_x + 0 * ButtonWidth + 25, top + m_buttonHeight - 3);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]
			->RenderString(m_x + 0 * ButtonWidth + 53 , top + m_buttonHeight + 24 - 4, ButtonWidth2 - 28,
						   g_Locale->getText(LOCALE_AUDIOPLAYER_KEYLEVEL), COL_INFOBAR, 0, true); // UTF-8
	}

	if (m_key_level == 0)
	{
		if (m_playlist.empty())
			::paintButtons(m_frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, 
						   m_x + ButtonWidth + 10, top + 4, ButtonWidth, 1, &(AudioPlayerButtons[1][1]));
		else
			::paintButtons(m_frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale,
						   m_x + 10, top + 4, ButtonWidth, 4, AudioPlayerButtons[1]);
	}
	else if (m_key_level == 1)
	{
		if (m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
		{
			::paintButtons(m_frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, m_x + 10, top + 4, ButtonWidth, 4, AudioPlayerButtons[0]);
		}
		else
		{
			::paintButtons(m_frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, m_x + 10, top + 4, ButtonWidth*2, 2, AudioPlayerButtons[6]);
		}
	} else { // key_level == 2
		if (m_state == CAudioPlayerGui::STOP)
		{
			if (m_select_title_by_name)
			{
				::paintButtons(m_frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, 
							   m_x + ButtonWidth + 10, top + 4, ButtonWidth, 2, AudioPlayerButtons[5]);
			} else {
				::paintButtons(m_frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale,
							   m_x + ButtonWidth + 10, top + 4, ButtonWidth, 2, AudioPlayerButtons[4]);
			}
		} else {
			if (m_select_title_by_name)
			{
				::paintButtons(m_frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale,
							   m_x + ButtonWidth + 10, top + 4, ButtonWidth*2, 2, AudioPlayerButtons[3]);
			} else {
				::paintButtons(m_frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale,
							   m_x + ButtonWidth + 10, top + 4, ButtonWidth*2, 2, AudioPlayerButtons[2]);
			}
		}
	}
}
//------------------------------------------------------------------------
void CAudioPlayerGui::paintInfo()
{
	if(m_state == CAudioPlayerGui::STOP && m_show_playlist)
		m_frameBuffer->paintBackgroundBoxRel(m_x, m_y, m_width, m_title_height);
	else
	{
		if (!m_show_playlist)
		{
			// no playlist -> smaller Info-Box
			m_frameBuffer->paintBoxRel(m_x, m_y, m_width, m_title_height - 10 - m_fheight, COL_MENUCONTENT_PLUS_6);
			m_frameBuffer->paintBoxRel(m_x + 2, m_y + 2 , m_width - 4, m_title_height - 14 - m_fheight,
									   COL_MENUCONTENTSELECTED_PLUS_0);
		}
		else
		{
			m_frameBuffer->paintBoxRel(m_x, m_y, m_width, m_title_height - 10, COL_MENUCONTENT_PLUS_6);
			m_frameBuffer->paintBoxRel(m_x + 2, m_y + 2 , m_width - 4, m_title_height - 14, 
									   COL_MENUCONTENTSELECTED_PLUS_0);      
		}
		// first line (Track number)
		char sNr[20];
		sprintf(sNr, ": %2d", m_current + 1);
		std::string tmp = g_Locale->getText(LOCALE_AUDIOPLAYER_PLAYING);
		tmp += sNr ;
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp, true); // UTF-8
		int xstart = (m_width - w) / 2;
		if(xstart < 10)
			xstart = 10;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x + xstart, m_y + 4 + 1*m_fheight, m_width - 20,
																tmp, COL_MENUCONTENTSELECTED, 0, true); // UTF-8
		// second line (Artist/Title...)
		if (m_curr_audiofile.FileType != CFile::STREAM_AUDIO &&
			!m_curr_audiofile.MetaData.bitrate)
		{
			GetMetaData(m_curr_audiofile);
		}

		if (m_curr_audiofile.MetaData.title.empty())
			tmp = m_curr_audiofile.MetaData.artist;
		else if (m_curr_audiofile.MetaData.artist.empty())
			tmp = m_curr_audiofile.MetaData.title;
		else if (g_settings.audioplayer_display == TITLE_ARTIST)
		{
			tmp = m_curr_audiofile.MetaData.title;
			tmp += " / ";
			tmp += m_curr_audiofile.MetaData.artist;
		}
		else //if(g_settings.audioplayer_display == ARTIST_TITLE)
		{
			tmp = m_curr_audiofile.MetaData.artist;
			tmp += " / ";
			tmp += m_curr_audiofile.MetaData.title;
		}
		w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp, true); // UTF-8
		xstart=(m_width-w)/2;
		if(xstart < 10)
			xstart=10;
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x+xstart, m_y +4+ 2*m_fheight, m_width- 20, tmp, COL_MENUCONTENTSELECTED, 0, true); // UTF-8
		
		// reset so fields get painted always
		m_metainfo.clear();
		m_time_total = 0;
		m_time_played = 0;
		updateMetaData();
    
		updateTimes(true);
	}
}
//------------------------------------------------------------------------

void CAudioPlayerGui::paint()
{
	if (m_show_playlist) 
	{
		m_liststart = (m_selected / m_listmaxshow) * m_listmaxshow;
		paintHead();
		for (unsigned int count=0;count<m_listmaxshow;count++)
		{
			paintItem(count);
		}

		int ypos = m_y + m_title_height + m_theight;
		int sb = m_fheight * m_listmaxshow;
		m_frameBuffer->paintBoxRel(m_x + m_width - 15, ypos, 15, sb, COL_MENUCONTENT_PLUS_1);

		int sbc = ((m_playlist.size() - 1) / m_listmaxshow) + 1;
		float sbh = (sb - 4) / sbc;
		int sbs = (m_selected / m_listmaxshow);

		m_frameBuffer->paintBoxRel(m_x + m_width - 13, ypos + 2 + int(sbs * sbh) , 11, int(sbh), COL_MENUCONTENT_PLUS_3);
	}

	paintFoot();
	paintInfo();
	m_visible = true;
	
}

//------------------------------------------------------------------------

void CAudioPlayerGui::clearItemID3DetailsLine ()
{
	paintItemID3DetailsLine(-1);
}
//------------------------------------------------------------------------

void CAudioPlayerGui::paintItemID3DetailsLine (int pos)
{
	int xpos  = m_x - ConnectLineBox_Width;
	int ypos1 = m_y + m_title_height + m_theight+ 0 + pos*m_fheight;
	int ypos2 = m_y + (m_height - m_info_height);
	int ypos1a = ypos1 + (m_fheight / 2) - 2;
	int ypos2a = ypos2 + (m_info_height / 2) - 2;
	fb_pixel_t col1 = COL_MENUCONTENT_PLUS_6;
	fb_pixel_t col2 = COL_MENUCONTENT_PLUS_1;


	// Clear
	m_frameBuffer->paintBackgroundBoxRel(xpos - 1, m_y + m_title_height, ConnectLineBox_Width + 1, 
										 m_height - m_title_height);

	// paint Line if detail info (and not valid list pos)
	if (!m_playlist.empty() && (pos >= 0))
	{
		// 1. col thick line
		m_frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 4, ypos1, 4, m_fheight, col1);
		m_frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 4, ypos2, 4, m_info_height, col1);
		
		m_frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 16, ypos1a, 4, ypos2a - ypos1a, col1);
		
		m_frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 16, ypos1a, 12, 4, col1);
		m_frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 16, ypos2a, 12, 4, col1);
		
		// 2. col small line
		m_frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 4, ypos1, 1, m_fheight, col2);
		m_frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 4, ypos2, 1, m_info_height, col2);
		
		m_frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 16, ypos1a, 1, ypos2a - ypos1a + 4, col2);

		m_frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 16, ypos1a, 12, 1, col2);
		m_frameBuffer->paintBoxRel(xpos + ConnectLineBox_Width - 12, ypos2a,  8, 1, col2);
		
		// -- small Frame around infobox
		m_frameBuffer->paintBoxRel(m_x, ypos2, m_width, 2, col1);
		m_frameBuffer->paintBoxRel(m_x, ypos2 + 2, 2, m_info_height - 4, col1);
		m_frameBuffer->paintBoxRel(m_x + m_width - 2, ypos2 + 2, 2, m_info_height - 4, col1);
		m_frameBuffer->paintBoxRel(m_x, ypos2 + m_info_height - 2, m_width, 2, col1);
//		m_frameBuffer->paintBoxRel(m_x, ypos2, m_width, m_info_height, col1);

		// paint id3 infobox 
		m_frameBuffer->paintBoxRel(m_x + 2, ypos2 + 2 , m_width - 4, m_info_height - 4, COL_MENUCONTENTDARK_PLUS_0);
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x + 10, ypos2 + 2 + 1*m_fheight, m_width- 80,
																m_playlist[m_selected].MetaData.title, COL_MENUCONTENTDARK, 0, true); // UTF-8
		std::string tmp;
		if (m_playlist[m_selected].MetaData.genre.empty())
			tmp = m_playlist[m_selected].MetaData.date;
		else if (m_playlist[m_selected].MetaData.date.empty())
			tmp = m_playlist[m_selected].MetaData.genre;
		else
		{
			tmp = m_playlist[m_selected].MetaData.genre;
			tmp += " / ";
			tmp += m_playlist[m_selected].MetaData.date;
		}
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(tmp, true) + 10; // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x + m_width - w - 5, ypos2 + 2 + 1*m_fheight,
																w, tmp, COL_MENUCONTENTDARK, 0, true); // UTF-8
		tmp = m_playlist[m_selected].MetaData.artist;
		if (!(m_playlist[m_selected].MetaData.album.empty()))
		{
			tmp += " (";
			tmp += m_playlist[m_selected].MetaData.album;
			tmp += ')';
		}
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x + 10, ypos2 + 2*m_fheight - 2, m_width - 20,
																tmp, COL_MENUCONTENTDARK, 0, true); // UTF-8
	}
	else
	{
		m_frameBuffer->paintBackgroundBoxRel(m_x, ypos2, m_width, m_info_height);
	}
}
//------------------------------------------------------------------------

void CAudioPlayerGui::stop()
{
	m_state = CAudioPlayerGui::STOP;
	m_current = 0;
	//LCD
	paintLCD();
	//Display
	paintInfo();
	m_key_level = 0;
	paintFoot();
	
	if(CAudioPlayer::getInstance()->getState() != CBaseDec::STOP)
		CAudioPlayer::getInstance()->stop();
}
//------------------------------------------------------------------------

void CAudioPlayerGui::pause()
{
	if(m_state == CAudioPlayerGui::PLAY 
	   || m_state == CAudioPlayerGui::FF 
	   || m_state == CAudioPlayerGui::REV)
	{
		m_state = CAudioPlayerGui::PAUSE;
		CAudioPlayer::getInstance()->pause();
	}
	else if(m_state == CAudioPlayerGui::PAUSE)
	{
		m_state = CAudioPlayerGui::PLAY;
		CAudioPlayer::getInstance()->pause();
	}
	paintLCD();
}
//------------------------------------------------------------------------

void CAudioPlayerGui::ff(unsigned int seconds)
{
	if(m_state == CAudioPlayerGui::FF)
	{
		m_state = CAudioPlayerGui::PLAY;
		CAudioPlayer::getInstance()->ff(seconds);
	}
	else if(m_state == CAudioPlayerGui::PLAY 
			|| m_state == CAudioPlayerGui::PAUSE 
			|| m_state == CAudioPlayerGui::REV)
	{
		m_state = CAudioPlayerGui::FF;
		CAudioPlayer::getInstance()->ff(seconds);
	}
	paintLCD();
}
//------------------------------------------------------------------------

void CAudioPlayerGui::rev(unsigned int seconds)
{
	if(m_state == CAudioPlayerGui::REV)
	{
		m_state = CAudioPlayerGui::PLAY;
		CAudioPlayer::getInstance()->rev(seconds);
	}
	else if(m_state == CAudioPlayerGui::PLAY 
			|| m_state == CAudioPlayerGui::PAUSE
			|| m_state == CAudioPlayerGui::FF)
	{
		m_state = CAudioPlayerGui::REV;
		CAudioPlayer::getInstance()->rev(seconds);
	}
	paintLCD();
}
//------------------------------------------------------------------------

void CAudioPlayerGui::play(int pos)
{
	//printf("AudioPlaylist: play %d/%d\n",pos,playlist.size());
	unsigned int old_current = m_current;
	unsigned int old_selected = m_selected;

	m_current = pos;
	if(g_settings.audioplayer_follow)
		m_selected = pos;

	if(m_selected - m_liststart >= m_listmaxshow && g_settings.audioplayer_follow)
	{
		m_liststart = m_selected;
		if(!m_screensaver)
			paint();
	}
	else if(m_liststart - m_selected < 0 && g_settings.audioplayer_follow)
	{
		m_liststart = m_selected - m_listmaxshow + 1;
		if(!m_screensaver)
			paint();
	}
	else
	{
		if(old_current - m_liststart >=0 && old_current - m_liststart < m_listmaxshow)
		{
			if(!m_screensaver)
				paintItem(old_current - m_liststart);
		}
		if(pos - m_liststart >=0 && pos - m_liststart < m_listmaxshow)
		{
			if(!m_screensaver)
				paintItem(pos - m_liststart);
		}
		if(g_settings.audioplayer_follow)
		{
			if(old_selected - m_liststart >=0 && old_selected - m_liststart < m_listmaxshow)
				if(!m_screensaver)
					paintItem(old_selected - m_liststart);
		}
	}

	if (m_playlist[pos].FileType != CFile::STREAM_AUDIO &&
		 !m_playlist[pos].MetaData.bitrate)
	{
		// id3tag noch nicht geholt
		//printf("play: need getMetaData\n");
		GetMetaData(m_playlist[pos]);
	}
	m_metainfo.clear();
	m_time_played = 0;
	m_time_total = m_playlist[m_current].MetaData.total_time;
	m_state = CAudioPlayerGui::PLAY;
	m_curr_audiofile = m_playlist[m_current];
	// Play
	CAudioPlayer::getInstance()->play(&m_curr_audiofile, g_settings.audioplayer_highprio == 1); 
	//LCD
	paintLCD();
	// Display
	if(!m_screensaver)
		paintInfo();
	m_key_level = 1;
	if(!m_screensaver)
		paintFoot();
}
//------------------------------------------------------------------------

int CAudioPlayerGui::getNext()
{
	int ret= m_current + 1;
	if(m_playlist.empty())
		return -1;
	if((unsigned)ret >= m_playlist.size()) {
		if (g_settings.audioplayer_repeat_on == 1)
			ret = 0;
		else
			ret = -1;
	}
	return ret;
}
//------------------------------------------------------------------------

void CAudioPlayerGui::updateMetaData()
{
	bool updateMeta = false;
	bool updateLcd = false;
	bool updateScreen = false;

	if(m_state == CAudioPlayerGui::STOP || !m_show_playlist)
		return;

	if( CAudioPlayer::getInstance()->hasMetaDataChanged()
		|| m_metainfo.empty() )
	{
		const CAudioMetaData meta =
			CAudioPlayer::getInstance()->getMetaData();

		std::stringstream info;
		info.precision(3);

		if ( meta.bitrate > 0 )
		{
			info << " / ";
			if ( meta.vbr )
		{
				info << "VBR ";
		}
			info << meta.bitrate/1000 << "kbps";
		}

		if ( meta.samplerate > 0 )
		{
			info << " / " << std::showpoint
				 << static_cast<float>( meta.samplerate ) / 1000 << "kHz";
		}
		
		m_metainfo = meta.type_info + info.str();
		updateMeta = true;
		
		if (!meta.artist.empty()  &&
			 meta.artist != m_curr_audiofile.MetaData.artist)
		{
			m_curr_audiofile.MetaData.artist = meta.artist;
			updateScreen = true;
			updateLcd = true;
		}
		if (!meta.title.empty() &&
			 meta.title != m_curr_audiofile.MetaData.title)
		{
			m_curr_audiofile.MetaData.title = meta.title;
			updateScreen = true;
			updateLcd = true;
		}
		if (!meta.sc_station.empty()  &&
			 meta.sc_station != m_curr_audiofile.MetaData.album)
		{
			m_curr_audiofile.MetaData.album = meta.sc_station;
			updateLcd = true;
		}
	}
	if (CAudioPlayer::getInstance()->getScBuffered() != 0)
	{
		updateLcd = true;
	}
	if(updateLcd)
		paintLCD();
	if(updateScreen)
		paintInfo();
	if(updateMeta || updateScreen)
	{
		m_frameBuffer->paintBoxRel(m_x + 10, m_y + 4 + 2*m_fheight, m_width - 20,
								   m_sheight, COL_MENUCONTENTSELECTED_PLUS_0);
		int xstart = ((m_width - 20 - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(m_metainfo))/2)+10;
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]
			->RenderString(m_x + xstart, m_y + 4 + 2*m_fheight + m_sheight,
						   m_width- 2*xstart, m_metainfo, COL_MENUCONTENTSELECTED);
	}
}
//------------------------------------------------------------------------

void CAudioPlayerGui::updateTimes(const bool force)
{
	if (m_state != CAudioPlayerGui::STOP)
	{
		bool updateTotal = force;
		bool updatePlayed = force;

		if (m_time_total != CAudioPlayer::getInstance()->getTimeTotal())
		{
			m_time_total = CAudioPlayer::getInstance()->getTimeTotal();
			if (m_curr_audiofile.MetaData.total_time != CAudioPlayer::getInstance()->getTimeTotal())
			{
				m_curr_audiofile.MetaData.total_time = CAudioPlayer::getInstance()->getTimeTotal();
				if(m_current >= 0)
					m_playlist[m_current].MetaData.total_time = CAudioPlayer::getInstance()->getTimeTotal();
			}
			updateTotal = true;
		}
		if ((m_time_played != CAudioPlayer::getInstance()->getTimePlayed()))
		{
			m_time_played = CAudioPlayer::getInstance()->getTimePlayed();
			updatePlayed = true;
		}
		if(!m_screensaver)
		{
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
				m_frameBuffer->paintBoxRel(m_x + m_width - w1 - 10, m_y + 4, w1 + 4, 
										   m_fheight, COL_MENUCONTENTSELECTED_PLUS_0);
				if(m_time_total > 0)
					g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x + m_width - w1 - 10, m_y + 4 + m_fheight,
																			w1, tot_time, COL_MENUCONTENTSELECTED);
			}
			if (updatePlayed || (m_state == CAudioPlayerGui::PAUSE))
			{
				m_frameBuffer->paintBoxRel(m_x + m_width - w1 - w2 - 15, m_y + 4, w2 + 4, m_fheight,
										   COL_MENUCONTENTSELECTED_PLUS_0);
				struct timeval tv;
				gettimeofday(&tv, NULL);
				if ((m_state != CAudioPlayerGui::PAUSE) || (tv.tv_sec & 1))
				{
					g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_x + m_width - w1 - w2 -11, m_y + 4 + m_fheight,
																			w2, play_time, COL_MENUCONTENTSELECTED);
				}
			}
		}
		if((updatePlayed || updateTotal) && m_time_total != 0)
		{
			CLCD::getInstance()->showAudioProgress((int)(100.0 * m_time_played / m_time_total), CNeutrinoApp::getInstance()->isMuted());
		}
	}
}

//------------------------------------------------------------------------

void CAudioPlayerGui::paintLCD()
{
	switch(m_state)
	{
	case CAudioPlayerGui::STOP:
		CLCD::getInstance()->showAudioPlayMode(CLCD::AUDIO_MODE_STOP);
		CLCD::getInstance()->showAudioProgress(0, CNeutrinoApp::getInstance()->isMuted());
		break;
	case CAudioPlayerGui::PLAY:
		CLCD::getInstance()->showAudioPlayMode(CLCD::AUDIO_MODE_PLAY);

		CLCD::getInstance()->showAudioTrack(m_curr_audiofile.MetaData.artist, m_curr_audiofile.MetaData.title,
											m_curr_audiofile.MetaData.album);
		if(m_curr_audiofile.FileType != CFile::STREAM_AUDIO)
			CLCD::getInstance()->showAudioProgress((int)(100.0 * m_time_played / m_time_total), CNeutrinoApp::getInstance()->isMuted());

#ifdef INCLUDE_UNUSED_STUFF
		else
			CLCD::getInstance()->showAudioProgress((int)(100.0 * CAudioPlayer::getInstance()->getScBuffered() / 65536), CNeutrinoApp::getInstance()->isMuted());
#endif /* INCLUDE_UNUSED_STUFF */
		break;
	case CAudioPlayerGui::PAUSE:
		CLCD::getInstance()->showAudioPlayMode(CLCD::AUDIO_MODE_PAUSE);
		CLCD::getInstance()->showAudioTrack(m_curr_audiofile.MetaData.artist, m_curr_audiofile.MetaData.title,
											m_curr_audiofile.MetaData.album);
		break;
	case CAudioPlayerGui::FF:
		CLCD::getInstance()->showAudioPlayMode(CLCD::AUDIO_MODE_FF);
		CLCD::getInstance()->showAudioTrack(m_curr_audiofile.MetaData.artist, m_curr_audiofile.MetaData.title,
											m_curr_audiofile.MetaData.album);
		break;
	case CAudioPlayerGui::REV:
		CLCD::getInstance()->showAudioPlayMode(CLCD::AUDIO_MODE_REV);
		CLCD::getInstance()->showAudioTrack(m_curr_audiofile.MetaData.artist, m_curr_audiofile.MetaData.title,
											m_curr_audiofile.MetaData.album);
		break;
	}
}
//------------------------------------------------------------------------

void CAudioPlayerGui::screensaver(bool on)
{
	if(on)
	{
		m_screensaver = true;
		m_frameBuffer->ClearFrameBuffer();
	}
	else
	{
		m_screensaver = false;
		m_frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
		m_frameBuffer->loadBackground("radiomode.raw");
		m_frameBuffer->useBackground(true);
		m_frameBuffer->paintBackground();
		paint();
		m_idletime = time(NULL);
	}
}
//------------------------------------------------------------------------

void CAudioPlayerGui::GetMetaData(CAudiofileExt &File)
{
//	printf("GetMetaData\n");
	bool ret =
	  CAudioPlayer::getInstance()->readMetaData(&File,
												m_state != CAudioPlayerGui::STOP &&
												!g_settings.audioplayer_highprio);	
	if (!ret ||
		(File.MetaData.artist.empty() && File.MetaData.title.empty() ) )
	{
		//Set from Filename
		std::string tmp = File.Filename.substr(File.Filename.rfind('/') + 1);
		tmp = tmp.substr(0,tmp.length()-4);	//remove extension (.mp3)
		unsigned int i = tmp.rfind(" - ");
		if(i != std::string::npos)
		{ // Trennzeichen " - " gefunden
			File.MetaData.artist = tmp.substr(0, i);
			File.MetaData.title = tmp.substr(i + 3);
		}
		else
		{
			i = tmp.rfind('-');
			if(i != std::string::npos)
			{ //Trennzeichen "-"
				File.MetaData.artist = tmp.substr(0, i);
				File.MetaData.title = tmp.substr(i + 1);
			}
			else
				File.MetaData.title = tmp;
		}
		File.MetaData.artist = FILESYSTEM_ENCODING_TO_UTF8_STRING(File.MetaData.artist);
		File.MetaData.title  = FILESYSTEM_ENCODING_TO_UTF8_STRING(File.MetaData.title );
	}
}
//------------------------------------------------------------------------

bool CAudioPlayerGui::getNumericInput(neutrino_msg_data_t& msg, int& val) {
	
	neutrino_msg_data_t data;
	int x1 = (g_settings.screen_EndX - g_settings.screen_StartX) / 2 + g_settings.screen_StartX - 50;
	int y1 = (g_settings.screen_EndY - g_settings.screen_StartY) / 2 + g_settings.screen_StartY;
	char str[11];
	do
	{
		val = val * 10 + CRCInput::getNumericValue(msg);
		sprintf(str, "%d", val);
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getRenderWidth(str);
		int h = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getHeight();
		m_frameBuffer->paintBoxRel(x1 - 7, y1 - h - 5, w + 14, h + 10, COL_MENUCONTENT_PLUS_6);
		m_frameBuffer->paintBoxRel(x1 - 4, y1 - h - 3, w +  8, h +  6, COL_MENUCONTENTSELECTED_PLUS_0);
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->RenderString(x1, y1, w + 1, str, COL_MENUCONTENTSELECTED, 0);
		g_RCInput->getMsg(&msg, &data, 100); 
	} while (g_RCInput->isNumeric(msg) && val < 1000000);
	return (msg == CRCInput::RC_ok);
}

//------------------------------------------------------------------------


void CAudioPlayerGui::getFileInfoToDisplay(std::string &info, CAudiofileExt &file)
{
	std::string fileInfo;
	std::string artist ="Artist?";
	std::string title = "Title?";
	
 	if (!file.MetaData.bitrate)
 	{
 		GetMetaData(file);
 	}

	if (!file.MetaData.artist.empty())
		artist = file.MetaData.artist;

	if (!file.MetaData.title.empty())
		title = file.MetaData.title;

	if(g_settings.audioplayer_display == TITLE_ARTIST)
	{
		fileInfo += title;
		fileInfo += ", ";
		fileInfo += artist;
	}
	else //if(g_settings.audioplayer_display == ARTIST_TITLE)
	{
		fileInfo += artist;
		fileInfo += ", ";
		fileInfo += title;
	}
	
	if (!file.MetaData.album.empty())
	{
		fileInfo += " (";
		fileInfo += file.MetaData.album;
		fileInfo += ')';
	} 
	if (fileInfo.empty())
	{
		fileInfo += "Unknown";
	}
	file.firstChar = tolower(fileInfo[0]);
	info += fileInfo;
}
//------------------------------------------------------------------------

void CAudioPlayerGui::addToPlaylist(CAudiofileExt &file)
{	
	if (m_select_title_by_name)
	{	
		if (!file.MetaData.bitrate)
		{
			std::string t = "";
			getFileInfoToDisplay(t,file);
		}
	}
	m_playlist.push_back(file);
	m_playlistHasChanged = true;
}

//------------------------------------------------------------------------
 void CAudioPlayerGui::removeFromPlaylist(long pos)
{
	unsigned char firstChar = ' ';
	if (m_select_title_by_name)
	{
		// must be called before m_playlist.erase()
		firstChar = getFirstChar(m_playlist[pos]);
	}

	m_playlist.erase(m_playlist.begin() + pos); 
	m_playlistHasChanged = true;
	
	if (m_select_title_by_name)
	{
		
#ifdef AUDIOPLAYER_TIME_DEBUG
		timeval start;
		gettimeofday(&start,NULL);
#endif
		//printf("searching for key: %c val: %ld\n",firstChar,pos);
			
		CTitle2Pos::iterator item = m_title2Pos.find(firstChar);
		if (item != m_title2Pos.end())
		{
			//const CPosList::size_type del =
			item->second.erase(pos);
			
			// delete empty entries
			if (item->second.size() == 0)
			{
				m_title2Pos.erase(item);
			}
		} else
		{
			printf("could not find key: %c pos: %ld\n",firstChar,pos);
		}
		// decrease position information for all titles with a position 
		// behind item to delete
		long p = 0;
		for (CTitle2Pos::iterator title=m_title2Pos.begin();
			 title!=m_title2Pos.end();title++)
		{
			CPosList newList;
			for (CPosList::iterator posIt=title->second.begin();
				 posIt!=title->second.end();posIt++)
			{
				p = *(posIt);
				if (*posIt > pos)
					p--;
				// old list is sorted so we can give a hint to insert at the end
				newList.insert(newList.end(), p);
			}
			//title->second.clear();
			title->second = newList;
		}		
#ifdef AUDIOPLAYER_TIME_DEBUG
		timeval end;
		gettimeofday(&end,NULL);
		printf("delete took: ");
		printTimevalDiff(start,end);
#endif

	}
}



//------------------------------------------------------------------------

void CAudioPlayerGui::selectTitle(unsigned char selectionChar)
{
	unsigned long i;
	
	//printf("fastLookup: key %c\n",selectionChar);
	CTitle2Pos::iterator it = m_title2Pos.find(selectionChar);
	if (it!=m_title2Pos.end())
	{
		// search for the next greater id
		// if nothing found take the first
		CPosList::iterator posIt = it->second.upper_bound(m_selected);
		if (posIt != it->second.end())
		{
			i = *posIt; 
			//printf("upper bound i: %ld\n",i);
		} else
		{
			if (it->second.size() > 0)
			{
				i = *(it->second.begin());
				//printf("using begin i: %ld\n",i);
			} else
			{
				//printf("no title with that key\n");
				return;
			}
		}	
	} else
	{
		//printf("no title with that key\n");
		return;
	}		
	
	int prevselected = m_selected;
	m_selected = i;
	
	paintItem(prevselected - m_liststart);
	unsigned int oldliststart = m_liststart;
	m_liststart = (m_selected / m_listmaxshow)*m_listmaxshow;
	//printf("before paint\n");
	if(oldliststart != m_liststart)
	{
		paint();
	}
	else
	{
		paintItem(m_selected - m_liststart);
	}
}
//------------------------------------------------------------------------

void CAudioPlayerGui::printSearchTree()
{
	for (CTitle2Pos::iterator it=m_title2Pos.begin();
		 it!=m_title2Pos.end();it++)
	{
		printf("key: %c\n",it->first);
		long pos=-1;
		for (CPosList::iterator it2=it->second.begin();
			 it2!=it->second.end();it2++)
		{
			pos++;
			printf(" val: %ld ",*it2);
			if (pos % 5 == 4)
				printf("\n");
		}
		printf("\n");
	}
}

//------------------------------------------------------------------------

void CAudioPlayerGui::buildSearchTree()
{

//	printf("before\n");
//	printSearchTree();

#ifdef AUDIOPLAYER_TIME_DEBUG
	timeval start;
	gettimeofday(&start,NULL);
#endif

	CProgressWindow progress;
	progress.setTitle(LOCALE_AUDIOPLAYER_BUILDING_SEARCH_INDEX);
	progress.exec(this, "");

	long maxProgress = m_playlist.size() - 1;

	m_title2Pos.clear();
	long listPos = -1;

	for (CAudioPlayList::iterator it=m_playlist.begin();
		 it!=m_playlist.end();it++)
	{
// 		if (m_state == CAudioPlayerGui::PLAY)
// 			usleep(10*1000);
		listPos++;
		progress.showGlobalStatus(100*listPos / maxProgress);
		//std::string info;
		progress.showStatusMessageUTF(it->Filename);
		unsigned char firstChar = getFirstChar(*it);
		const std::pair<CTitle2Pos::iterator,bool> item =
			m_title2Pos.insert(CTitle2PosItem(firstChar,CPosList()));
		item.first->second.insert(listPos);
	}
	progress.hide();
	m_playlistHasChanged = false;

#ifdef AUDIOPLAYER_TIME_DEBUG
	timeval end;
	gettimeofday(&end,NULL);
	printf("searchtree took: ");
	printTimevalDiff(start,end);
#endif
//	printf("after:\n");
	//printSearchTree();
}

//------------------------------------------------------------------------

unsigned char CAudioPlayerGui::getFirstChar(CAudiofileExt &file)
{
	if (file.firstChar == '\0')
	{
		std::string info;
		getFileInfoToDisplay(info,file);
	}
	//printf("getFirstChar: %c\n",file.firstChar);
	return file.firstChar;
}


//------------------------------------------------------------------------
#ifdef AUDIOPLAYER_TIME_DEBUG
void CAudioPlayerGui::printTimevalDiff(timeval &start, timeval &end)
{
	
	long secs = end.tv_sec - start.tv_sec;
	long usecs = end.tv_usec -start.tv_usec;
	if (usecs < 0)
	{
		usecs = 1000000 + usecs;
		secs--;		
	}
	printf("%ld:%ld\n",secs,usecs);
}
#endif

//------------------------------------------------------------------------

void CAudioPlayerGui::savePlaylist()
{
	const char *path;
	
	// .m3u playlist
	// http://hanna.pyxidis.org/tech/m3u.html
	
	CFileBrowser browser;
	browser.Multi_Select = false;
	browser.Dir_Mode = true;
	CFileFilter dirFilter;
	dirFilter.addFilter("m3u");
	browser.Filter = &dirFilter;
	// select preferred directory if exists
	if (strlen(g_settings.network_nfs_audioplayerdir) != 0)
		path = g_settings.network_nfs_audioplayerdir;
	else
		path = "/";
	
	// let user select target directory
	this->hide();
	if (browser.exec(path)) {
		// refresh view
		this->paint();
		CFile *file = browser.getSelectedFile();
		std::string absPlaylistDir = file->getPath();
		
		// add a trailing slash if necessary
		if ((absPlaylistDir.empty()) || ((*(absPlaylistDir.rbegin()) != '/')))
		{
			absPlaylistDir += '/';
		}
		absPlaylistDir += file->getFileName();
		
		const int filenamesize = 30;
		char filename[filenamesize + 1] = "";
		
		if (file->getType() == CFile::FILE_PLAYLIST) {
			// file is playlist so we should ask if we can overwrite it
			std::string name = file->getPath();
			name += '/';
			name += file->getFileName();
			bool overwrite = askToOverwriteFile(name);
			if (!overwrite) {
				return;
			}
			snprintf(filename, name.size(), "%s", name.c_str());
		} else if (file->getType() == CFile::FILE_DIR) {
			// query for filename
			this->hide();
			CStringInputSMS filenameInput(LOCALE_AUDIOPLAYER_PLAYLIST_NAME,
						      filename,
						      filenamesize - 1,
						      LOCALE_AUDIOPLAYER_PLAYLIST_NAME_HINT1,
						      LOCALE_AUDIOPLAYER_PLAYLIST_NAME_HINT2,
						      "abcdefghijklmnopqrstuvwxyz0123456789-.,:!?/ ");
			filenameInput.exec(NULL, "");
			// refresh view
			this->paint();
			std::string name = absPlaylistDir;
			name += '/';
			name += filename;
			name += ".m3u";
			std::ifstream input(name.c_str());
			
			// test if file exists and query for overwriting it or not
			if (input.is_open()) {
				bool overwrite = askToOverwriteFile(name);
				if (!overwrite) {
					return;
				}
			}
			input.close();
		} else {
			std::cout << "CAudioPlayerGui: neither .m3u nor directory selected, abort" << std::endl;
			return;
		}
		std::string absPlaylistFilename = absPlaylistDir;
		absPlaylistFilename += '/';
		absPlaylistFilename += filename;
		absPlaylistFilename += ".m3u";		
		std::ofstream playlistFile(absPlaylistFilename.c_str());
		std::cout << "CAudioPlayerGui: writing playlist to " << absPlaylistFilename << std::endl;
		if (!playlistFile) {
			// an error occured
			const int msgsize = 255;
			char msg[msgsize] = "";
			snprintf(msg,
				 msgsize,
				 "%s\n%s",
				 g_Locale->getText(LOCALE_AUDIOPLAYER_PLAYLIST_FILEERROR_MSG),
				 absPlaylistFilename.c_str());
			
			DisplayErrorMessage(msg);
			// refresh view
			this->paint();
			std::cout << "CAudioPlayerGui: could not create play list file " 
				  << absPlaylistFilename << std::endl;
			return;
		}
		// writing .m3u file
		playlistFile << "#EXTM3U" << std::endl;
		
		CAudioPlayList::const_iterator it;
		for (it = m_playlist.begin();it!=m_playlist.end();it++) {
			playlistFile << "#EXTINF:" << it->MetaData.total_time << ","
				     << it->MetaData.artist << " - " << it->MetaData.title << std::endl;
			playlistFile << absPath2Rel(absPlaylistDir, it->Filename) << std::endl;
		}
		playlistFile.close();
	}  
	this->paint();
}
//------------------------------------------------------------------------

bool CAudioPlayerGui::askToOverwriteFile(const std::string& filename) {
	
	char msg[filename.length() + 127];
	snprintf(msg,
		 filename.length() + 126,
		 "%s\n%s",
		 g_Locale->getText(LOCALE_AUDIOPLAYER_PLAYLIST_FILEOVERWRITE_MSG),
		 filename.c_str());
	bool res = (ShowMsgUTF(LOCALE_AUDIOPLAYER_PLAYLIST_FILEOVERWRITE_TITLE,
			       msg,CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo)
		    == CMessageBox::mbrYes);
	this->paint();
	return res;
}
//------------------------------------------------------------------------

std::string CAudioPlayerGui::absPath2Rel(const std::string& fromDir,
					 const std::string& absFilename) {
	std::string res = "";

	int length = fromDir.length() < absFilename.length() ? fromDir.length() : absFilename.length();
	int lastSlash = 0;
	// find common prefix for both paths
	// fromDir:     /foo/bar/angle/1          (length: 16)
	// absFilename: /foo/bar/devil/2/fire.mp3 (length: 19)
	// -> /foo/bar/ is prefix, lastSlash will be 8
	for (int i=0;i<length;i++) {
		if (fromDir[i] == absFilename[i]) {
			if (fromDir[i] == '/') {
				lastSlash = i;
			}
		} else {
			break;
		}
	}
	// cut common prefix
	std::string relFilepath = absFilename.substr(lastSlash + 1, absFilename.length() - lastSlash + 1);
	// relFilepath is now devil/2/fire.mp3
	
	// First slash is not removed because we have to go up each directory.
	// Since the slashes are counted later we make sure for each directory one slash is present
	std::string relFromDir = fromDir.substr(lastSlash, fromDir.length() - lastSlash);
	// relFromDir is now /angle/1
	
	// go up as many directories as neccessary
	for (unsigned int i=0;i<relFromDir.size();i++) {
		if (relFromDir[i] == '/') {
			res = res + "../";
		}
	}
	
	res = res + relFilepath;
	return res;
}
