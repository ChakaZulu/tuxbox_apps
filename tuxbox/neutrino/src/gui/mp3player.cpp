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

#include "eventlist.h"
#include "mp3player.h"
#include "color.h"
#include "infoviewer.h"

#include "widget/menue.h"
#include "widget/messagebox.h"
#include "widget/hintbox.h"
#include "widget/stringinput.h"

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
	width = 720;
	if ((g_settings.screen_EndX- g_settings.screen_StartX) < width-ConnectLineBox_Width)
		width=(g_settings.screen_EndX- g_settings.screen_StartX)-ConnectLineBox_Width;
	height = 570;
	if ((g_settings.screen_EndY- g_settings.screen_StartY) < height)
		height=(g_settings.screen_EndY- g_settings.screen_StartY);
	buttonHeight = min(25,g_Fonts->infobar_small->getHeight());
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->menu->getHeight();
	title_height=fheight*3+4;
	info_height=fheight*2+4;
	listmaxshow = (height-info_height-title_height-theight-2*buttonHeight)/(fheight);
	height = theight+info_height+title_height+2*buttonHeight+listmaxshow*fheight;	// recalc height

	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX + ConnectLineBox_Width;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height)/ 2) + g_settings.screen_StartY;
	liststart = 0;
	filebrowser = new CFileBrowser();
	filebrowser->Multi_Select = true;
	filebrowser->Select_Dirs = true;
	mp3filter.addFilter("mp3");
	filebrowser->Filter = &mp3filter;
	Path = "/";
}

//------------------------------------------------------------------------

CMP3PlayerGui::~CMP3PlayerGui()
{
	playlist.clear();
	delete filebrowser;
}

//------------------------------------------------------------------------
int CMP3PlayerGui::exec(CMenuTarget* parent, string actionKey)
{
	m_state=STOP;
	current=-1;

	if(parent)
	{
		parent->hide();
	}
	
	// set radio mode background
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
	
	/*int ret =*/ show();
	
	// Restore normal background
	if(frameBuffer->getActive())
		memset(frameBuffer->getFrameBufferPointer(), 255, frameBuffer->getStride()*576);
	frameBuffer->useBackground(false);
	
	// Restore last mode
 	//t_channel_id channel_id=CNeutrinoApp::getInstance()->channelList->getActiveChannel_ChannelID();
 	//g_Zapit->zapTo_serviceID(channel_id);
	g_Zapit->setStandby(false);
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , m_LastMode );
	sleep(3); // zapit doesnt like fast zapping in the moment

	// always exit all	
	return menu_return::RETURN_EXIT_ALL;
}

//------------------------------------------------------------------------

