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

#include <sys/stat.h>

//------------------------------------------------------------------------

int CFile::getType()
{
	if(S_ISDIR(Mode))
		return FILE_DIR;

	int ext_pos = Name.rfind(".");
	if( ext_pos > 0)
	{
		string extension;
		extension = Name.substr(ext_pos + 1, Name.length() - ext_pos);
		if((strcasecmp(extension.c_str(),"mp3") == 0) || (strcasecmp(extension.c_str(),"m2a") == 0) ||
			(strcasecmp(extension.c_str(),"mpa") == 0) )
			return FILE_MP3;
      if((strcasecmp(extension.c_str(),"m3u") == 0))
         return FILE_MP3_PLAYLIST;
		if((strcasecmp(extension.c_str(),"txt") == 0) || (strcasecmp(extension.c_str(),"sh") == 0))
			return FILE_TEXT;
		if((strcasecmp(extension.c_str(),"jpg") == 0) || (strcasecmp(extension.c_str(),"png") == 0) || 
			(strcasecmp(extension.c_str(),"bmp") == 0) || (strcasecmp(extension.c_str(),"gif") == 0))
			return FILE_PICTURE;
	}
	return FILE_UNKNOWN;
}

//------------------------------------------------------------------------

string CFile::getFileName()		// return name.extension or folder name without trailing /
{
	int namepos = Name.rfind("/");
	if( namepos >= 0)
	{
		return Name.substr(namepos + 1, Name.length() - namepos);
	}
	else
		return Name;
	
}

//------------------------------------------------------------------------

