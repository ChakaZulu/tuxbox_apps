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

$Id: gamelist.cpp,v 1.19 2001/12/12 18:45:39 McClean Exp $

$Log: gamelist.cpp,v $
Revision 1.19  2001/12/12 18:45:39  McClean
fix gamelist-design, manual-update bug, add save settings now

Revision 1.18  2001/12/05 21:38:09  rasc
gamelist: eigener Fontdef fuer 2-zeiliges Menue


*/

#include "gamelist.h"
#include "../include/debug.h"
#include "../global.h"
#include <config.h>

// hi McClean - hab schon mal was geaendert - falls es nicht gefaellt
// altes gamelist.cpp ist als svd.gamelist.cpp eingecheckt.


CGameList::CGameList(string Name)
{
	name = Name;
	selected = 0;
	width = 500;
	height = 440;
	theight= g_Fonts->menu_title->getHeight();
//
	fheight1= g_Fonts->gamelist_itemLarge->getHeight();
	fheight2= g_Fonts->gamelist_itemSmall->getHeight();
	fheight = fheight1 + fheight2 + 2;
//
	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	liststart = 0;
}

CGameList::~CGameList()
{
	for(unsigned int count=0;count<gamelist.size();count++)
	{
		delete gamelist[count];
	}
	gamelist.clear();
}

static	int	_loadInfo( const char *fname, struct SPluginInfo *info )
{
	FILE	*fp;
	char	buffer[ 512 ];
	char	*p;

	*info->name=0;
	*info->depend=0;
	*info->desc=0;

	fp=fopen( fname , "r" );
	if ( !fp )
		return -1;
	while( fgets(buffer,512,fp) )
	{
		if ( !*buffer )
			continue;
		p=strchr(buffer,'\n');
		if ( p )
			*p=0;
		p=strchr(buffer,'=');
		if ( !p )
			continue;
		*p=0;
		p++;
		if ( !strcmp(buffer,"name") )
			strcpy(info->name,p);
		else if ( !strcmp(buffer,"desc") )
			strcpy(info->desc,p);
		else if ( !strcmp(buffer,"depend") )
			strcpy(info->depend,p);
// rest ist erstma egal
	}
	fclose(fp);

	if ( !*info->name || !*info->desc )
		return -2;
	return 0;
}

//void CGameList::exec()
int CGameList::exec(CMenuTarget* parent, string actionKey)
{
	if (parent)
	{
		parent->hide();
	}

	paintHead();

	//scan4games here!
	for(unsigned int count=0;count<gamelist.size();count++)
	{
		delete gamelist[count];
	}
	gamelist.clear();

	game* tmp = new game();
    tmp->name = g_Locale->getText("menu.back");
	gamelist.insert(gamelist.end(), tmp);

	struct dirent **namelist;
	int n;

	n = scandir(PLUGINDIR "/", &namelist, 0, alphasort);
	if (n < 0)
	{
		perror("scandir");
	}
	else
	{
		for(int count=0;count<n;count++)
		{
			SPluginInfo		info;
			string			filen = namelist[count]->d_name;
			int				pos = filen.find(".cfg");
			if(pos!=-1)
			{
				string pluginname = filen.substr(0,pos);
				printf("found game plugin: %s\n", pluginname.c_str());
				if (_loadInfo( (PLUGINDIR"/"+filen).c_str(), &info ) )
					continue;

				game* tmp = new game();
			    tmp->name = info.name;
			    tmp->desc = info.desc;
			    tmp->depend = info.depend;
				tmp->filename = pluginname;
				gamelist.insert(gamelist.end(), tmp);
			}
			free(namelist[count]);
		}
		free(namelist);
	}

	paint();
	
	bool loop=true;
	while (loop)
	{
		int key = g_RCInput->getKey(100);
		if ((key==CRCInput::RC_timeout) || (key==g_settings.key_channelList_cancel))
		{
			loop=false;
		}
		else if (key==g_settings.key_channelList_pageup)
		{
			selected+=listmaxshow;
			if (selected>gamelist.size()-1)
				selected=0;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if (key==g_settings.key_channelList_pagedown)
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=gamelist.size()-1;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paint();
		}
		else if (key==CRCInput::RC_up)
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = gamelist.size()-1;
			}
			else selected--;
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
		else if (key==CRCInput::RC_down)
		{
			int prevselected=selected;
			selected = (selected+1)%gamelist.size();
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
		else if (key==CRCInput::RC_ok)
		{
			if(selected==0)
			{
				loop=false;
			}
			else
			{//exec the plugin :))
				runGame( selected );
			}
		} else {
			g_RCInput->pushbackKey (key);
			loop=false;
		}
	}
	hide();
	return RETURN_REPAINT;
}