int CMP3PlayerGui::show()
{
	int res = -1;

	CLCD::getInstance()->setMode(CLCD::MODE_MP3);
	
	unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_menu );
	uint msg; uint data;

	bool loop=true;
	bool update=true;
	State last_state=STOP;
	while(loop)
	{
		showTime();
		if(CNeutrinoApp::getInstance()->getMode()!=NeutrinoMessages::mode_mp3)
		{
			// stop if mode was changed in another thread
			loop=false;
		}
		if(m_state != last_state)
		{
			last_state=m_state;
			update=true;
		}
		if(m_state == PLAY && CMP3Player::getInstance()->state!=CMP3Player::PLAY && playlist.size() >0)
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
//		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
		g_RCInput->getMsg( &msg, &data, 10 ); // 1 sec timeout to update play/stop state display

		if( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_menu );

		if( msg == CRCInput::RC_home)
		{ //Exit after cancel key
			loop=false;
		}
		else if ( msg == CRCInput::RC_timeout )
		{
			// do nothing
		}
		else if ( msg == CRCInput::RC_left )
		{
			if((unsigned)current-1 > 0)
				play(current-1);
			else
				play(0);

/*			selected+=listmaxshow;
			if (selected>playlist.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();*/
		}
		else if ( msg == CRCInput::RC_right )
		{
			int next = getNext();
			play(next);
/*			if ((int(selected)-int(listmaxshow))<0)
				selected=playlist.size()-1;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();*/
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
				paint();
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
				paint();
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
		else if(msg==CRCInput::RC_red && playlist.size() > 0)
		{
			CPlayList::iterator p = playlist.begin()+selected;
			playlist.erase(p);
			if((int)selected==current)
			{
				current--;
				CMP3Player::getInstance()->stop(); // Stop if song is deleted, next song will be startet automat.
			}
			if(selected > playlist.size()-1)
				selected = playlist.size()-1;
			update=true;
		}
		else if(msg==CRCInput::RC_green)
		{
			hide();
			if(filebrowser->exec(Path))
			{
				CFileList::iterator files = filebrowser->getSelectedFiles()->begin();
				for(; files != filebrowser->getSelectedFiles()->end();files++)
				{
					string file = files->Name;
//					Path = file.substr(0,file.rfind("/"));
					if(file.length() > 0)
					{
						int ext_pos = file.rfind(".");
						if( ext_pos > 0)
						{
							string extension;
							extension = file.substr(ext_pos + 1, file.length() - ext_pos);
							if(extension == "mp3")
							{
								CMP3 mp3;
								mp3.Filename = file;
								get_id3(&mp3);
								//printf("id3: Title: '%s' Artist: '%s' Comment: '%s'\n", mp3.Title.c_str(), mp3.Artist.c_str(), mp3.Comment.c_str());
								playlist.push_back(mp3);
							}
						}
					}
				}
			}
			CLCD::getInstance()->setMode(CLCD::MODE_MP3);
			update=true;
		}
		else if(msg==CRCInput::RC_yellow)
		{
			stop();
			playlist.clear();
			current=-1;
			selected=0;
			update=true;
		}
		else if(msg==CRCInput::RC_blue)
		{
			stop();
		}
		else if(msg==CRCInput::RC_help)
		{
			int i=0;
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
			if(m_state==PLAY)
				current=0;
			else
				current=-1;
			update=true;
		}
		else if(msg==CRCInput::RC_setup)
		{
			//pushback key if...
			g_RCInput->postMsg( msg, data );
			loop=false;
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
		}
	}
	hide();
	
	if (m_state == PLAY)
		stop();

	return(res);
}

//------------------------------------------------------------------------

void CMP3PlayerGui::hide()
{
	if(visible)
	{
		frameBuffer->paintBackgroundBoxRel(x-ConnectLineBox_Width, y+title_height, width+ConnectLineBox_Width, height-title_height);
		clearItemID3DetailsLine();
		visible = false;
	}
}

//------------------------------------------------------------------------

void CMP3PlayerGui::paintItem(int pos)
{
	int ypos = y+ +title_height+theight+0 + pos*fheight;
	int color;
	if( (liststart+pos < playlist.size()) && (pos % 2) )
		color = COL_MENUCONTENTDARK;
	else
		color = COL_MENUCONTENT;

	if(liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
		paintItemID3DetailsLine(pos);
	}
	if(liststart+pos==(unsigned)current)
	{
		color = color+2;
	}

	frameBuffer->paintBoxRel(x,ypos, width-15, fheight, color);
	if(liststart+pos<playlist.size())
	{
		string tmp="???";
		if(playlist[liststart+pos].Artist != "" && playlist[liststart+pos].Title!="")
			tmp = playlist[liststart+pos].Artist + " / " + playlist[liststart+pos].Title;
		else if(playlist[liststart+pos].Artist != "")
			tmp = playlist[liststart+pos].Artist;
		else if(playlist[liststart+pos].Title != "")
			tmp = playlist[liststart+pos].Title;
		g_Fonts->menu->RenderString(x+10,ypos+fheight, width-25, tmp.c_str(), color, fheight);
		if(liststart+pos==selected && m_state==STOP)
			CLCD::getInstance()->showMP3(playlist[liststart+pos].Artist, playlist[liststart+pos].Title);

	}
}

//------------------------------------------------------------------------