string CFile::getPath()			// return complete path including trailing /
{
	int pos = 0;
	string tpath;

	if((pos = Name.rfind("/")) > 1)
	{
		tpath = Name.substr(0,pos+1);
	}else
		tpath = "/";
	return (tpath);
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

bool sortByType (const CFile& a, const CFile& b)
{
	if(a.Mode == b.Mode)
		return a.Name > b.Name;
	else
		return a.Mode < b.Mode ;
}

bool sortByDate (const CFile& a, const CFile& b)
{
	return a.Time < b.Time ;
}

//------------------------------------------------------------------------

CFileBrowser::CFileBrowser()
{
	frameBuffer = CFrameBuffer::getInstance();

	Filter = NULL;
	use_filter = true;
	Multi_Select = false;
	Dirs_Selectable = false;
	Dir_Mode = false;
	selected = 0;
	smode = 0;

	width  = 500;
	height = 380;
   if((g_settings.screen_EndX- g_settings.screen_StartX) < width)
      width=(g_settings.screen_EndX- g_settings.screen_StartX);
	if((g_settings.screen_EndY- g_settings.screen_StartY) < height)
      height=(g_settings.screen_EndY- g_settings.screen_StartY);
	
	foheight = 30;

	theight  = g_Fonts->eventlist_title->getHeight();
	fheight = g_Fonts->filebrowser_item->getHeight();


	listmaxshow = max(1,(int)(height - theight - foheight)/fheight);

	height = theight+foheight+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	if(x+width > 720)
		x=720-width;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	if(y+height > 576)
		x=576-width;
	
	liststart = 0;
}

//------------------------------------------------------------------------


CFileBrowser::~CFileBrowser()
{
}

//------------------------------------------------------------------------

CFile *CFileBrowser::getSelectedFile()
{
	if(filelist[selected].Name.length() > 0)
		return &filelist[selected];
	else
		return NULL;
}

//------------------------------------------------------------------------

CFileList *CFileBrowser::getSelectedFiles()
{
	return &selected_filelist;
}

//------------------------------------------------------------------------

void CFileBrowser::ChangeDir(string filename)
{
	if(filename == "..")
	{
		int pos = (Path.substr(0,Path.length()-1)).rfind("/");
		string newpath = Path.substr(0,pos);
//		printf("path: %s filename: %s newpath: %s\n",Path.c_str(),filename.c_str(),newpath.c_str());
		readDir(newpath);
		name = newpath;
	}
	else
	{
		readDir( filename );
		name = filename;
	}
	paintHead();
	paint();
}

//------------------------------------------------------------------------

bool CFileBrowser::readDir(string dirname)
{
struct stat statbuf;
struct dirent **namelist;
int n;

	Path = dirname;
	
	if(dirname[dirname.length()-1] != '/')
	{
		Path = Path + "/";
	}

	filelist.clear();
	n = scandir(Path.c_str(), &namelist, 0, alphasort);
	if (n < 0)
	{
		perror(("Filebrowser scandir: "+Path).c_str());
		Path = "/";
		name = "/";
		paintHead();
		n = scandir(Path.c_str(), &namelist, 0, alphasort);
	}
	for(int i = 0; i < n;i++)
	{
		CFile file;
		if(strcmp(namelist[i]->d_name,".") != 0)
		{
			file.Name = Path + namelist[i]->d_name;
//			printf("file.Name: '%s', getFileName: '%s' getPath: '%s'\n",file.Name.c_str(),file.getFileName().c_str(),file.getPath().c_str());
			if(stat((file.Name).c_str(),&statbuf) != 0)
				perror("stat error");
			file.Mode = statbuf.st_mode;
			file.Size = statbuf.st_size;
			file.Time = statbuf.st_mtime;

			if(Filter != NULL && (!S_ISDIR(file.Mode)) && use_filter)
				if(!Filter->matchFilter(file.Name))
				{
					free(namelist[i]);
					continue;
				}
			if(Dir_Mode && (!S_ISDIR(file.Mode)))
			{
				free(namelist[i]);
				continue;
			}
			filelist.push_back(file);
		}
		free(namelist[i]);
	}

	free(namelist);

	if( smode == 0 )
		sort(filelist.begin(), filelist.end(), sortByName);
	else
		sort(filelist.begin(), filelist.end(), sortByDate);

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
			if(Multi_Select)
			{
				if(filelist[selected].getFileName() != "..")
				{
					if( (S_ISDIR(filelist[selected].Mode) && Dirs_Selectable) || !S_ISDIR(filelist[selected].Mode) )
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
			if(S_ISDIR(filelist[selected].Mode) && (filelist[selected].getFileName() != "..") )
			{
					ChangeDir(filelist[selected].Name);
			}
		}
		else if ( msg == CRCInput::RC_left )
		{
			ChangeDir("..");
		}
		else if ( msg == CRCInput::RC_blue )
		{
			if(Filter != NULL)
			{
				use_filter = !use_filter;
				ChangeDir(Path);
			}
		}
		else if ( msg == CRCInput::RC_home )
		{
			loop = false;
		}
		else if (msg==CRCInput::RC_ok)
		{
			
			if( filelist[selected].getFileName() == "..")
			{
				ChangeDir("..");
			}
			else
			{
				string filename = filelist[selected].Name;
				if ( filename.length() > 1 )
				{
					if((!Multi_Select) && S_ISDIR(filelist[selected].Mode) && !Dir_Mode)
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
		else if (msg==CRCInput::RC_help)
		{
			smode++;
			if( smode > 1 )
				smode = 0;
			if( smode == 0 )
				sort(filelist.begin(), filelist.end(), sortByName);
			else
				sort(filelist.begin(), filelist.end(), sortByDate);
			paint();
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

	if(res && Multi_Select)
	{
		CProgressWindow * progress = new CProgressWindow();
		progress->setTitle(g_Locale->getText("filebrowser.scan"));
		progress->exec(NULL,"");
		for(unsigned int i = 0; i < filelist.size();i++)
			if(filelist[i].Marked)
			{
				if(S_ISDIR(filelist[i].Mode))
					addRecursiveDir(&selected_filelist,filelist[i].Name, progress);
				else
					selected_filelist.push_back(filelist[i]);
			}
		progress->hide();
		delete progress;
	}

	#ifdef USEACTIONLOG
		g_ActionLog->println("Filebrowser: closed");
	#endif

	return res;
}

//------------------------------------------------------------------------

void CFileBrowser::addRecursiveDir(CFileList * re_filelist, string rpath, CProgressWindow * progress)
{
struct stat statbuf;
struct dirent **namelist;
int n;
CFile file;

	if(rpath[rpath.length()-1]!='/')
	{
		rpath = rpath + "/";
	}

	
	n = scandir(rpath.c_str(), &namelist, 0, alphasort);
	if (n < 0)
		perror(("Recursive scandir: "+rpath).c_str());
	else 
	{
		if(progress)
		{
			progress->showStatusMessage(rpath);
		}
		for(int i = 0; i < n;i++)
		{
			if(progress)
			{
				progress->showGlobalStatus(100/n*i);
			}
			if( (strcmp(namelist[i]->d_name,".") != 0) && (strcmp(namelist[i]->d_name,"..") != 0) )
			{
				file.Name = rpath + namelist[i]->d_name;
				if(stat((file.Name).c_str(),&statbuf) != 0)
					perror("stat error");
				file.Mode = statbuf.st_mode;
				file.Size = statbuf.st_size;
				file.Time = statbuf.st_mtime;

				if(Filter != NULL && (!S_ISDIR(file.Mode)) && use_filter)
				{
					if(!Filter->matchFilter(file.Name))
					{
						free(namelist[i]);
						continue;
					}
				}
				if(!S_ISDIR(file.Mode))
					re_filelist->push_back(file);
				else
					addRecursiveDir(re_filelist,file.Name, progress);
			}
			free(namelist[i]);
		}
		free(namelist);
	}
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
		color = COL_MENUCONTENT;//DARK;
	}
	
	if( (liststart + pos) <filelist.size() )
	{
		actual_file = &filelist[liststart+pos];
		if(actual_file->Marked)
			color = color+2;

		frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, color);

		if ( actual_file->Name.length() > 0 )
		{
			if (liststart+pos==selected)
				CLCD::getInstance()->showMenuText(0, actual_file->getFileName().c_str() );
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
			frameBuffer->paintIcon(fileicon, x+5 , ypos + (fheight-16) / 2 );
			
			g_Fonts->filebrowser_item->RenderString(x+35, ypos+ fheight, width -(35+170) , actual_file->getFileName().c_str(), color);

			if( S_ISREG(actual_file->Mode) )
			{
				string modestring;
				for(int m = 2; m >=0;m--)
				{
					modestring += string((actual_file->Mode & (4 << (m*3)))?"r":"-");
					modestring += string((actual_file->Mode & (2 << (m*3)))?"w":"-");
					modestring += string((actual_file->Mode & (1 << (m*3)))?"x":"-");
				}
				g_Fonts->filebrowser_item->RenderString(x + width - 160 , ypos+ fheight, 80, modestring.c_str(), color);

				char tmpstr[256];
				snprintf(tmpstr,sizeof(tmpstr),"%d kb", (int)((actual_file->Size / 1024) +0.9));
				int breite = g_Fonts->filebrowser_item->getRenderWidth(tmpstr)< 70?g_Fonts->filebrowser_item->getRenderWidth(tmpstr):60;
				g_Fonts->filebrowser_item->RenderString(x + width - 80 + (60 - breite), ypos+ fheight, breite, tmpstr, color);
			}
			if( S_ISDIR(actual_file->Mode) )
			{
				char timestring[18];
				time_t rawtime;

				rawtime = actual_file->Time;
				strftime(timestring, 18, "%d-%m-%Y %H:%M", gmtime(&rawtime));
				int breite = g_Fonts->filebrowser_item->getRenderWidth(timestring);
				g_Fonts->filebrowser_item->RenderString(x + width - 20 - breite , ypos+ fheight, breite+1, timestring, color);
			}
		}
	}
	else
		frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, COL_MENUCONTENT/*DARK*/);
}

//------------------------------------------------------------------------

void CFileBrowser::paintHead()
{
	char l_name[100];
	snprintf(l_name, sizeof(l_name), "%s %s", CZapitClient::Utf8_to_Latin1(g_Locale->getText("filebrowser.head")).c_str(), name.c_str());  // <- FIXME (UTF-8 and Latin combination)

	frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->eventlist_title->RenderString(x+10,y+theight+1, width-11, l_name, COL_MENUHEAD);

}

//------------------------------------------------------------------------

void CFileBrowser::paintFoot()
{
//	int ButtonWidth = 25;
	int dx = width / 4;
	int type;
	int by = y + height - (foheight -4);
	int ty = by + g_Fonts->infobar_small->getHeight();

	frameBuffer->paintBoxRel(x, y+height- (foheight ), width, (foheight ), COL_MENUHEAD);

	if(filelist.size()>0)
	{
		string nextsort;
		type = filelist[selected].getType();

		if( (type != CFile::FILE_UNKNOWN) || (S_ISDIR(filelist[selected].Mode)) )
		{
			frameBuffer->paintIcon("ok.raw", x +3 , by -3);
			g_Fonts->infobar_small->RenderString(x + 35, ty, dx - 35, g_Locale->getText("filebrowser.select").c_str(), COL_INFOBAR, 0, true); // UTF-8
		}

		frameBuffer->paintIcon("help.raw", x + (1 * dx), by -3);
		if( smode == 1 )
			nextsort = g_Locale->getText("filebrowser.sort.name");
		else
			nextsort = g_Locale->getText("filebrowser.sort.date");
		g_Fonts->infobar_small->RenderString(x + 35 + (1 * dx), ty, dx - 35, nextsort.c_str(), COL_INFOBAR, 0, true); // UTF-8

		if(Multi_Select)
		{
			frameBuffer->paintIcon("gelb.raw", x + (2 * dx), by);
			g_Fonts->infobar_small->RenderString(x + 25 + (2 * dx), ty, dx - 25, g_Locale->getText("filebrowser.mark").c_str(), COL_INFOBAR, 0, true); // UTF-8
			
		}

		if(Filter != NULL)
		{
			frameBuffer->paintIcon("blau.raw", x + (3 * dx), by);
			g_Fonts->infobar_small->RenderString(x + 25 + (3 * dx), ty, dx - 25, use_filter?g_Locale->getText("filebrowser.filter.inactive").c_str():g_Locale->getText("filebrowser.filter.active").c_str(), COL_INFOBAR, 0, true); // UTF-8
		}
	}
}

//------------------------------------------------------------------------

void CFileBrowser::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

//	if (filelist[0].Name.length() != 0)
//		frameBuffer->paintIcon("help.raw", x+ width- 30, y+ 5 );
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, g_Locale->getText("filebrowser.head") );

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

