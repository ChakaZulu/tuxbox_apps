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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/gamelist.h>

#include <sstream>
#include <fstream>
#include <iostream>

#include <dirent.h>
#include <dlfcn.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <global.h>
#include <neutrino.h>

#include <zapit/client/zapittools.h>

/* for alexW images with old drivers: 
 * #define USE_VBI_INTERFACE 1
 */

#ifdef USE_VBI_INTERFACE
 #define AVIA_VBI_START_VTXT	1
 #define AVIA_VBI_STOP_VTXT	2 
#endif

#include <daemonc/remotecontrol.h>
extern CPlugins       * g_PluginList;    /* neutrino.cpp */
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

bool CPlugins::plugin_exists(const std::string & filename)
{
	return (find_plugin(filename) >= 0);
}

int CPlugins::find_plugin(const std::string & filename)
{
	for(int i = 0; i <  (int) plugin_list.size();i++)
	{
		if( (filename.compare(plugin_list[i].filename) == 0) || (filename.compare(plugin_list[i].filename + ".cfg") == 0) )
			return i;
	}
	return -1;
}

void CPlugins::scanDir(const char *dir)
{
	struct dirent **namelist;
	std::string fname;

	int number_of_files = scandir(dir, &namelist, 0, alphasort);

	for (int i = 0; i < number_of_files; i++)
	{
		std::string filename;

		filename = namelist[i]->d_name;
		int pos = filename.find(".cfg");
		if (pos > -1)
		{
			plugin new_plugin;
			new_plugin.filename = filename.substr(0, pos);
			fname = dir;
			fname += '/';
			new_plugin.cfgfile = fname.append(new_plugin.filename);
			new_plugin.cfgfile.append(".cfg");
			new_plugin.sofile = fname;
			new_plugin.sofile.append(".so");
			parseCfg(&new_plugin);
			if(!plugin_exists(new_plugin.filename))
			{
				plugin_list.push_back(new_plugin);
				number_of_plugins++;
			}
		}
	}
}

void CPlugins::loadPlugins()
{
	frameBuffer = CFrameBuffer::getInstance();
	number_of_plugins = 0;
	plugin_list.clear();

	scanDir("/var/tuxbox/plugins");
	scanDir(PLUGINDIR);
	sort(plugin_list.begin(), plugin_list.end());
}

CPlugins::~CPlugins()
{
	plugin_list.clear();
}

void CPlugins::parseCfg(plugin *plugin_data)
{
//	FILE *fd;

	std::ifstream inFile;
	std::string line[20];
	int linecount = 0;

	inFile.open(plugin_data->cfgfile.c_str());

	while(linecount < 20 && getline(inFile, line[linecount++]));

	plugin_data->fb = false;
	plugin_data->rc = false;
	plugin_data->lcd = false;
	plugin_data->vtxtpid = false;
	plugin_data->showpig = false;
	plugin_data->needoffset = false;
	plugin_data->type = PLUGIN_TYPE_DISABLED;

	for (int i = 0; i < linecount; i++)
	{
		std::istringstream iss(line[i]);
		std::string cmd;
		std::string parm;

		getline(iss, cmd, '=');
		getline(iss, parm, '=');

		if (cmd == "pluginversion")
		{
			plugin_data->version = atoi(parm.c_str());
		}
		else if (cmd == "name")
		{
			plugin_data->name = parm;
		}
		else if (cmd == "desc")
		{
			plugin_data->description = parm;
		}
		else if (cmd == "depend")
		{
			plugin_data->depend = parm;
		}
		else if (cmd == "type")
		{
			plugin_data->type = (plugin_type_t)atoi(parm.c_str());
		}
		else if (cmd == "needfb")
		{
			plugin_data->fb = ((parm == "1")?true:false);
		}
		else if (cmd == "needrc")
		{
			plugin_data->rc = ((parm == "1")?true:false);
		}
		else if (cmd == "needlcd")
		{
			plugin_data->lcd = ((parm == "1")?true:false);
		}
		else if (cmd == "needvtxtpid")
		{
			plugin_data->vtxtpid = ((parm == "1")?true:false);
		}
		else if (cmd == "pigon")
		{
			plugin_data->showpig = ((parm == "1")?true:false);
		}
		else if (cmd == "needoffsets")
		{
			plugin_data->needoffset = ((parm == "1")?true:false);
		}
	}

	inFile.close();
}