void CMP3PlayerGui::paintHead()
{
	string strCaption = g_Locale->getText("mp3player.head");
	frameBuffer->paintBoxRel(x,y+title_height, width,theight+0, COL_MENUHEAD);
	frameBuffer->paintIcon("mp3.raw",x+7,y+title_height+10);
	g_Fonts->menu_title->RenderString(x+35,y+theight+title_height+0, width- 45, strCaption.c_str(), COL_MENUHEAD);
}

//------------------------------------------------------------------------

void CMP3PlayerGui::paintFoot()
{
	int ButtonWidth = (width-20) / 4;
	frameBuffer->paintBoxRel(x,y+(height-info_height-2*buttonHeight), width,2*buttonHeight, COL_MENUHEAD);
	frameBuffer->paintHLine(x, x+width,  y+(height-info_height-2*buttonHeight), COL_INFOBAR_SHADOW);

	if(playlist.size()>0)
	{
		frameBuffer->paintIcon("rot.raw", x+ 0* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x + 0* ButtonWidth + 30, y+(height-info_height-2*buttonHeight)+24 - 1, ButtonWidth- 20, g_Locale->getText("mp3player.delete").c_str(), COL_INFOBAR);
		
		frameBuffer->paintIcon("ok.raw", x+ 3* ButtonWidth + 10, y+(height-info_height-buttonHeight)-3);
		g_Fonts->infobar_small->RenderString(x+3 * ButtonWidth + 38 , y+(height-info_height-buttonHeight)+24 - 4, ButtonWidth- 28, g_Locale->getText("mp3player.play").c_str(), COL_INFOBAR);
	
		frameBuffer->paintIcon("gelb.raw", x+ 2* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x+ 2* ButtonWidth + 30, y+(height-info_height-2*buttonHeight)+24 - 1, ButtonWidth- 20, g_Locale->getText("mp3player.deleteall").c_str(), COL_INFOBAR);
	
		frameBuffer->paintIcon("help.raw", x+ 2* ButtonWidth + 10, y+(height-info_height-buttonHeight)-3);
		g_Fonts->infobar_small->RenderString(x+ 2* ButtonWidth +38 , y+(height-info_height-buttonHeight)+24 - 4, ButtonWidth- 28, g_Locale->getText("mp3player.shuffle").c_str(), COL_INFOBAR);
	}

	frameBuffer->paintIcon("gruen.raw", x+ 1* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
	g_Fonts->infobar_small->RenderString(x+ 1* ButtonWidth +30, y+(height-info_height-2*buttonHeight)+24 - 1, ButtonWidth- 20, g_Locale->getText("mp3player.add").c_str(), COL_INFOBAR);

	if(m_state == PLAY)
	{
		frameBuffer->paintIcon("blau.raw", x+ 3* ButtonWidth + 10, y+(height-info_height-2*buttonHeight)+4);
		g_Fonts->infobar_small->RenderString(x+ 3* ButtonWidth + 30, y+(height-info_height-2*buttonHeight)+24 - 2, ButtonWidth- 20, g_Locale->getText("mp3player.stop").c_str(), COL_INFOBAR);
	}
}
//------------------------------------------------------------------------

void CMP3PlayerGui::paint()
{
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX + ConnectLineBox_Width;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height)/ 2) + g_settings.screen_StartY;

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
	visible = true;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------

// shameless stolen from player.c (mad)

