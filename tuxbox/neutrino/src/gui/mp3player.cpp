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

#include "eventlist.h"
#include "mp3player.h"
#include "color.h"
#include "infoviewer.h"

#include "widget/menue.h"
#include "widget/messagebox.h"
#include "widget/hintbox.h"
#include "widget/stringinput.h"

#define info_height 60


//------------------------------------------------------------------------

CMP3PlayerGui::CMP3PlayerGui()
{
	frameBuffer = CFrameBuffer::getInstance();

	visible = false;
	selected = 0;
	width = 505;
	height = 300;
	buttonHeight = 25;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->menu->getHeight();
	listmaxshow = (height-theight-0)/(fheight);
	height = theight+0+listmaxshow*fheight;	// recalc height

	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-( height+ info_height) ) / 2) + g_settings.screen_StartY;
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
	g_Zapit->setStandby(false);
 	//t_channel_id channel_id=CNeutrinoApp::getInstance()->channelList->getActiveChannel_ChannelID();
 	//g_Zapit->zapTo_serviceID(channel_id);
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , m_LastMode );
	//sleep(5); // zapit doesnt like fast zapping in the moment

	// always exit all	
	return menu_return::RETURN_EXIT_ALL;
}

//------------------------------------------------------------------------

int CMP3PlayerGui::show()
{
	int res = -1;

	unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_menu );
	uint msg; uint data;

	bool loop=true;
	bool update=true;
	CMP3Player::State last_state=CMP3Player::STOP;
	while(loop)
	{
		if(CNeutrinoApp::getInstance()->getMode()!=NeutrinoMessages::mode_mp3)
		{
			// stop if mode was changed in another thread
			loop=false;
		}
		if(CMP3Player::getInstance()->state != last_state)
		{
			last_state=CMP3Player::getInstance()->state;
			update=true;
		}
		
		if(update)
		{
			hide();
			update=false;
			paint();
		}
//		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
		g_RCInput->getMsg( &msg, &data, 30 ); // 3 sec timeout to update play/stop state display

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
			selected+=listmaxshow;
			if (selected>playlist.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if ( msg == CRCInput::RC_right )
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=playlist.size()-1;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
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
			if(CMP3Player::getInstance()->state == CMP3Player::STOP)
				CMP3Player::getInstance()->play(playlist[selected].Filename.c_str());
			else
			{
				CMP3Player::getInstance()->stop();
				CMP3Player::getInstance()->play(playlist[selected].Filename.c_str());
			}
		}
		else if(msg==CRCInput::RC_red && playlist.size() > 0)
		{
			CPlayList::iterator p = playlist.begin();
			for(unsigned int i = 0 ;i < selected && p != playlist.end();p++,i++);
			playlist.erase(p);
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
								printf("id3: Title: '%s' Artist: '%s' Comment: '%s'\n", mp3.Title.c_str(), mp3.Artist.c_str(), mp3.Comment.c_str());
								playlist.push_back(mp3);
							}
						}
					}
				}
			}
			update=true;
		}
		else if(msg==CRCInput::RC_yellow)
		{
			if(CMP3Player::getInstance()->state == CMP3Player::PLAY)
				CMP3Player::getInstance()->stop();
		}
		else if((msg==CRCInput::RC_blue)||
				  (msg==CRCInput::RC_setup) ||
				  (CRCInput::isNumeric(msg)) )
		{
			//pushback key if...
			g_RCInput->postMsg( msg, data );
			loop=false;
		}
		else if( msg == CRCInput::RC_help )
		{
			// help key
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
	
	if(CMP3Player::getInstance()->state == CMP3Player::PLAY)
		CMP3Player::getInstance()->stop();

	return(res);
}

//------------------------------------------------------------------------

void CMP3PlayerGui::hide()
{
	if(visible)
	{
		frameBuffer->paintBackgroundBoxRel(x, y, width, height + buttonHeight);
		clearItemID3DetailsLine();
		visible = false;
	}
}

//------------------------------------------------------------------------

void CMP3PlayerGui::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
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

	frameBuffer->paintBoxRel(x,ypos, width-15, fheight, color);
	if(liststart+pos<playlist.size())
	{
		string tmp = playlist[liststart+pos].Filename.substr(playlist[liststart+pos].Filename.rfind("/")+1);
		g_Fonts->menu->RenderString(x+10,ypos+fheight, width-10, tmp.c_str(), color, fheight);
		if(liststart+pos==selected)
			CLCD::getInstance()->showMenuText(0, tmp.c_str() );

	}


}

//------------------------------------------------------------------------

