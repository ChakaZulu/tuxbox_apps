/*
	LCD-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/



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

#ifndef __lcdd__
#define __lcdd__

#include <libconfigfile/configfile.h>
#include <liblcddisplay.h>
#include <lcddclient.h>

#include <pthread.h>


class CLCDD
{
	private:
		
		class FontsDef
		{
			public:
				Font *channelname;
				Font *time; 
				Font *menutitle;
				Font *menu;
		};

		CConfigFile		configfile;
		CLCDDisplay		display;
		fontRenderClass	*fontRenderer;
		FontsDef		fonts;
		pthread_t		thrTime;

		CLcddClient::mode	mode;
		raw_display_t	icon_lcd;
		raw_display_t	icon_setup;
		raw_display_t	icon_power;

		char			servicename[40];
		char			volume;
		int				lcd_brightness;
		int				lcd_standbybrightness;
		bool			muted, shall_exit, debugoutput;
		bool			showclock;

		CLCDD();
		~CLCDD();

		static void* TimeThread(void*);
		static void sig_catch(int);

		void parse_command(int connfd, CLcddClient::commandHead rmsg);
		
		void show_servicename(string name);
		void show_time();
		void show_signal();
		void show_volume(char vol);
		void show_menu(int position, char* text, int highlight);

		void dimmlcd(int val);

		void set_mode(CLcddClient::mode m, char *title);

	public:

		static CLCDD* getInstance();
		int main(int argc, char **argv);

		void saveConfig();
		void loadConfig();

};


#endif
