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

#include <lcddisplay/fontrenderer.h>


class CLCDPainter;
class LcdFontRenderClass;
class CLCD
{
	public:

		enum MODES
		{
			MODE_TVRADIO,
			MODE_SCART,
			UNUSED_LEGACY_MODE, /* MODE_MENU */
			MODE_SAVER,
			MODE_SHUTDOWN,
			MODE_STANDBY,
			MODE_MENU_UTF8,
			MODE_MP3
		};
		enum MP3MODES
		{
			MP3_PLAY,
			MP3_STOP,
			MP3_FF,
			MP3_PAUSE,
			MP3_REV
		};


	private:

		class FontsDef
		{
			public:
				LcdFont *channelname;
				LcdFont *time; 
				LcdFont *menutitle;
				LcdFont *menu;
		};

		CLCDDisplay			display;
		LcdFontRenderClass		*fontRenderer;
		FontsDef			fonts;

		raw_display_t		icon_lcd;
		raw_display_t		icon_lcd2;
		raw_display_t		icon_lcd3;
		raw_display_t		icon_setup;
		raw_display_t		icon_power;

		MODES				mode;

		std::string			servicename;
		char				volume;
		unsigned char			percentOver;
		bool				muted;
		bool				showclock;
		CConfigFile			configfile;
		pthread_t			thrTime;

		CLCD();
		~CLCD();

		static void* TimeThread(void*);
		bool lcdInit(const char * fontfile, const char * fontname, const bool _setlcdparameter);
		void setlcdparameter(int dimm, int contrast, int power, int inverse);

	public:

		static CLCD* getInstance();
		void init(const char * fontfile, const char * fontname, const bool _setlcdparameter);
		void setlcdparameter(void);

		void setMode(const MODES m, const char * const title = "");

		void showServicename(const std::string & name); // UTF-8
		void showTime();
		void showVolume(const char vol, const bool perform_update = true);
		void showPercentOver(const unsigned char perc, const bool perform_update = true);
		void showMenuText(const int position, const std::string & text, const int highlight = -1, const bool utf_encoded = false);
		void showMP3(const std::string & artist, const std::string & title, const std::string & album);
		void showMP3Play(MP3MODES m=MP3_PLAY);
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

		void setAutoDimm(int);
		int getAutoDimm();

		void setMuted(bool);

		void resume();
		void pause();
};


#endif
