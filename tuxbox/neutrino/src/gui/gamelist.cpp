#include "gamelist.h"
#include "../include/debug.h"
#include "../global.h"

// hi McClean - hab schon mal was geaendert - falls es nicht gefaellt
// altes gamelist.cpp ist als svd.gamelist.cpp eingecheckt.


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
			string			filen = namelist[count]->d_name;
			int				pos = filen.find(".cfg");
			if(pos!=-1)
			{
				string pluginname = filen.substr(0,pos);
				printf("found game plugin: %s\n", pluginname.c_str());
				if (_loadInfo( ("/usr/lib/neutrino/games/"+filen).c_str(), &info ) )
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
		libhandle[i] = dlopen ( ("/lib/"+libname).c_str(), RTLD_NOW | RTLD_GLOBAL );
		if ( !libhandle )
		{
			fputs (dlerror(), stderr);
			break;
		}
	}
	while ( i == argc )		// alles geladen
	{
		handle = dlopen ( ("/usr/lib/neutrino/games/"+pluginname+".so").c_str(), RTLD_NOW);
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
						g_RCInput->getFileHandle(), -1);
		dlclose(handle);
		printf("exec done...\n");
		g_RCInput->restartInput();
		//restore framebuffer...
		g_FrameBuffer->paletteSet();
		memset(g_FrameBuffer->lfb, 255, g_FrameBuffer->Stride()*576);
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

