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
//#include <fontrenderer.h>
#include <string>


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
		};

		enum AUDIOMODES
		{
			AUDIO_MODE_PLAY,
			AUDIO_MODE_STOP,
			AUDIO_MODE_FF,
			AUDIO_MODE_PAUSE,
			AUDIO_MODE_REV
		};

		enum SOURCES
		{
			SOURCE_STREAM,
			SOURCE_FILE
		};

	private:

		class FontsDef
		{
			public:
				LcdFont *channelname;
				LcdFont *time; 
				LcdFont *menutitle;
				LcdFont *menu;
				int channelname_size;
				int time_size; 
				int menutitle_size;
				int menu_size;

		};

		CLCDDisplay			display;
		LcdFontRenderClass		*fontRenderer;
		FontsDef			fonts;

#define LCD_NUMBER_OF_BACKGROUNDS 5
		raw_display_t                   background[LCD_NUMBER_OF_BACKGROUNDS];

		MODES				mode;
		SOURCES				source;

		std::string			servicename;
		char				volume;
		unsigned char			percentOver;
		bool				muted;
		bool				showclock;
//		CConfigFile			configfile;
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

//State indicator

		AUDIOMODES	amode;

//   MENU   ////////////////////////////////////////////////////////

#define NUM_MENU_LINES	3

		std::string	menu_title;
		std::string	menu_lines[NUM_MENU_LINES];
		bool		menu_line_set[NUM_MENU_LINES];
		bool		menu_top;
		bool		menu_bottom;
		int		menu_selected;

////////////////////////////////////////////////////////////////////

	public:

		void		DrawString( 
							const int			_x,
							const int			_y,
							const int			_width,	
							const std::string	_text,
							LcdFont*		_font,
							int			_font_size,
							const bool			_invert,
							const unsigned long	_frame );

//   MENU   ////////////////////////////////////////////////////////
		void		setMenuTitle( const std::string _title );
		void		setMenuLine( const std::string _line );
		void		setMenuCursors( const bool _top, const bool _bottom );
		void		setMenuSelected( const int _selected );
		void		ShowMenu(unsigned long);
		void		ClearMenu();
////////////////////////////////////////////////////////////////////

		unsigned long ShowPlayingFile( const std::string _filename, const char _percentover, std::string _tplayed, unsigned long _frame );

		void setlcdparameter(void);

		static CLCD* getInstance();
		void init(const char * fontfile, const char * fontname,
		          const char * fontfile2=NULL, const char * fontname2=NULL,
		          const char * fontfile3=NULL, const char * fontname3=NULL); 

		void setMode(const MODES m, const char * const title = "");

////////////////////////////////////////////////
		void showEditBox( std::string _ip, int _line, int _selected );
		void clear( bool _commit = false );

		void showServicename(const std::string & name); // UTF-8
		void showTime();
		/** blocks for duration seconds */

		void showMessageBox( std::string _msg, int _type, const unsigned long _frame = 0 );

		void showRCLock(int duration = 2);
		void showVolume(const char vol, const bool perform_update = true);
		void showPercentOver(const unsigned char perc, const bool perform_update = true);
		void showMenuText(const int position, const char * text, const int highlight = -1, const bool utf_encoded = false);
		void showAudioTrack(const std::string & artist, const std::string & title, const std::string & album);
		void showAudioPlayMode(AUDIOMODES m=AUDIO_MODE_PLAY);
		void showAudioProgress(const char perc, bool isMuted);
////////////////////////////////////////////////


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
};


#endif
