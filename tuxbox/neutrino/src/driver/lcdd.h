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

#include <configfile.h>
#include <pthread.h>

#include <lcddisplay/lcddisplay.h>
#include <lcddisplay/fontrenderer.h>


class CLCDPainter;
class fontRenderClass;
class CLCD
{
	public:

		enum MODES
		{
			MODE_TVRADIO,
			MODE_SCART,
			MODE_MENU,
			MODE_SAVER,
			MODE_SHUTDOWN,
			MODE_STANDBY,
			MODE_MENU_UTF8
		};


	private:

		class FontsDef
		{
			public:
				Font *channelname;
				Font *time; 
				Font *menutitle;
				Font *menu;
		};

		CLCDDisplay			display;
		fontRenderClass		*fontRenderer;
		FontsDef			fonts;

		raw_display_t		icon_lcd;
		raw_display_t		icon_setup;
		raw_display_t		icon_power;

		MODES				mode;

		string				servicename;
		char				volume;
		int					lcd_brightness;
		int					lcd_standbybrightness;
		int					lcd_contrast;
		int					lcd_power;
		int					lcd_inverse;
		bool				muted;
		bool				showclock;
		CConfigFile			configfile;
		pthread_t			thrTime;

		CLCD();
		~CLCD();

		static void* TimeThread(void*);
		bool lcdInit();
		void setlcdparameter(int dimm, int contrast, int power, int inverse);

	public:

		static CLCD* getInstance();
		void init();

		void saveConfig();
		void loadConfig();


		void setMode(MODES m, string title="");

		void showServicename(string name);
		void showTime();
		void showVolume(char vol);
		void showMenuText(int position, string text, int highlight=-1);

		void setBrightness(int);
		int getBrightness();

		void setBrightnessStandby(int);
		int getBrightnessStandby();

		void setContrast(int);
		int getContrast();

		void setPower(int);
		int getPower();

		void setInverse(int);
		int getInverse();

		void setMuted(bool);

		void resume();
		void pause();
};


#endif
