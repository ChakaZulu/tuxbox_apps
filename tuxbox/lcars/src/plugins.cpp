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

#include "plugins.h"

#include <unistd.h>
#include <fcntl.h>

void plugins::loadPlugins()
{
	printf("Checking plugins-directory\n");
	printf("Dir: %s\n", plugin_dir.c_str());

	struct dirent **namelist;
	
	int number_of_files = scandir(plugin_dir.c_str(), &namelist, 0, alphasort);
	
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
			std::string fname = plugin_dir;
			new_plugin.cfgfile = fname.append(new_plugin.filename);
			new_plugin.cfgfile.append(".cfg");
			fname = plugin_dir;
			new_plugin.sofile = fname.append(new_plugin.filename);
			new_plugin.sofile.append(".so");

			parseCfg(&new_plugin);
			
			plugin_list.insert(plugin_list.end(), new_plugin);
		}
	}
	printf("%d plugins found...\n", number_of_plugins);

}

void plugins::parseCfg(plugin *plugin_data)
{
	FILE *fd;
	
	printf("Name: %s\n", plugin_data->cfgfile.c_str());
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
		
		sscanf(text, "pigon=%d\n", &(plugin_data->showpig));
		sscanf(text, "pigsize=%dx%d\n", &(plugin_data->sizex), &(plugin_data->sizey));
		sscanf(text, "pigpos=%d,%d\n", &(plugin_data->posx), &(plugin_data->posy));
		
	}

	fclose(fd);
}

void plugins::startPlugin(int number)
{
	PluginExecProc execPlugin;
	char depstring[129];
	char			*argv[20];
	void			*libhandle[20];
	int				argc;
	int				i;
	char			*p;
	char			*np;
	void			*handle;
	char			*error;


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
		handle = dlopen ( plugin_list[number].sofile.c_str(), RTLD_NOW);
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
		printf("try exec...\n");
		execPlugin(fb, rc, lcd);
		dlclose(handle);
		printf("exec done...\n");
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