PluginParam * CPlugins::makeParam(const char * const id, const char * const value, PluginParam * const next)
{
	PluginParam * startparam = new PluginParam;

	startparam->next = next;
	startparam->id   = strdup(id   );
	startparam->val  = strdup(value);

	return startparam;
}

PluginParam * CPlugins::makeParam(const char * const id, const int value, PluginParam * const next)
{
	char aval[10];

	sprintf(aval, "%d", value);

	return makeParam(id, aval, next);
}

void CPlugins::startPlugin(const char * const name)
{
	int pluginnr = find_plugin(name);
	if (pluginnr > -1)
		startPlugin(pluginnr);
}

void CPlugins::startPlugin(int number)
{
	PluginExec execPlugin;
	char depstring[129];
	char			*argv[20];
	void			*libhandle[20];
	int			argc, i, lcd_fd=-1;
	char			*p;
	char			*np;
	void			*handle;
	char *        error;
	int           vtpid      =  0;
	PluginParam * startparam =  0;

	if (plugin_list[number].fb)
	{
		startparam = makeParam(P_ID_FBUFFER  , frameBuffer->getFileHandle()    , startparam);
	}
	if (plugin_list[number].rc)
	{
		startparam = makeParam(P_ID_RCINPUT  , g_RCInput->getFileHandle()      , startparam);
		startparam = makeParam(P_ID_RCBLK_ANF, g_settings.repeat_genericblocker, startparam);
		startparam = makeParam(P_ID_RCBLK_REP, g_settings.repeat_blocker       , startparam);
	}
	else
	{
		g_RCInput->stopInput();
	}
	if (plugin_list[number].lcd)
	{
		CLCD::getInstance()->pause();

		lcd_fd = open("/dev/dbox/lcd0", O_RDWR);

		startparam = makeParam(P_ID_LCD      , lcd_fd                          , startparam);
	}
	if (plugin_list[number].vtxtpid)
	{
		vtpid = g_RemoteControl->current_PIDs.PIDs.vtxtpid;
#ifdef USE_VBI_INTERFACE
		int fd = open("/dev/dbox/vbi0", O_RDWR);
		if (fd > 0)
		{
			ioctl(fd, AVIA_VBI_STOP_VTXT, 0);
			close(fd);
		}
#endif
		startparam = makeParam(P_ID_VTXTPID, vtpid, startparam);
	}
	if (plugin_list[number].needoffset)
	{
		startparam = makeParam(P_ID_VFORMAT  , g_settings.video_Format         , startparam);
		startparam = makeParam(P_ID_OFF_X    , g_settings.screen_StartX        , startparam);
		startparam = makeParam(P_ID_OFF_Y    , g_settings.screen_StartY        , startparam);
		startparam = makeParam(P_ID_END_X    , g_settings.screen_EndX          , startparam);
		startparam = makeParam(P_ID_END_Y    , g_settings.screen_EndY          , startparam);
	}

	PluginParam *par = startparam;
	for( ; par; par=par->next )
	{
		printf("[gamelist.cpp] (id,val):(%s,%s)\n", par->id, par->val);
	}

	std::string pluginname = plugin_list[number].filename;

	strcpy(depstring, plugin_list[number].depend.c_str());

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
		std::string libname = argv[i];
		printf("[CPlugins] try load shared lib : %s\n",argv[i]);
		libhandle[i] = dlopen ( *argv[i] == '/' ?
			argv[i] : (PLUGINDIR "/"+libname).c_str(),
			RTLD_NOW | RTLD_GLOBAL );
		if ( !libhandle )
		{
			fputs (dlerror(), stderr);
			break;
		}
	}
	while ( i == argc )		// alles geladen
	{
		handle = dlopen ( plugin_list[number].sofile.c_str(), RTLD_NOW);
		if (!handle)
		{
			fputs (dlerror(), stderr);
			//should unload libs!
			break;
		}
		execPlugin = (PluginExec) dlsym(handle, "plugin_exec");
		if ((error = dlerror()) != NULL)
		{
			fputs(error, stderr);
			dlclose(handle);
			//should unload libs!
			break;
		}
		printf("[CPlugins] try exec...\n");
		execPlugin(startparam);
		dlclose(handle);
		printf("[CPlugins] exec done...\n");
		//restore framebuffer...


		if (!plugin_list[number].rc)
			g_RCInput->restartInput();
		g_RCInput->clearRCMsg();

		if (plugin_list[number].lcd)
		{       
			if(lcd_fd != -1)
				close(lcd_fd);
			CLCD::getInstance()->resume();
		}       

		if (plugin_list[number].fb)
		{
			frameBuffer->paletteSet();
			frameBuffer->paintBackgroundBox(0,0,720,576);
		}

#ifdef USE_VBI_INTERFACE
		if (plugin_list[number].vtxtpid)
		{
			if (vtpid != 0)
			{
				// versuche, den gtx/enx_vbi wieder zu starten
				int fd = open("/dev/dbox/vbi0", O_RDWR);
				if (fd > 0)
				{
					ioctl(fd, AVIA_VBI_START_VTXT, vtpid);
					close(fd);
				}
			}
		}
#endif
		//redraw menue...
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

	for(par = startparam ; par; )
	{
		free(par->id);
		free(par->val);
		PluginParam * tmp = par;
		par = par->next;
		delete tmp;
	}
}