void CMP3PlayerGui::get_id3(CMP3 *mp3)
{
unsigned int i;
struct id3_frame const *frame;
id3_ucs4_t const *ucs4;
id3_latin1_t *latin1;
char const spaces[] = "          ";

struct 
{
    char const *id;
    char const *name;
} const info[] = 
{
	{ ID3_FRAME_TITLE,  "Title"     },
	{ "TIT3",           0               },  /* Subtitle */
	{ "TCOP",           0,              },  /* Copyright */
	{ "TPRO",           0,              },  /* Produced */
	{ "TCOM",           "Composer"  },
	{ ID3_FRAME_ARTIST, "Artist"    },
	{ "TPE2",           "Orchestra" },
	{ "TPE3",           "Conductor" },
	{ "TEXT",           "Lyricist"  },
	{ ID3_FRAME_ALBUM,  "Album"     },
	{ ID3_FRAME_YEAR,   "Year"      },
	{ ID3_FRAME_TRACK,  "Track"     },
	{ "TPUB",           "Publisher" },
	{ ID3_FRAME_GENRE,  "Genre"     },
	{ "TRSN",           "Station"   },
	{ "TENC",           "Encoder"   }
};

  /* text information */

	struct id3_file *id3file = id3_file_open(mp3->Filename.c_str(), ID3_FILE_MODE_READONLY);
	if(id3file == 0)
		printf("error open id3 file\n");
	id3_tag *tag=id3_file_tag(id3file);
	if (tag) 
	{
		for (i = 0; i < sizeof(info) / sizeof(info[0]); ++i) 
		{
		union id3_field const *field;
		unsigned int nstrings, namelen, j;
		char const *name;

			frame = id3_tag_findframe(tag, info[i].id, 0);
			if (frame == 0)
				continue;

			field    = &frame->fields[1];
			nstrings = id3_field_getnstrings(field);

			name = info[i].name;
			namelen = name ? strlen(name) : 0;
			assert(namelen < sizeof(spaces));

			for (j = 0; j < nstrings; ++j) 
			{
				ucs4 = id3_field_getstrings(field, j);
				assert(ucs4);

				if (strcmp(info[i].id, ID3_FRAME_GENRE) == 0)
					ucs4 = id3_genre_name(ucs4);

				latin1 = id3_ucs4_latin1duplicate(ucs4);
				if (latin1 == 0)
					goto fail;

				if (j == 0 && name)
				{
					if(strcmp(name,"Title") == 0)
						mp3->Title = (char *) latin1;
					if(strcmp(name,"Artist") == 0)
						mp3->Artist = (char *) latin1;
					if(strcmp(name,"Year") == 0)
						mp3->Year = (char *) latin1;
					//printf("%s%s: %s\n", &spaces[namelen], name, latin1);
				}
				else 
				{
					if (strcmp(info[i].id, "TCOP") == 0 || strcmp(info[i].id, "TPRO") == 0) 
					{
						//printf("%s  %s %s\n", spaces, (info[i].id[1] == 'C') ? ("Copyright (C)") : ("Produced (P)"), latin1);
					}
					//else
						//printf("%s  %s\n", spaces, latin1);
				}

				free(latin1);
			}
		}

	  /* comments */

		i = 0;
		while ((frame = id3_tag_findframe(tag, ID3_FRAME_COMMENT, i++))) 
		{
			id3_latin1_t *ptr, *newline;
			int first = 1;

			ucs4 = id3_field_getstring(&frame->fields[2]);
			assert(ucs4);

			if (*ucs4)
				continue;

			ucs4 = id3_field_getfullstring(&frame->fields[3]);
			assert(ucs4);

			latin1 = id3_ucs4_latin1duplicate(ucs4);
			if (latin1 == 0)
				goto fail;

			ptr = latin1;
			while (*ptr) 
			{
				newline = (id3_latin1_t *) strchr((char*)ptr, '\n');
				if (newline)
					*newline = 0;

				if (strlen((char *)ptr) > 66) 
				{
				id3_latin1_t *linebreak;

					linebreak = ptr + 66;

					while (linebreak > ptr && *linebreak != ' ')
						--linebreak;

					if (*linebreak == ' ') 
					{
						if (newline)
							*newline = '\n';

						newline = linebreak;
						*newline = 0;
					}
				}

				if (first) 
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

			free(latin1);
			break;
		}
		id3_tag_delete(tag);
	}
	else
		printf("error open id3 tag\n");

	if(id3file)
		id3_file_close(id3file);

	if(mp3->Artist == "" && mp3->Title=="")
	{
		//Set from Filename
		string tmp = mp3->Filename.substr(mp3->Filename.rfind("/")+1);
		tmp = tmp.substr(0,tmp.length()-4); //remove .mp3
		unsigned int i = tmp.find("-");
		if(i == string::npos)
			mp3->Artist = tmp;
		else
		{
			mp3->Artist = tmp.substr(0, i);
			mp3->Title = tmp.substr(i+1);
		}
	}
	if (0) 
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
	if (pos >= 0)
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
		//
		g_Fonts->menu->RenderString(x+10, ypos2 + 2+ 1*fheight, width- 20, playlist[selected].Title.c_str(), COL_MENUCONTENTDARK);
		g_Fonts->menu->RenderString(x+10, ypos2 + 2+ 2*fheight, width- 20, playlist[selected].Artist.c_str(), COL_MENUCONTENTDARK);




	}
}

