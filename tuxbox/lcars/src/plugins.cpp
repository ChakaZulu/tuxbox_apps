/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
$Log: plugins.cpp,v $
Revision 1.10  2004/06/16 08:46:45  thegoodguy
fix compilation (untested)

Revision 1.9  2002/06/02 12:18:47  TheDOC
source reformatted, linkage-pids correct, xmlrpc removed, all debug-messages removed - 110k smaller lcars with -Os :)

Revision 1.8  2002/05/20 20:08:12  TheDOC
some new timer and epg-stuff

Revision 1.7  2002/05/18 04:31:02  TheDOC
Warningelimination

Revision 1.6  2002/05/18 02:55:24  TheDOC
LCARS 0.21TP7

Revision 1.5  2002/03/03 22:56:27  TheDOC
lcars 0.20

<<<<<<< plugins.cpp
Revision 1.4  2001/12/20 00:31:38  tux
Plugins korrigiert

Revision 1.3  2001/12/19 04:48:37  tux
Neue Plugin-Schnittstelle

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

=======
Revision 1.4  2002/01/12 21:04:35  fx2
libfx2 is now in PLUGINDIR (moved from /lib)

>>>>>>> 1.4
Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include "plugins.h"

#include <unistd.h>
#include <fcntl.h>

void plugins::loadPlugins()
{
	//printf("Checking plugins-directory\n");
	//printf("Dir: %s\n", PLUGINDIR "/");

	struct dirent **namelist;

	int number_of_files = scandir(PLUGINDIR, &namelist, 0, alphasort);

	number_of_plugins = 0;
	plugin_list.clear();
	for (int i = 0; i < number_of_files; i++)
	{
		std::string filename;

		filename = namelist[i]->d_name;
		int pos = filename.find(".cfg");
		if (pos > -1)
		{
			number_of_plugins++;

			plugin new_plugin;
			new_plugin.filename = filename.substr(0, pos);
			std::string fname = PLUGINDIR "/";
			new_plugin.cfgfile = fname.append(new_plugin.filename);
			new_plugin.cfgfile.append(".cfg");
			fname = PLUGINDIR "/";
			new_plugin.sofile = fname.append(new_plugin.filename);
			new_plugin.sofile.append(".so");

			parseCfg(&new_plugin);

			plugin_list.insert(plugin_list.end(), new_plugin);
		}
	}
	//printf("%d plugins found...\n", number_of_plugins);

}


void plugins::addParm(std::string cmd, std::string value)
{
	params[cmd] = value;
}

void plugins::addParm(std::string cmd, int value)
{
	std::stringstream ostr;
	ostr << value << std::ends;
	addParm(cmd, ostr.str());
}

void plugins::setfb(int fd)
{
	addParm(P_ID_FBUFFER, fd);
}

void plugins::setrc(int fd)
{
	addParm(P_ID_RCINPUT, fd);
}

void plugins::setlcd(int fd)
{
	addParm(P_ID_LCD, fd);
}

void plugins::setvtxtpid(int fd)
{
	addParm(P_ID_VTXTPID, fd);
}

void plugins::parseCfg(plugin *plugin_data)
{
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
			plugin_data->type = atoi(parm.c_str());
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
			plugin_data->offsets = ((parm == "1")?true:false);
		}
		else if (cmd == "needvidformat")
		{
			plugin_data->vformat = ((parm == "1")?true:false);
		}
	}

	inFile.close();

	/*//printf("Name: %s\n", plugin_data->cfgfile.c_str());
	fd = fopen(plugin_data->cfgfile.c_str(), "rt");

	char text[512];
	while(fgets(text, 512, fd))
	{
		char tmp_text[512];
		sscanf(text, "pluginversion=%d\n", &(plugin_data->version));
		
		if (sscanf(text, "name=%s\n", tmp_text))
			plugin_data->name = std::string(tmp_text);

		if (sscanf(text, "desc=%s\n", tmp_text))
			plugin_data->description = std::string(tmp_text);

		if (sscanf(text, "depend=%s\n", tmp_text))
			plugin_data->depend = std::string(tmp_text);

		sscanf(text, "type=%d\n", &(plugin_data->type));

		sscanf(text, "needfb=%d\n", &(plugin_data->fb));

		sscanf(text, "needrc=%d\n", &(plugin_data->rc));

		sscanf(text, "needlcd=%d\n", &(plugin_data->lcd));
		sscanf(text, "needvtxtpid=%d\n", &(plugin_data->vtxtpid));
		
		sscanf(text, "pigon=%d\n", &(plugin_data->showpig));
		sscanf(text, "pigsize=%dx%d\n", &(plugin_data->sizex), &(plugin_data->sizey));
		sscanf(text, "pigpos=%d,%d\n", &(plugin_data->posx), &(plugin_data->posy));
		
	}

	local_plugin_data = plugin_data;
	fclose(fd);*/
}