// CGameList ...

CGameList::CGameList(const neutrino_locale_t Name)
{
	frameBuffer = CFrameBuffer::getInstance();
	name = Name;
	selected = 0;
	width = 500;
   if(width>(g_settings.screen_EndX-g_settings.screen_StartX))
      width=(g_settings.screen_EndX-g_settings.screen_StartX);
	height = 526;
   if((height+50)>(g_settings.screen_EndY-g_settings.screen_StartY))
      height=(g_settings.screen_EndY-g_settings.screen_StartY) - 50; // 2*25 pixel frei
	theight  = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	//
	fheight1 = g_Font[SNeutrinoSettings::FONT_TYPE_GAMELIST_ITEMLARGE]->getHeight();
	fheight2 = g_Font[SNeutrinoSettings::FONT_TYPE_GAMELIST_ITEMSMALL]->getHeight();
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


int CGameList::exec(CMenuTarget* parent, const std::string & actionKey)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	//scan4games here!
	for(unsigned int count=0;count<gamelist.size();count++)
	{
		delete gamelist[count];
	}
	gamelist.clear();

	game* tmp = new game();
	tmp->name = g_Locale->getText(LOCALE_MENU_BACK);
	gamelist.push_back(tmp);

	for(unsigned int count=0;count < (unsigned int)g_PluginList->getNumberOfPlugins();count++)
	{
		if (g_PluginList->getType(count) == PLUGIN_TYPE_GAME)
		{
			tmp = new game();
			tmp->number = count;
			tmp->name = g_PluginList->getName(count);
			tmp->desc = g_PluginList->getDescription(count);
			gamelist.push_back(tmp);
		}
	}

	paint();

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

		if ( ( msg == CRCInput::RC_timeout ) ||
			 ( msg == (neutrino_msg_t)g_settings.key_channelList_cancel ) )
		{
			loop=false;
		}
		else if ( msg == (neutrino_msg_t)g_settings.key_channelList_pageup )
		{
			if ((int(selected)-int(listmaxshow))<0)
				selected=0;
			else
				selected -= listmaxshow;
			liststart = (selected/listmaxshow)*listmaxshow;
			paintItems();
		}
		else if ( msg == (neutrino_msg_t)g_settings.key_channelList_pagedown )
		{
			selected+=listmaxshow;
			if (selected>gamelist.size()-1)
				selected=gamelist.size()-1;
			liststart = (selected/listmaxshow)*listmaxshow;
			paintItems();
		}
		else if ( msg == CRCInput::RC_up )
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = gamelist.size()-1;
			}
			else
				selected--;
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paintItems();
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if ( msg == CRCInput::RC_down )
		{
			int prevselected=selected;
			selected = (selected+1)%gamelist.size();
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paintItems();
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if ( msg == CRCInput::RC_ok )
		{
			if(selected==0)
			{
				loop=false;
			}
			else
			{//exec the plugin :))
				runGame( selected );
			}
		}
		else if( (msg== CRCInput::RC_red) ||
				 (msg==CRCInput::RC_green) ||
				 (msg==CRCInput::RC_yellow) ||
				 (msg==CRCInput::RC_blue)  ||
		         (CRCInput::isNumeric(msg)) )
		{
			g_RCInput->postMsg(msg, data);
			loop=false;
		}
		else if ( CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all )
		{
			loop = false;
			res = menu_return::RETURN_EXIT_ALL;
		}
	}
	hide();
	return res;
}

