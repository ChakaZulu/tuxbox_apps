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
$Log: plugins.h,v $
Revision 1.4  2001/12/16 18:45:35  waldi
- move all configfiles to CONFIGDIR
- make CONFIGDIR in install-data-local

Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef PLUGINS_H
#define PLUGINS_H

#include <string>
#include <dirent.h>
#include <vector>
#include <dlfcn.h> 

#include "gameplugins.h"

#include <config.h>

class plugins
{
	struct plugin
	{
		std::string filename;
		std::string cfgfile;
		std::string sofile;
		int version;
		std::string name;
		std::string description;
		std::string depend;
		int type;

		int fb;
		int rc;
		int lcd;
		int posx, posy, sizex, sizey;
		int showpig;
	};
	
	int fb, rc, lcd;
	int number_of_plugins;
	std::string plugin_dir;
	std::vector<struct plugin> plugin_list;

	void parseCfg(plugin *plugin_data);
public:	
	void loadPlugins();

	void setPluginDir(std::string dir) { plugin_dir = dir; }

	void setfb(int fd) { fb = fd; }
	void setrc(int fd) { rc = fd;}
	void setlcd(int fd) { lcd = fd; }

	int getNumberOfPlugins() { return plugin_list.size(); }
	std::string getName(int number) { return plugin_list[number].name; }
	std::string getDescription(int number) { return plugin_list[number].description; }
	int getShowPig(int number) { return plugin_list[number].showpig; }
	int getPosX(int number) { return plugin_list[number].posx; }
	int getPosY(int number) { return plugin_list[number].posy; }
	int getSizeX(int number) { return plugin_list[number].sizex; }
	int getSizeY(int number) { return plugin_list[number].sizey; }
	
	void startPlugin(int number);
};

#endif
