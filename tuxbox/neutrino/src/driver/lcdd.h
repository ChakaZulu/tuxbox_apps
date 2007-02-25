/*
	$Id: lcdd.h,v 1.33 2007/02/25 21:25:03 guenther Exp $

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

#ifndef LCD_UPDATE
#define LCD_UPDATE 1
#endif

#ifdef LCD_UPDATE
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
// TODO Why is USE_FILE_OFFSET64 not defined, if file.h is included here????
#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64 1
#endif
#include "driver/file.h"
#endif // LCD_UPDATE

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
			MODE_SHUTDOWN,
			MODE_STANDBY,
			MODE_MENU_UTF8,
			MODE_AUDIO
#ifdef LCD_UPDATE
		,	MODE_FILEBROWSER,
			MODE_PROGRESSBAR,
			MODE_PROGRESSBAR2,
			MODE_INFOBOX
#endif // LCD_UPDATE
		};
		enum AUDIOMODES
		{
			AUDIO_MODE_PLAY,
			AUDIO_MODE_STOP,
			AUDIO_MODE_FF,
			AUDIO_MODE_PAUSE,
			AUDIO_MODE_REV
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

#define LCD_NUMBER_OF_BACKGROUNDS 5
		raw_display_t                   background[LCD_NUMBER_OF_BACKGROUNDS];

		MODES				mode;

		std::string			servicename;
		char				volume;
		unsigned char			percentOver;
		bool				muted;
		bool				showclock;
		CConfigFile			configfile;
		pthread_t			thrTime;
		int                             last_toggle_state_power;
		int				clearClock;
		unsigned int                    timeout_cnt;

		void wake_up();
		void count_down();

		CLCD();

		static void* TimeThread(void*);
		bool lcdInit(const char * fontfile1, const char * fontname1, 
		             const char * fontfile2=NULL, const char * fontname2=NULL,
		             const char * fontfile3=NULL, const char * fontname3=NULL);
		void setlcdparameter(int dimm, int contrast, int power, int inverse);
		void displayUpdate();
	public:

		void setlcdparameter(void);

		static CLCD* getInstance();
		void init(const char * fontfile, const char * fontname,
		          const char * fontfile2=NULL, const char * fontname2=NULL,
		          const char * fontfile3=NULL, const char * fontname3=NULL); 

		void setMode(const MODES m, const char * const title = "");

		void showServicename(const std::string & name); // UTF-8
		void showTime();
		/** blocks for duration seconds */
		void showRCLock(int duration = 2);
		void showVolume(const char vol, const bool perform_update = true);
		void showPercentOver(const unsigned char perc, const bool perform_update = true);
		void showMenuText(const int position, const char * text, const int highlight = -1, const bool utf_encoded = false);
		void showAudioTrack(const std::string & artist, const std::string & title, const std::string & album);
		void showAudioPlayMode(AUDIOMODES m=AUDIO_MODE_PLAY);
		void showAudioProgress(const char perc, bool isMuted);
		void setBrightness(int);
		int getBrightness();

		void setBrightnessStandby(int);
		int getBrightnessStandby();

		void setContrast(int);
		int getContrast();

		void setPower(int);
		int getPower();

		void togglePower(void);

		void setInverse(int);
		int getInverse();

		void setAutoDimm(int);
		int getAutoDimm();

		void setMuted(bool);

		void resume();
		void pause();
#ifdef LCD_UPDATE
	private:
		CFileList* m_fileList;
		int m_fileListPos;
		std::string m_fileListHeader;

		std::string m_infoBoxText;
		std::string m_infoBoxTitle;
		int m_infoBoxTimer;   // for later use
		bool m_infoBoxAutoNewline;
		
		bool m_progressShowEscape;
		std::string  m_progressHeaderGlobal;
		std::string  m_progressHeaderLocal;
		int m_progressGlobal;
		int m_progressLocal;
	public:
		MODES getMode(void){return mode;};

		void showFilelist(int flist_pos = -1,CFileList* flist = NULL,const char * const mainDir=NULL);
		void showInfoBox(const char * const title = NULL,const char * const text = NULL,int autoNewline = -1,int timer = -1);
		void showProgressBar(int global = -1,const char * const text = NULL,int show_escape = -1,int timer = -1);
		void showProgressBar2(int local = -1,const char * const text_local = NULL,int global = -1,const char * const text_global = NULL,int show_escape = -1);
#endif // LCD_UPDATE
};


#endif