PluginParam* plugins::makeParam(const char * const id, PluginParam *next)
{
	//std::cout << "Adding " << id << " With Value " << params.find(id)->second.c_str() << " and next: " << (int) next << std::endl;

	PluginParam *startparam = new PluginParam;
	startparam->next = next;
	startparam->id = id;
	startparam->val = new char[params.find(id)->second.length() + 2];
	strcpy(startparam->val, params.find(id)->second.c_str());

	//std::cout << "Startparam: " << (int) startparam << std::endl;
	return startparam;
}

void plugins::startPlugin(int number)
{
	//std::cout << "Plugin-Number: " << number << std::endl;
	PluginExec execPlugin;
	char depstring[129];
	char			*argv[20];
	void			*libhandle[20];
	int				argc;
	int				i;
	char			*p;
	char			*np;
	void			*handle;
	char			*error;

	PluginParam *startparam;
	PluginParam *tmpparam;

	startparam = 0;
	tmpparam = startparam;

	addParm(P_ID_VFORMAT, 2);
	addParm(P_ID_OFF_X, 0);
	addParm(P_ID_OFF_Y, 0);
	addParm(P_ID_END_X, 720);
	addParm(P_ID_END_Y, 576);

	if (plugin_list[number].fb)
	{
		//std::cout << "With FB " << params.find(P_ID_FBUFFER)->second.c_str() <<std::endl;
		startparam = makeParam(P_ID_FBUFFER, startparam);
		//std::cout << "New Startparam: " << startparam << std::endl;
		//std::cout << "New Tmpparam: " << tmpparam << std::endl;


	}
	if (plugin_list[number].rc)
	{
		//std::cout << "With RC " << params.find(P_ID_RCINPUT)->second.c_str() << std::endl;

		startparam = makeParam(P_ID_RCINPUT, startparam);
	}
	if (plugin_list[number].lcd)
	{
		//std::cout << "With LCD " << std::endl;

		startparam = makeParam(P_ID_LCD, startparam);
	}
	if (plugin_list[number].vtxtpid)
	{
		//std::cout << "With VTXTPID " << params.find(P_ID_VTXTPID)->second.c_str() << std::endl;

		startparam = makeParam(P_ID_VTXTPID, startparam);
	}
	if (plugin_list[number].vformat)
	{
		startparam = makeParam(P_ID_VFORMAT, startparam);
	}
	if (plugin_list[number].offsets)
	{
		startparam = makeParam(P_ID_OFF_X, startparam);
		startparam = makeParam(P_ID_OFF_Y, startparam);
		startparam = makeParam(P_ID_END_X, startparam);
		startparam = makeParam(P_ID_END_Y, startparam);
	}

	//PluginParam *par = startparam;
	/*for( ; par; par=par->next )
	{
		//printf ("id: %s - val: %s\n", par->id, par->val);
		//printf("%d\n", par->next);
	}*/

	//std::cout << "Mark-2" << std::endl;

	std::string pluginname = plugin_list[number].filename;

	strcpy(depstring, plugin_list[number].depend.c_str());

	//std::cout << "Mark-1" << std::endl;

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
	//std::cout << "Mark0" << std::endl;
	for( i=0; i<argc; i++ )
	{
		std::string libname = argv[i];
		//printf("try load shared lib : %s\n",argv[i]);
		libhandle[i] = dlopen ( *argv[i] == '/' ?
		                        argv[i] : (PLUGINDIR "/"+libname).c_str(),
		                        RTLD_NOW | RTLD_GLOBAL );
		if ( !libhandle )
		{
			fputs (dlerror(), stderr);
			break;
		}
	}
	//std::cout << "Mark1" << std::endl;
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
		//printf("try exec...\n");
		execPlugin(startparam);
		dlclose(handle);
		//printf("exec done...\n");
		//restore framebuffer...
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


}
