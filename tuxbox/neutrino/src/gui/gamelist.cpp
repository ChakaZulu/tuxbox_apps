#include "gamelist.h"
#include "../include/debug.h"
#include "../global.h"


CGameList::CGameList(string Name)
{
	name = Name;
	selected = 0;
	width = 500;
	height = 440;
	theight= g_Fonts->menu_title->getHeight();
	fheight= g_Fonts->channellist->getHeight();
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

	n = scandir("/usr/lib/neutrino/games", &namelist, 0, alphasort);
	if (n < 0)
	{
		perror("scandir");
	}
	else
	{
		for(int count=0;count<n;count++)
		{
			SPluginInfo		info;
			void			*handle;
			PluginInfoProc	getInfo;
			char			*error;
			string			filen = namelist[count]->d_name;
			int				pos = filen.find(".so");
			if(pos!=-1)
			{
				string pluginname = filen.substr(0,pos);
				printf("found game plugin: %s\n", pluginname.c_str());
				handle = dlopen ( ("/usr/lib/neutrino/games/"+pluginname+".so").c_str(), RTLD_LAZY);
				if (!handle)
				{
					fputs (dlerror(), stderr);
					break;
				}
				
				getInfo = (PluginInfoProc)dlsym(handle,(pluginname+"_getInfo").c_str());
				if ((error = dlerror()) != NULL)
				{
					fputs(error, stderr);
					break;
				}
				getInfo(&info);
				game* tmp = new game();
			    tmp->name = info.name;
			    tmp->desc = info.desc;
				tmp->filename = pluginname;
				gamelist.insert(gamelist.end(), tmp);
				dlclose(handle);
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
				printf("PLUGINDEMO------------------------------------------------\n\n");
				void			*handle;
				PluginInfoProc	getInfo;
				PluginExecProc	execPlugin;
				char			*error;
				SPluginInfo		info;

				string pluginname = gamelist[selected]->filename;

				handle = dlopen ( ("/usr/lib/neutrino/games/"+pluginname+".so").c_str(), RTLD_LAZY);
				if (!handle)
				{
					fputs (dlerror(), stderr);
					break;
				}
				
				getInfo = (PluginInfoProc) dlsym(handle,(pluginname+"_getInfo").c_str());
				if ((error = dlerror()) != NULL)
				{
					fputs(error, stderr);
					break;
				}
				execPlugin = (PluginExecProc) dlsym(handle, (pluginname+"_exec").c_str());
				if ((error = dlerror()) != NULL)
				{
					fputs(error, stderr);
					break;
				}


				getInfo(&info);
				printf("Plugin Name: %s\n", info.name);
				printf("Plugin Desc: %s\n", info.desc);
				printf("try exec...\n");

				g_RCInput->stopInput();
				execPlugin(g_FrameBuffer->getFileHandle(), g_RCInput->getFileHandle(), -1);
				printf("exec done...\n");
				dlclose(handle);
				g_RCInput->restartInput();
				//restore framebuffer...
				g_FrameBuffer->paletteSet();
				memset(g_FrameBuffer->lfb, 255, g_FrameBuffer->Stride()*576);
				//redraw menue...
				paintHead();
				paint();
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
	int ypos = y+ theight+0 + pos*fheight;
	int color = COL_MENUCONTENT;
	if (liststart+pos==selected)
	{
		color = COL_MENUCONTENTSELECTED;
	}

	g_FrameBuffer->paintBoxRel(x,ypos, width, fheight, color);
	if(liststart+pos<gamelist.size())
	{
		game* aktgame = gamelist[liststart+pos];
		g_Fonts->channellist->RenderString(x+10, ypos+fheight, width-20, aktgame->name.c_str(), color);
	}
}

void CGameList::paintHead()
{
	g_FrameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+theight+0, width, g_Locale->getText(name).c_str(), COL_MENUHEAD);
}

void CGameList::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	
	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}
}

