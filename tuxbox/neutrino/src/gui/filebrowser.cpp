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


int CFile::getType()
{
	if(S_ISDIR(Mode))
		return FILE_DIR;

	int ext_pos = Name.rfind(".");
	if( ext_pos > 0)
	{
		string extension;
		extension = Name.substr(ext_pos + 1, Name.length() - ext_pos);
		if(extension == "mp3")
			return FILE_MP3;
		if((strcasecmp(extension.c_str(),"txt") == 0) || (strcasecmp(extension.c_str(),"sh") == 0))
			return FILE_TEXT;
		if((strcasecmp(extension.c_str(),"jpg") == 0) || (strcasecmp(extension.c_str(),"png") == 0) || (strcasecmp(extension.c_str(),"bmp") == 0))
			return FILE_PICTURE;
	}
	return FILE_UNKNOWN;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------

// sort operators
bool sortByName (const CFile& a, const CFile& b)
{
	if(a.Name == b.Name)
		return a.Mode < b.Mode;
	else
		return a.Name < b.Name ;
}

//------------------------------------------------------------------------

bool sortByType (const CFile& a, const CFile& b)
{
	if(a.Mode == b.Mode)
		return a.Name > b.Name;
	else
		return a.Mode < b.Mode ;
}

//------------------------------------------------------------------------

CFileBrowser::CFileBrowser()
{
	frameBuffer = CFrameBuffer::getInstance();

	filter = NULL;
	use_filter = true;
	multi_select = false;
	select_dirs = false;
	selected = 0;

	width  = 500;
	height = 380;
	foheight = 30;

	theight  = g_Fonts->eventlist_title->getHeight();
	fheight = g_Fonts->eventlist_itemLarge->getHeight();


	listmaxshow = (height - theight - foheight)/fheight;
	height = theight+foheight+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	
	liststart = 0;
}

//------------------------------------------------------------------------


CFileBrowser::~CFileBrowser()
{
}

//------------------------------------------------------------------------

string CFileBrowser::getFileName()
{
	if(filelist[selected].Name.length() > 0)
		return path + filelist[selected].Name;
	else
		return "";
}

//------------------------------------------------------------------------

CFileList *CFileBrowser::getSelectedFiles()
{
	return &selected_filelist;
}

//------------------------------------------------------------------------

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
		perror(("Filebrowser scandir: "+path).c_str());
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

				if(filter != NULL && (!S_ISDIR(file.Mode)) && use_filter)
					if(!filter->matchFilter(file.Name))
					{
						free(namelist[i]);
						continue;
					}
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

//------------------------------------------------------------------------

bool CFileBrowser::exec(string Dirname)
{
	bool res = false;

	name = Dirname;
	paintHead();
	readDir(Dirname);
	paint();
	paintFoot();

	int oldselected = selected;

	#ifdef USEACTIONLOG
		char buf[1000];
		sprintf((char*) buf, "Filebrowser: \"%s\"", dirname.c_str() );
		g_ActionLog->println(buf);
	#endif

	uint msg; uint data;
	unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_filebrowser );

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_filebrowser );

		if ( msg == CRCInput::RC_yellow )
		{
			if(multi_select)
			{
				if(filelist[selected].Name != "..")
				{
					if( (S_ISDIR(filelist[selected].Mode) && select_dirs) || !S_ISDIR(filelist[selected].Mode) )
					{
						filelist[selected].Marked = !filelist[selected].Marked;
						paintItem(selected - liststart);
					}
				}
				msg = CRCInput::RC_down;	// jump to next item
			}
		}

		if ( msg == CRCInput::RC_red )
		{
			selected+=listmaxshow;
			if (selected>filelist.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if ( msg == CRCInput::RC_green )
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
		else if ( ( msg == CRCInput::RC_timeout ) )
		{
			selected = oldselected;
			loop=false;
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
		else if ( msg == CRCInput::RC_blue )
		{
			if(filter != NULL)
			{
				use_filter = !use_filter;
				ChangeDir("");
			}
		}
		else if ( msg == CRCInput::RC_home )
		{
			if(multi_select)
				res = true;
			loop = false;
		}
		else if (msg==CRCInput::RC_ok)
		{
			if(multi_select)
			{
				if(filelist[selected].Name != "..")
				{
					if( (S_ISDIR(filelist[selected].Mode) && select_dirs) || !S_ISDIR(filelist[selected].Mode) )
					{
						filelist[selected].Marked = !filelist[selected].Marked;
						paintItem(selected - liststart);
					}
				}
			}
			else
			{
				string filename = filelist[selected].Name;
				if ( filename.length() > 1 )
				{
					if(S_ISDIR(filelist[selected].Mode))
					{
						ChangeDir(filelist[selected].Name);
					}
					else
					{
						filelist[selected].Marked = true;
						loop = false;
						res = true;
					}
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

	selected_filelist.clear();

	if(res && multi_select)
	{
		for(unsigned int i = 0; i < filelist.size();i++)
			if(filelist[i].Marked)
			{
				if(S_ISDIR(filelist[i].Mode))
					addRecursiveDir(&selected_filelist,filelist[i].Name);
				else
					selected_filelist.push_back(filelist[i]);
			}
	}

	#ifdef USEACTIONLOG
		g_ActionLog->println("Filebrowser: closed");
	#endif

	return res;
}

void CFileBrowser::addRecursiveDir(CFileList * re_filelist, string path)
{
struct stat statbuf;
struct dirent **namelist;
int n;

	if(path[path.length()-1]!='/')
		path = path + "/";

	n = scandir(path.c_str(), &namelist, 0, alphasort);
	if (n < 0)
		perror(("Recursive scandir: "+path).c_str());
	else 
	{
		for(int i = 0; i < n;i++)
		{
		CFile file;
			if( (strcmp(namelist[i]->d_name,".") != 0) && (strcmp(namelist[i]->d_name,"..") != 0) )
			{
				file.Name = path + namelist[i]->d_name;
				if(stat((file.Name).c_str(),&statbuf) != 0)
					perror("stat error");
				file.Mode = statbuf.st_mode;
				file.Size = statbuf.st_size;

				if(filter != NULL && (!S_ISDIR(file.Mode)) && use_filter)
				{
					if(!filter->matchFilter(file.Name))
					{
						free(namelist[i]);
						continue;
					}
				}
				if(!S_ISDIR(file.Mode))
					re_filelist->push_back(file);
				else
					addRecursiveDir(re_filelist,file.Name);
			}
			free(namelist[i]);
		}
		free(namelist);
	}
}

//------------------------------------------------------------------------

void CFileBrowser::ChangeDir(string filename)
{
	if(filename == "..")
	{
		int pos = path.substr(0,path.length()-1).rfind("/");
		string newpath = path.substr(0,pos);
		readDir(newpath);
		name = newpath;
	}
	else
	{
		readDir( path + filename );
		name = filename;
	}
	paintHead();
	paint();
}

//------------------------------------------------------------------------

void CFileBrowser::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

//------------------------------------------------------------------------

void CFileBrowser::paintItem(unsigned int pos, unsigned int spalte)
{
	int color;
	int ypos = y+ theight+0 + pos*fheight;
	CFile * actual_file = NULL;
	string fileicon;


	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
		paintFoot();
	}
	else
	{
		color = COL_MENUCONTENTDARK;
	}
	
	if( (liststart + pos) <filelist.size() )
	{
		actual_file = &filelist[liststart+pos];
		if(actual_file->Marked)
			color = color+1;

		frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, color);

		if ( actual_file->Name.length() > 0 )
		{
			if (liststart+pos==selected)
				CLCD::getInstance()->showMenuText(0, actual_file->Name.c_str() );
			switch(actual_file->getType())
			{
				case CFile::FILE_MP3 : 
						fileicon = "mp3.raw";
//						color = COL_MENUCONTENT;
					break;
				case CFile::FILE_DIR : 
						fileicon = "folder.raw";
					break;
				case CFile::FILE_PICTURE:
				case CFile::FILE_TEXT:
				default:
						fileicon = "file.raw";
			}
			frameBuffer->paintIcon(fileicon, x+5 , ypos +7 );
			
			g_Fonts->filebrowser_itemFile->RenderString(x+35, ypos+ fheight+3, width -(35+170) , actual_file->Name.c_str(), color);

			if( S_ISREG(actual_file->Mode) )
			{
				string modestring;
				for(int m = 2; m >=0;m--)
				{
					modestring += string((actual_file->Mode & (4 << (m*3)))?"r":"-");
					modestring += string((actual_file->Mode & (2 << (m*3)))?"w":"-");
					modestring += string((actual_file->Mode & (1 << (m*3)))?"x":"-");
				}
				g_Fonts->filebrowser_itemFile->RenderString(x + width - 160 , ypos+ fheight+3, 80, modestring.c_str(), color);

				char tmpstr[256];
				snprintf(tmpstr,sizeof(tmpstr),"%d kb", (int)((actual_file->Size / 1024) +0.9));
				int breite = g_Fonts->filebrowser_itemFile->getRenderWidth(tmpstr)< 70?g_Fonts->filebrowser_itemFile->getRenderWidth(tmpstr):60;
				g_Fonts->filebrowser_itemFile->RenderString(x + width - 80 + (60 - breite), ypos+ fheight+3, breite, tmpstr, color);
			}
		}
	}
	else
		frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, COL_MENUCONTENTDARK);
}

//------------------------------------------------------------------------

void CFileBrowser::paintHead()
{
	char l_name[100];
	snprintf(l_name, sizeof(l_name), "%s %s", g_Locale->getText("filebrowser.head").c_str(), name.c_str());

	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->eventlist_title->RenderString(x+10,y+theight+1, width, l_name, COL_MENUHEAD);

}

//------------------------------------------------------------------------

void CFileBrowser::paintFoot()
{
//	int ButtonWidth = 25;
	int dx = width / 4;
	int type = filelist[selected].getType();
	int by = y + height - (foheight -8);
	int ty = by + g_Fonts->infobar_small->getHeight();

	frameBuffer->paintBoxRel(x, y+height- (foheight -5), width, (foheight -5), COL_MENUHEAD);

	if(filelist.size()>0)
	{
		if( (type != CFile::FILE_UNKNOWN) || (S_ISDIR(filelist[selected].Mode)) )
		{
			frameBuffer->paintIcon("ok.raw", x +3 , by -3);
			g_Fonts->infobar_small->RenderString(x + 35, ty, dx - 35, g_Locale->getText(multi_select?"filebrowser.mark":"filebrowser.select").c_str(), COL_INFOBAR);
		}

		if(multi_select)
		{
			frameBuffer->paintIcon("home.raw", x + (1 * dx), by - 3);
			g_Fonts->infobar_small->RenderString(x + 35 + (1 * dx), ty, dx - 35, g_Locale->getText("filebrowser.commit").c_str(), COL_INFOBAR);

			frameBuffer->paintIcon("gelb.raw", x + (2 * dx), by);
			g_Fonts->infobar_small->RenderString(x + 25 + (2 * dx), ty, dx - 25, g_Locale->getText("filebrowser.mark").c_str(), COL_INFOBAR);
			
		}

		if(filter != NULL)
		{
			frameBuffer->paintIcon("blau.raw", x + (3 * dx), by);
			g_Fonts->infobar_small->RenderString(x + 25 + (3 * dx), ty, dx - 25, g_Locale->getText("filebrowser.filter").c_str(), COL_INFOBAR);
		}
	}
}

//------------------------------------------------------------------------

void CFileBrowser::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

//	if (filelist[0].Name.length() != 0)
//		frameBuffer->paintIcon("help.raw", x+ width- 30, y+ 5 );
	CLCD::getInstance()->setMode(CLCD::MODE_MENU, g_Locale->getText("filebrowser.head") );

	for(unsigned int count=0;count<listmaxshow;count++)
		paintItem(count);

	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT+ 1);

	int sbc= ((filelist.size()- 1)/ listmaxshow)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT+ 3);
}