void CGameList::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CGameList::paintItem(int pos)
{
	int ypos = (y+theight) + pos*fheight;
	int itemheight = fheight;
	if(pos==0)
	{	//back is half-height...
		itemheight = (fheight / 2) + 3;
		g_FrameBuffer->paintBoxRel(x,ypos+itemheight, width, 15, COL_MENUCONTENT);
		g_FrameBuffer->paintBoxRel(x+10,ypos+itemheight+5, width-20, 1, COL_MENUCONTENT+5);
		g_FrameBuffer->paintBoxRel(x+10,ypos+itemheight+6, width-20, 1, COL_MENUCONTENT+2);
	}
	else
	{
		ypos -= (fheight / 2) - 15;
	}

	int color = COL_MENUCONTENT;
	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}

	g_FrameBuffer->paintBoxRel(x,ypos, width, itemheight, color);
	if(liststart+pos<gamelist.size())
	{
		game* aktgame = gamelist[liststart+pos];
		// 2001-12-05 rasc: fonts sind in neutrino.cpp definiert
		g_Fonts->gamelist_itemLarge->RenderString(x+10, ypos+fheight1+3, width-20, aktgame->name.c_str(), color);
		g_Fonts->gamelist_itemSmall->RenderString(x+20, ypos+fheight,    width-20, aktgame->desc.c_str(), color);

	}
}

void CGameList::paintHead()
{
	g_FrameBuffer->paintBoxRel(x,y, width,theight, COL_MENUHEAD);
	g_FrameBuffer->paintIcon("games.raw",x+8,y+5);
	g_Fonts->menu_title->RenderString(x+38,y+theight+1, width, g_Locale->getText(name).c_str(), COL_MENUHEAD);
}

void CGameList::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	
	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}
}

void CGameList::runGame(int selected )
{
	printf("PLUGINDEMO------------------------------------------------\n\n");
	void			*handle;
	PluginExecProc	execPlugin;
	char			*error;
	char			*p;
	char			*np;
	char			*argv[20];
	void			*libhandle[20];
	int				argc;
	int				i;
	char			depstring[129];

	string pluginname = gamelist[selected]->filename;

	strcpy(depstring, gamelist[selected]->depend.c_str());

	argc=0;
	if ( depstring[0] )
	{
		p=depstring;
		while( 1 )
		{
			argv[ argc ] = p;
			argc++;
			np = strchr(p,',');
			if ( !np )
				break;
		
			*np=0;
			p=np+1;
			if ( argc == 20 )	// mehr nicht !
				break;
		}
	}
	for( i=0; i<argc; i++ )
	{
		string libname = argv[i];
		printf("try load shared lib : %s\n",argv[i]);
		libhandle[i] = dlopen ( *argv[i] == '/' ? 
			argv[i] : ("/lib/"+libname).c_str(),
			RTLD_NOW | RTLD_GLOBAL );
		if ( !libhandle )
		{
			fputs (dlerror(), stderr);
			break;
		}
	}
	while ( i == argc )		// alles geladen
	{
		handle = dlopen ( (PLUGINDIR "/"+pluginname+".so").c_str(), RTLD_NOW);
		if (!handle)
		{
			fputs (dlerror(), stderr);
			//should unload libs!
			break;
		}
		execPlugin = (PluginExecProc) dlsym(handle, (pluginname+"_exec").c_str());
		if ((error = dlerror()) != NULL)
		{
			fputs(error, stderr);
			dlclose(handle);
			//should unload libs!
			break;
		}
		g_RCInput->stopInput();
		printf("try exec...\n");
		execPlugin(g_FrameBuffer->getFileHandle(),
						g_RCInput->getFileHandle(), -1,
						0 /*cfgfile*/);
		dlclose(handle);
		printf("exec done...\n");
		g_RCInput->restartInput();
		g_RCInput->clear();
		//restore framebuffer...
		g_FrameBuffer->paletteSet();
		g_FrameBuffer->paintBackgroundBox(0,0,720,576);		
		//redraw menue...
		paintHead();
		paint();
		break;	// break every time - never loop - run once !!!
	}

	/* unload shared libs */
	for( i=0; i<argc; i++ )
	{
		if ( libhandle[i] )
			dlclose(libhandle[i]);
		else
			break;
	}
}