void CMP3PlayerGui::stop()
{
	m_state=STOP;
	//LCD
	CLCD::getInstance()->showMP3Play(false);
	//Display
	frameBuffer->paintBackgroundBoxRel(x, y, width, title_height);
	//MP3
	if(CMP3Player::getInstance()->state == CMP3Player::PLAY)
		CMP3Player::getInstance()->stop();
	current=-1;
}

void CMP3PlayerGui::play(int pos)
{
	unsigned int old_current=current;
	current=pos;
	if(old_current - liststart >=0 && old_current - liststart < listmaxshow)
		paintItem(old_current - liststart);
	if(pos - liststart >=0 && pos - liststart < listmaxshow)
		paintItem(pos - liststart);
	//LCD
	CLCD::getInstance()->showMP3Play(true);
	CLCD::getInstance()->showMP3(playlist[current].Artist, playlist[current].Title);
	// Display
	unsigned char colFrame = COL_MENUCONTENT+6;
	frameBuffer->paintBoxRel(x,         y, width, title_height-20, colFrame);
	frameBuffer->paintBoxRel(x+2, y +2 , width-4, title_height-24, COL_MENUCONTENTSELECTED);
	
	int w=g_Fonts->menu->getRenderWidth(g_Locale->getText("mp3player.playing"));
	int xstart=(width-w)/2;
	if(xstart < 10)
		xstart=10;
	g_Fonts->menu->RenderString(x+xstart, y + 4 + 1*fheight, width- 20, g_Locale->getText("mp3player.playing"), COL_MENUCONTENTSELECTED);
	w=g_Fonts->menu->getRenderWidth(playlist[current].Artist + " / " + playlist[current].Title);
	xstart=(width-w)/2;
	if(xstart < 10)
		xstart=10;
	g_Fonts->menu->RenderString(x+xstart, y +4+ 2*fheight, width- 20, playlist[current].Artist + " / " + playlist[current].Title, COL_MENUCONTENTSELECTED);
	//g_Fonts->menu->RenderString(x+10, y + 5 +3*g_Fonts->channellist->getHeight(), width- 20, g_Locale->getText("mp3player.title") + ": " + playlist[current].Title, COL_MENUCONTENT);
	//g_Fonts->menu->RenderString(x+10, y + 5 +3*g_Fonts->channellist->getHeight(), width- 20, "Comment: " + playlist[current].Comment, COL_MENUCONTENT);
	//int w= g_Fonts->menu->getRenderWidth("Year: " + playlist[current].Year);
	//g_Fonts->menu->RenderString(x+width-w-10, y + 5 +3*g_Fonts->channellist->getHeight(), width- 20, "Year: " + playlist[current].Year, COL_MENUCONTENT);

	CMP3Player::getInstance()->play(playlist[current].Filename.c_str()); 
	m_starttime = time(NULL);
	showTime();
	m_state=PLAY;
}

int CMP3PlayerGui::getNext()
{
	int ret=current+1;
	if((unsigned)ret+1 > playlist.size())
		ret=0;
	return ret;
}
void CMP3PlayerGui::showTime()
{
	if(m_state==PLAY)
	{
		int diff = time(NULL)-m_starttime;
		char sTime[11];
		sprintf(sTime,"%d:%02d",diff/60,diff%60);
		int w=g_Fonts->menu->getRenderWidth("000:00");
		frameBuffer->paintBoxRel(x+width-w-10, y+4, w+8, fheight, COL_MENUCONTENTSELECTED);
		g_Fonts->menu->RenderString(x+width-w-10, y+4 + 1*fheight, w, sTime, COL_MENUCONTENTSELECTED);
	}
}
