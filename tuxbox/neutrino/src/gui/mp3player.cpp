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
	buttonHeight = 25;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->menu->getHeight();
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-( height+ info_height) ) / 2) + g_settings.screen_StartY;
	liststart = 0;
	Path = "/";
}

//------------------------------------------------------------------------

CMP3PlayerGui::~CMP3PlayerGui()
{
	playlist.clear();
}

//------------------------------------------------------------------------
int CMP3PlayerGui::exec(CMenuTarget* parent, string actionKey)
{
	if(parent)
	{
		parent->hide();
	}

	int ret = show();

	if( ret > -1)
	{
		return menu_return::RETURN_REPAINT;
	}
	else if( ret == -1)
	{
		// -1 bedeutet nur REPAINT
		return menu_return::RETURN_REPAINT;
	}
	else
	{
		// -2 bedeutet EXIT_ALL
		return menu_return::RETURN_EXIT_ALL;
	}
}

//------------------------------------------------------------------------

int CMP3PlayerGui::show()
{
	int res = -1;

	unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_menu );
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
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_menu );

		if( ( msg == CRCInput::RC_timeout ) ||
			 ( msg == CRCInput::RC_home) )
		{ //Exit after timeout or cancel key
			loop=false;
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
			update=true;
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
			string file = filebrowser.exec(Path);
			Path = file.substr(0,file.rfind("/"));
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
			update=true;
		}
		else if(msg==CRCInput::RC_yellow)
		{
			if(CMP3Player::getInstance()->state == CMP3Player::PLAY)
				CMP3Player::getInstance()->stop();
			update=true;
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
		else
		{
			if( CNeutrinoApp::getInstance()->handleMsg( msg, data ) == messages_return::cancel_all )
			{
				loop = false;
				res = - 2;
			}
		}
	}
	hide();

	return(res);
}

//------------------------------------------------------------------------

void CMP3PlayerGui::hide()
{
	if(visible)
	{
		frameBuffer->paintBackgroundBoxRel(x, y, width, height+ info_height+ 5);
		visible = false;
	}
}

//------------------------------------------------------------------------

void CMP3PlayerGui::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight*2;
	int color;
	if(pos % 2)
		color = COL_MENUCONTENTDARK;
	else
		color	= COL_MENUCONTENT;

	if(liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}

	frameBuffer->paintBoxRel(x,ypos, width-15, 2*fheight, color);
	if(liststart+pos<playlist.size())
	{
		string tmp = playlist[pos].Filename.substr(playlist[pos].Filename.rfind("/"));
		
		g_Fonts->menu->RenderString(x+10,ypos+fheight, width-10, tmp.c_str(), color, fheight);
//			CLCD::getInstance()->showMenuText(0, line1 );
//			CLCD::getInstance()->showMenuText(1, line2 );
	}
}

//------------------------------------------------------------------------

void CMP3PlayerGui::paintHead()
{
	string strCaption = g_Locale->getText("mp3player.name");
	int real_width=width;
	if(playlist.size()<=listmaxshow)
	{
		real_width-=15; //no scrollbar
	}
	frameBuffer->paintBoxRel(x,y, real_width,theight+0, COL_MENUHEAD);
	frameBuffer->paintIcon("mp3.raw",x+5,y+4);
	g_Fonts->menu_title->RenderString(x+35,y+theight+0, real_width- 45, strCaption.c_str(), COL_MENUHEAD);

/*	frameBuffer->paintIcon("help.raw", x+ width- 30, y+ 5 );
	if (bouquetList!=NULL)
		frameBuffer->paintIcon("dbox.raw", x+ width- 60, y+ 5 );*/
}

//------------------------------------------------------------------------

void CMP3PlayerGui::paintFoot()
{
	int real_width=width;
	if(playlist.size()<=listmaxshow)
	{
		real_width-=15; //no scrollbar
	}
	int ButtonWidth = (real_width-28) / 4;
	frameBuffer->paintBoxRel(x,y+height, real_width,buttonHeight, COL_MENUHEAD);
	frameBuffer->paintHLine(x, x+real_width,  y, COL_INFOBAR_SHADOW);

	if(playlist.size()>0)
	{
		frameBuffer->paintIcon("rot.raw", x+real_width- 4* ButtonWidth - 20, y+height+4);
		g_Fonts->infobar_small->RenderString(x+real_width- 4* ButtonWidth, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("mp3player.delete").c_str(), COL_INFOBAR);
		
		if(CMP3Player::getInstance()->state == CMP3Player::STOP)
		{
			frameBuffer->paintIcon("ok.raw", x+real_width- 1* ButtonWidth - 30, y+height);
			g_Fonts->infobar_small->RenderString(x+real_width-1 * ButtonWidth , y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("mp3player.play").c_str(), COL_INFOBAR);
		}
	}

	frameBuffer->paintIcon("gruen.raw", x+real_width- 3* ButtonWidth - 30, y+height+4);
	g_Fonts->infobar_small->RenderString(x+real_width- 3* ButtonWidth - 10, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("mp3player.add").c_str(), COL_INFOBAR);

	if(CMP3Player::getInstance()->state == CMP3Player::PLAY)
	{
		frameBuffer->paintIcon("gelb.raw", x+real_width- 2* ButtonWidth - 30, y+height+4);
		g_Fonts->infobar_small->RenderString(x+real_width- 2* ButtonWidth - 10, y+height+24 - 2, ButtonWidth- 26, g_Locale->getText("mp3player.stop").c_str(), COL_INFOBAR);
	}
}
//------------------------------------------------------------------------

void CMP3PlayerGui::paint()
{
	height = (g_settings.screen_EndY-g_settings.screen_StartY)-(info_height+50);
	listmaxshow = (height-theight-0)/(fheight*2);
	height = theight+0+listmaxshow*fheight*2;	// recalc height
	if(playlist.size() < listmaxshow)
	{
		listmaxshow=playlist.size();
		height = theight+0+listmaxshow*fheight*2;	// recalc height
	}
	if(selected==playlist.size() && playlist.size()!=0)
	{
		selected=playlist.size()-1;
		liststart = (selected/listmaxshow)*listmaxshow;
	}
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-( height+ info_height) ) / 2) + g_settings.screen_StartY;

	liststart = (selected/listmaxshow)*listmaxshow;

	CLCD::getInstance()->setMode(CLCD::MODE_MENU, g_Locale->getText("mp3player.name") );

	paintHead();
	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	if(playlist.size()>listmaxshow)
	{
		int ypos = y+ theight;
		int sb = 2*fheight* listmaxshow;
		frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT+ 1);

		int sbc= ((playlist.size()- 1)/ listmaxshow)+ 1;
		float sbh= (sb- 4)/ sbc;
		int sbs= (selected/listmaxshow);

		frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);
	}

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
	/*
		if (name)
		  name = gettext(name);
	*/
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
