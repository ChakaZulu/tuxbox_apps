/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
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

#include <algorithm>
#include <global.h>
#include <neutrino.h>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include "filebrowser.h"

// sort operators
bool sortByName (const CFile& a, const CFile& b)
{
	if(a.Name == b.Name)
		return a.Mode < b.Mode;
	else
		return a.Name < b.Name ;
}

bool sortByType (const CFile& a, const CFile& b)
{
	if(a.Mode == b.Mode)
		return a.Name > b.Name;
	else
		return a.Mode < b.Mode ;
}

CFileBrowser::CFileBrowser()
{
	frameBuffer = CFrameBuffer::getInstance();
	selected = 0;

	width  = 500;
	height = 380;
	theight  = g_Fonts->eventlist_title->getHeight();
	fheight = g_Fonts->eventlist_itemLarge->getHeight();

	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	
	liststart = 0;
}


CFileBrowser::~CFileBrowser()
{
}


bool CFileBrowser::readDir(string dirname)
{
struct stat statbuf;
struct dirent **namelist;
int n;

	path = dirname;

	if(dirname[dirname.length()-1]!='/')
		path = path + "/";

	filelist.clear();
	n = scandir(path.c_str(), &namelist, 0, alphasort);
	if (n < 0)
		perror("Filebrowser scandir");
	else 
	{
		for(int i = 0; i < n;i++)
		{
		CFile file;
			if(strcmp(namelist[i]->d_name,".") != 0)
			{
				file.Name = namelist[i]->d_name;
				if(stat((path + file.Name).c_str(),&statbuf) != 0)
					perror("stat error");
				file.Mode = statbuf.st_mode;
				file.Size = statbuf.st_size;
				filelist.push_back(file);
			}
			free(namelist[i]);
		}
		free(namelist);
	}
//	sort(filelist.begin(),filelist.end(),sortByType);

	selected = 0;
	return true;
}


string CFileBrowser::exec(const std::string& dirname)
{

	string res;

	name = dirname;

	paintHead();
	readDir(dirname);
	paint();

	int oldselected = selected;

	#ifdef USEACTIONLOG
		char buf[1000];
		sprintf((char*) buf, "Filebrowser: \"%s\"", dirname.c_str() );
		g_ActionLog->println(buf);
	#endif

	uint msg; uint data;
	unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_chanlist );

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_chanlist );

		if ( msg == (uint) g_settings.key_channelList_pageup )
		{
			selected+=listmaxshow;
			if (selected>filelist.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if ( msg == (uint) g_settings.key_channelList_pagedown )
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=filelist.size()-1;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if ( msg == CRCInput::RC_up )
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = filelist.size()-1;
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
		else if ( msg == CRCInput::RC_down )
		{
			int prevselected=selected;
			selected = (selected+1)%filelist.size();
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
		else if ( ( msg == CRCInput::RC_timeout ) ||
			 	  ( msg == (uint) g_settings.key_channelList_cancel ) )
		{
			selected = oldselected;
			loop=false;
		}

		else if ( ( msg == CRCInput::RC_help ) ||
				  ( msg == CRCInput::RC_red ) )
		{
			loop= false;
		}
		else if ( msg == CRCInput::RC_right )
		{
			if(S_ISDIR(filelist[selected].Mode))
			{
					ChangeDir(filelist[selected].Name);
			}
		}
		else if ( msg == CRCInput::RC_left )
		{
			if(S_ISDIR(filelist[selected].Mode))
			{
					ChangeDir("..");
			}
		}
		else if (msg==CRCInput::RC_ok)
		{
			string filename = filelist[selected].Name;
			if ( filename.length() > 1 )
			{
//				printf("Ausgewaehlt: '%s'\n",filename.c_str());
				if(S_ISDIR(filelist[selected].Mode))
				{
					ChangeDir(filelist[selected].Name);
				}
				else
				{
					loop = false;
					res = path + filename;
					current_file = filelist[selected];
				}


			}
		}
		else
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
			}
		}
	}

	hide();

	#ifdef USEACTIONLOG
		g_ActionLog->println("Filebrowser: closed");
	#endif

	return res;
}

void CFileBrowser::ChangeDir(string filename)
{
	if(filename == "..")
	{
		int pos = path.substr(0,path.length()-1).rfind("/");
		string newpath = path.substr(0,pos);
		readDir(newpath);
	}
	else
	{
		readDir( path + filename );
	}
	name = filename;
	paintHead();
	paint();
}

void CFileBrowser::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CFileBrowser::paintItem(unsigned int pos, unsigned int spalte)
{
	int color;
	int ypos = y+ theight+0 + pos*fheight;

	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}
	else
	{
		color = COL_MENUCONTENTDARK;
	}

	CFile * actual_file;
	actual_file = &filelist[liststart+pos];

	frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, color);

	if( (liststart + pos) <filelist.size() )
	{
		if ( actual_file->Name.length() != 0 )
		{
			if( S_ISDIR(actual_file->Mode) )
			{
				//paint folder icon
				frameBuffer->paintIcon("folder.raw", x+5 , ypos +7 );

				g_Fonts->filebrowser_itemFolder->RenderString(x+35, ypos+ fheight+3, width-20, actual_file->Name.c_str(), color);
			}
			else
			{
				// paint file icon
				frameBuffer->paintIcon("file.raw", x+5 , ypos +7 );

				int ext_pos = actual_file->Name.rfind(".");
				if( ext_pos > 0)
				{
					string extension;
					extension = actual_file->Name.substr(ext_pos + 1, actual_file->Name.length() - ext_pos);
					if(extension == "mp3")
					{
						frameBuffer->paintIcon("mp3.raw", x+5 , ypos +7 );
						color = COL_MENUCONTENT;
					}
				}

				g_Fonts->filebrowser_itemFile->RenderString(x+35, ypos+ fheight+3, width-40, actual_file->Name.c_str(), color);
				char tmpstr[256];
				snprintf(tmpstr,sizeof(tmpstr),"%d kb", (int)((actual_file->Size / 1024) +0.9));
				g_Fonts->filebrowser_itemFile->RenderString(width-40, ypos+ fheight+3, width, tmpstr, color);
			}
		}
	}
}

void CFileBrowser::paintHead()
{
	char l_name[100];
	snprintf(l_name, sizeof(l_name), name.c_str(), g_Locale->getText("filebrowser.head").c_str()  );

	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->eventlist_title->RenderString(x+10,y+theight+1, width, l_name, COL_MENUHEAD);

}

void CFileBrowser::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

	if (filelist[0].Name.length() != 0)
		frameBuffer->paintIcon("help.raw", x+ width- 30, y+ 5 );

	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT+ 1);

	int sbc= ((filelist.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);
}
