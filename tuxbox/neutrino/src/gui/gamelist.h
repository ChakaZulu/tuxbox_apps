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

#ifndef __gamelist__
#define __gamelist__

#include <gui/widget/menue.h>

#include <driver/framebuffer.h>
#include <system/localize.h>

#include <plugin.h>

#include <string>
#include <vector>
#include <map>


class CPlugins
{
	private:

		CFrameBuffer	*frameBuffer;

		struct plugin
		{
			std::string filename;
			std::string cfgfile;
			std::string sofile;
			int version;
			std::string name;                // UTF-8 encoded
			std::string description;         // UTF-8 encoded
			std::string depend;
			plugin_type_t type;

			bool fb;
			bool rc;
			bool lcd;
			bool vtxtpid;
			int posx, posy, sizex, sizey;
			bool showpig;
			bool needoffset;
			bool operator< (const plugin& a) const
			{
				return this->filename < a.filename ;
			}
		};

		int fb, rc, lcd, pid;
		int number_of_plugins;
		std::string plugin_dir;
		std::vector<plugin> plugin_list;

		void parseCfg(plugin *plugin_data);
		void scanDir(const char *dir);
		bool plugin_exists(const std::string & filename);
		int find_plugin(const std::string & filename);


		std::map<std::string, std::string> params;
	public:

		~CPlugins();

		void loadPlugins();

		void setPluginDir(const std::string & dir) { plugin_dir = dir; }

		PluginParam* makeParam(const std::string & id, PluginParam *next);

		void addParm(const std::string & cmd, int value);
		void addParm(const std::string & cmd, const std::string & value);

		void setfb(int fd);
		void setrc(int fd);
		void setlcd(int fd);
		void setvtxtpid(int fd);


		int getNumberOfPlugins() { return plugin_list.size(); }
		const char * getName(int number) { return plugin_list[number].name.c_str(); }
		std::string getDescription(int number) { return plugin_list[number].description; }
		int getVTXT(int number) { return plugin_list[number].vtxtpid; }
		int getShowPig(int number) { return plugin_list[number].showpig; }
		int getPosX(int number) { return plugin_list[number].posx; }
		int getPosY(int number) { return plugin_list[number].posy; }
		int getSizeX(int number) { return plugin_list[number].sizex; }
		int getSizeY(int number) { return plugin_list[number].sizey; }
		int getType(int number) { return plugin_list[number].type; }

		void startPlugin(int number);

		void startPlugin(const char * const filename); // start plugins also by name
};


class CGameList : public CMenuTarget
{

	private:

		CFrameBuffer	*frameBuffer;

		struct game
		{
			int         number;
			std::string name;   // UTF-8 encoded
			std::string desc;   // UTF-8 encoded
		};

		unsigned int	    liststart;
		unsigned int	    listmaxshow;
		unsigned int	    selected;
		int		    key;
		neutrino_locale_t   name;
		std::vector<game *> gamelist;


		int		fheight; // Fonthoehe Channellist-Inhalt
		int		theight; // Fonthoehe Channellist-Titel

		int		fheight1,fheight2;

		int 		width;
		int 		height;
		int 		x;
		int 		y;

		void paintItem(int pos);
		void paintItems();
		void paint();
		void paintHead();

	public:

		CGameList(const neutrino_locale_t Name);
		~CGameList();

		void hide();
		int exec(CMenuTarget* parent, const std::string & actionKey);
		void runGame(int selected );
};


#endif