void CMP3PlayerGui::paintHead()
{
	string strCaption = g_Locale->getText("mp3player.head");
	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	frameBuffer->paintIcon("mp3.raw",x+5,y+4);
	g_Fonts->menu_title->RenderString(x+35,y+theight+0, width- 45, strCaption.c_str(), COL_MENUHEAD);
}

//------------------------------------------------------------------------

void CMP3PlayerGui::paintFoot()
{
	int ButtonWidth = (width-28) / 4;
	frameBuffer->paintBoxRel(x,y+height, width,buttonHeight, COL_MENUHEAD);
	frameBuffer->paintHLine(x, x+width,  y, COL_INFOBAR_SHADOW);

	if(playlist.size()>0)
	{
		frameBuffer->paintIcon("rot.raw", x+ width - 4* ButtonWidth - 20, y+height+4);
		g_Fonts->infobar_small->RenderString(x + width - 4* ButtonWidth, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("mp3player.delete").c_str(), COL_INFOBAR);
		
		frameBuffer->paintIcon("ok.raw", x+width- 1* ButtonWidth - 30, y+height);
		g_Fonts->infobar_small->RenderString(x+width-1 * ButtonWidth , y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("mp3player.play").c_str(), COL_INFOBAR);
	}

	frameBuffer->paintIcon("gruen.raw", x+width- 3* ButtonWidth - 30, y+height+4);
	g_Fonts->infobar_small->RenderString(x+width- 3* ButtonWidth - 10, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("mp3player.add").c_str(), COL_INFOBAR);

	if(CMP3Player::getInstance()->state == CMP3Player::PLAY)
	{
		frameBuffer->paintIcon("gelb.raw", x+width- 2* ButtonWidth - 30, y+height+4);
		g_Fonts->infobar_small->RenderString(x+width- 2* ButtonWidth - 10, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("mp3player.stop").c_str(), COL_INFOBAR);
	}
}
//------------------------------------------------------------------------

void CMP3PlayerGui::paint()
{
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-( height+ info_height) ) / 2) + g_settings.screen_StartY;

	liststart = (selected/listmaxshow)*listmaxshow;


	CLCD::getInstance()->setMode(CLCD::MODE_MENU, g_Locale->getText("mp3player.name") );

	paintHead();
	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int ypos = y+ theight;
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
					printf("%s%s: %s\n", &spaces[namelen], name, latin1);
				}
				else 
				{
					if (strcmp(info[i].id, "TCOP") == 0 || strcmp(info[i].id, "TPRO") == 0) 
					{
						printf("%s  %s %s\n", spaces, (info[i].id[1] == 'C') ? ("Copyright (C)") : ("Produced (P)"), latin1);
					}
					else
						printf("%s  %s\n", spaces, latin1);
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
					printf("%s%s: %s\n", &spaces[namelen], name, ptr);
					first = 0;
				}
				else 
					printf("%s  %s\n", spaces, ptr);

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
	#define ConnectLineBox_Width	16

	int xpos  = x - ConnectLineBox_Width;
	int ypos1 = y + theight+0 + pos*fheight;
	int ypos2 = y + height + buttonHeight;
	int ypos1a = ypos1 + (fheight/2)-2;
	int ypos2a = ypos2 + (info_height/2)-2;
	unsigned char col1 = COL_MENUCONTENT+6;
	unsigned char col2 = COL_MENUCONTENT+1;


	// Clear
	frameBuffer->paintBackgroundBoxRel(xpos,y, ConnectLineBox_Width, height + buttonHeight + info_height);
	frameBuffer->paintBackgroundBoxRel(x,ypos2, width,info_height);
	
	// paint Line if detail info (and not valid list pos)
	if(playlist.size() > 0)
	if (pos >= 0 &&  playlist[selected].Title != "")
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
		frameBuffer->paintBoxRel(x,         ypos2, 2,info_height, col1);
		frameBuffer->paintBoxRel(x+width-2, ypos2, 2,info_height, col1);
		frameBuffer->paintBoxRel(x        , ypos2, width-2,2,     col1);
		frameBuffer->paintBoxRel(x        , ypos2+info_height-2, width-2,2, col1);

		// paint id3 infobox 
		frameBuffer->paintBoxRel(x+2, ypos2 +2 , width-4, info_height-4, COL_MENUCONTENTDARK);

		//
		g_Fonts->channellist->RenderString(x+6, ypos2 + g_Fonts->channellist->getHeight(), width- 10, playlist[selected].Title.c_str(), COL_MENUCONTENTDARK);
		g_Fonts->channellist_descr->RenderString(x+6, ypos2 + g_Fonts->channellist_descr->getHeight() * 2, width- 10, playlist[selected].Artist.c_str(), COL_MENUCONTENTDARK);




	}

}