void CGameList::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width+15,height);
}

void CGameList::paintItem(int pos)
{
	int ypos = (y+theight) + pos*fheight;
	int itemheight = fheight;

	uint8_t    color   = COL_MENUCONTENT;
	fb_pixel_t bgcolor = COL_MENUCONTENT_PLUS_0;
	if (liststart+pos==selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}

	if(liststart+pos==0)
	{	//back is half-height...
		itemheight = (fheight / 2) + 3;
		frameBuffer->paintBoxRel(x     , ypos + itemheight    , width     , 15, COL_MENUCONTENT_PLUS_0);
		frameBuffer->paintBoxRel(x + 10, ypos + itemheight + 5, width - 20,  1, COL_MENUCONTENT_PLUS_5);
		frameBuffer->paintBoxRel(x + 10, ypos + itemheight + 6, width - 20,  1, COL_MENUCONTENT_PLUS_2);
	}
	else if(liststart==0)
	{
		ypos -= (fheight / 2) - 15;
		if(pos==(int)listmaxshow-1)
			frameBuffer->paintBoxRel(x,ypos+itemheight, width, (fheight / 2)-15, COL_MENUCONTENT_PLUS_0);

	}
	frameBuffer->paintBoxRel(x, ypos, width, itemheight, bgcolor);


	if(liststart+pos<gamelist.size())
	{
		game* aktgame = gamelist[liststart+pos];
		g_Font[SNeutrinoSettings::FONT_TYPE_GAMELIST_ITEMLARGE]->RenderString(x+10, ypos+fheight1+3, width-20, aktgame->name, color, 0, true); // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_GAMELIST_ITEMSMALL]->RenderString(x+20, ypos+fheight,    width-20, aktgame->desc, color, 0, true); // UTF-8
	}
}

void CGameList::paintHead()
{
	if(listmaxshow > gamelist.size()+1)
		frameBuffer->paintBoxRel(x,y, width,theight, COL_MENUHEAD_PLUS_0);
	else
		frameBuffer->paintBoxRel(x,y, width+15,theight, COL_MENUHEAD_PLUS_0);

	frameBuffer->paintIcon("games.raw",x+8,y+5);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+38,y+theight+1, width, g_Locale->getText(name), COL_MENUHEAD, 0, true); // UTF-8
}

void CGameList::paint()
{
	hide();
	width = 500;
   if(width>(g_settings.screen_EndX-g_settings.screen_StartX))
      width=(g_settings.screen_EndX-g_settings.screen_StartX);
	height = 526;
   if((height+50)>(g_settings.screen_EndY-g_settings.screen_StartY))
      height=(g_settings.screen_EndY-g_settings.screen_StartY) - 50; // 2*25 pixel frei
	listmaxshow = (height-theight-0)/fheight;
	height = theight+0+listmaxshow*fheight; // recalc height
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
	
   liststart = (selected/listmaxshow)*listmaxshow;
	
	paintHead();
	paintItems();
}

void CGameList::paintItems()
{
	if(listmaxshow <= gamelist.size()+1)
	{
		// Scrollbar
		int nrOfPages = ((gamelist.size()-1) / listmaxshow)+1; 
		int currPage  = (liststart/listmaxshow) +1;
		float blockHeight = (height-theight-4)/nrOfPages;
		frameBuffer->paintBoxRel(x+width, y+theight, 15, height-theight,  COL_MENUCONTENT_PLUS_1);
		frameBuffer->paintBoxRel(x+ width +2, y+theight+2+int((currPage-1)*blockHeight) , 11, int(blockHeight), COL_MENUCONTENT_PLUS_3);
	}
	
   for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}
}

void CGameList::runGame(int selected )
{
	#ifdef USEACTIONLOG
		g_ActionLog->println("mode: game, " + gamelist[selected]->name);
	#endif

	g_PluginList->startPlugin( gamelist[selected]->number );

    //redraw menue...
    paint();
}
