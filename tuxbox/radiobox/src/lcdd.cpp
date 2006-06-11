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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <lcdd.h>

//#include <global.h>
//#include <neutrino.h>
//#include <system/settings.h>

#include <newclock.h>
#include <lcddisplay/lcddisplay.h>

#include <dbox/fp.h>
#include <fcntl.h>
#include <sys/timeb.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <iostream>

//#include <daemonc/remotecontrol.h>
//extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */


#define DATADIR "/share/tuxbox"

int LCD_BRIGHTNESS = 100;
int LCD_STANDBY_BRIGHTNESS = 10;
int LCD_AUTODIMM = 0;
int g_lcd_setting_dim_time = 0;
int LCD_POWER = 1;
int LCD_CONTRAST = 100;
int LCD_INVERSE = 0;
int LCD_SHOW_VOLUME = 0;

CLCD::CLCD()
//	: configfile('\t')
{
}

CLCD* CLCD::getInstance()
{
	static CLCD* lcdd = NULL;
	if(lcdd == NULL)
	{
		lcdd = new CLCD();
	}
	return lcdd;
}

void CLCD::count_down() {
	if (timeout_cnt > 0) {
		timeout_cnt--;
		if (timeout_cnt == 0) {
			//TODO: setup
			if ( LCD_BRIGHTNESS > 0) 
			{
				// save lcd brightness, setBrightness() changes global setting
				setBrightness(LCD_BRIGHTNESS);
			} else
			{
				setPower(0);
			}
		}
	} 
}

void CLCD::wake_up() {
	//todo: setup
	if( g_lcd_setting_dim_time  > 0 ) {
		timeout_cnt = g_lcd_setting_dim_time;
		setBrightness(LCD_BRIGHTNESS);
	}
	else
		setPower(1);
}

void* CLCD::TimeThread(void *)
{
	while(1)
	{
		sleep(1);
		CLCD::getInstance()->showTime();
		CLCD::getInstance()->count_down();
	}
	return NULL;
}

void CLCD::init(const char * fontfile, const char * fontname,
                const char * fontfile2, const char * fontname2,
                const char * fontfile3, const char * fontname3)
{
//	InitNewClock();

	if (!lcdInit(fontfile, fontname, fontfile2, fontname2, fontfile3, fontname3 ))
	{
		printf("LCD-Init failed!\n");
		return;
	}

	if (pthread_create (&thrTime, NULL, TimeThread, NULL) != 0 )
	{
		perror("[lcdd]: pthread_create(TimeThread)");
		return ;
	}
}

enum backgrounds {
	BACKGROUND_SETUP = 0,
	BACKGROUND_POWER = 1,
	BACKGROUND_LCD2  = 2,
	BACKGROUND_LCD3  = 3,
	BACKGROUND_LCD   = 4
};
const char * const background_name[LCD_NUMBER_OF_BACKGROUNDS] = {
	"setup",
	"power",
	"lcd2",
	"lcd3",
	"lcd"
};
#define NUMBER_OF_PATHS 2
const char * const background_path[NUMBER_OF_PATHS] = {
	"/var/tuxbox/neutrino/lcd/",
	DATADIR "/lcdd/icons/"
};

bool CLCD::lcdInit(const char * fontfile, const char * fontname,
		   const char * fontfile2, const char * fontname2,
		   const char * fontfile3, const char * fontname3)
{
	fontRenderer = new LcdFontRenderClass(&display);
	const char * style_name = fontRenderer->AddFont(fontfile);
	const char * style_name2;
	const char * style_name3;

	if (fontfile2 != NULL)
		style_name2 = fontRenderer->AddFont(fontfile2);
	else
	{
		style_name2 = style_name;
		fontname2   = fontname;
	}

	if (fontfile3 != NULL)
		style_name3 = fontRenderer->AddFont(fontfile3);
	else
	{
		style_name3 = style_name;
		fontname3   = fontname;
	}
	fontRenderer->InitFontCache();

	fonts.menu_size        = 12;
	fonts.time_size        = 14;
	fonts.channelname_size = 15;
	fonts.menutitle_size   = fonts.channelname_size;

	fonts.menu        = fontRenderer->getFont(fontname,  style_name , fonts.menu_size);
	fonts.time        = fontRenderer->getFont(fontname2, style_name2, fonts.time_size);
	fonts.channelname = fontRenderer->getFont(fontname3, style_name3, fonts.channelname_size);
	fonts.menutitle   = fonts.channelname;

	setAutoDimm(LCD_AUTODIMM);

	if (!display.isAvailable())
	{
		printf("exit...(no lcd-support)\n");
		return false;
	}
/*
	for (int i = 0; i < LCD_NUMBER_OF_BACKGROUNDS; i++)
	{
		for (int j = 0; j < NUMBER_OF_PATHS; j++)
		{
			std::string file = background_path[j];
			file += background_name[i];
			file += ".png";
			if (display.load_png(file.c_str()))
				goto found;
		}
		printf("[neutrino/lcd] no valid %s background.\n", background_name[i]);
		return false;
	found:
		display.dump_screen(&(background[i]));
	}
*/
	setMode(MODE_TVRADIO);

	return true;
}

void CLCD::displayUpdate()
{
	struct stat buf;
	if (stat("/tmp/lcd.locked", &buf) == -1)
		display.update();
}

void CLCD::setlcdparameter(int dimm, const int contrast, const int power, const int inverse)
{
	int fd;
	if (power == 0)
		dimm = 0;

	if ((fd = open("/dev/dbox/fp0", O_RDWR)) == -1)
	{
		perror("[rb lcdd] open '/dev/dbox/fp0' failed");
	}
	else
	{
		if (ioctl(fd, FP_IOCTL_LCD_DIMM, &dimm) < 0)
		{
			perror("[rbx lcdd] set dimm failed!");
		}

		close(fd);
	}
	
	if ((fd = open("/dev/dbox/lcd0", O_RDWR)) == -1)
	{
		perror("[rbx lcdd] open '/dev/dbox/lcd0' failed");
	}
	else
	{
		if (ioctl(fd, LCD_IOCTL_SRV, &contrast) < 0)
		{
			perror("[rbx lcdd] set contrast failed!");
		}

		if (ioctl(fd, LCD_IOCTL_ON, &power) < 0)
		{
			perror("[rbx lcdd] set power failed!");
		}

		if (ioctl(fd, LCD_IOCTL_REVERSE, &inverse) < 0)
		{
			perror("[rbx lcdd] set invert failed!");
		}

		close(fd);
	}
}

void CLCD::setlcdparameter(void)
{
	//TODO: setup
	last_toggle_state_power = LCD_POWER;
	setlcdparameter((mode == MODE_STANDBY) ? LCD_STANDBY_BRIGHTNESS : LCD_BRIGHTNESS,
			LCD_CONTRAST,
			last_toggle_state_power,
			LCD_INVERSE);
}

void CLCD::showServicename(const std::string & name) // UTF-8
{
	servicename = name;
	if (mode != MODE_TVRADIO)
		return;
	display.draw_fill_rect (0,14,120,48, CLCDDisplay::PIXEL_OFF);

	if (fonts.channelname->getRenderWidth(name.c_str(), true) > 120)
	{
		int pos;
		std::string text1 = name;
		do
		{
			pos = text1.find_last_of("[ .]+"); // <- characters are UTF-encoded!
			if (pos != -1)
				text1 = text1.substr( 0, pos );
		} while ( ( pos != -1 ) && ( fonts.channelname->getRenderWidth(text1.c_str(), true) > 120 ) ); // UTF-8
		
		if ( fonts.channelname->getRenderWidth(text1.c_str(), true) <= 120 ) // UTF-8
			fonts.channelname->RenderString(1,29+16, 130, name.substr(text1.length()+ 1).c_str(), CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
		else
		{
			std::string text1 = name;
			while (fonts.channelname->getRenderWidth(text1.c_str(), true) > 120) // UTF-8
				text1= text1.substr(0, text1.length()- 1);
			
			fonts.channelname->RenderString(1,29+16, 130, name.substr(text1.length()).c_str(), CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
		}
		
		fonts.channelname->RenderString(1,29, 130, text1.c_str(), CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
	}
	else
	{
		fonts.channelname->RenderString(1,37, 130, name.c_str(), CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
	}
	
	wake_up();
	displayUpdate();
}

void CLCD::showTime()
{
	if (showclock)
	{
		char timestr[21];
		struct timeb tm;
		struct tm * t;

		ftime(&tm);
		t = localtime(&tm.time);

		if (mode == MODE_STANDBY)
		{
			display.draw_fill_rect(-1, -1, 120, 64, CLCDDisplay::PIXEL_OFF); // clear lcd

//			ShowNewClock(&display, t->tm_hour, t->tm_min, t->tm_wday, t->tm_mday, t->tm_mon);
		}
		else
		{
/*
			if (CNeutrinoApp::getInstance ()->recordingstatus && clearClock == 1)
			{
				strcpy(timestr,"  :  ");
				clearClock = 0;
			}
			else
*/
			{
				strftime((char*) &timestr, 20, "%H:%M", t);
				clearClock = 1;
			}

			display.draw_fill_rect (77, 50, 120, 64, CLCDDisplay::PIXEL_OFF);

			fonts.time->RenderString(122 - fonts.time->getRenderWidth(timestr), 62, 50, timestr, CLCDDisplay::PIXEL_ON);
		}
		displayUpdate();
	}
}

void CLCD::showRCLock(int duration)
{
	std::string icon = DATADIR "/lcdd/icons/rclock.raw";
	raw_display_t curr_screen;

	// Saving the whole screen is not really nice since the clock is updated
	// every second. Restoring the screen can cause a short travel to the past ;)
	display.dump_screen(&curr_screen);
	display.draw_fill_rect (-1,10,121,50, CLCDDisplay::PIXEL_OFF);
	display.paintIcon(icon,44,25,false);
	wake_up();
	displayUpdate();
	sleep(duration);
	display.load_screen(&curr_screen);
	wake_up();
	displayUpdate();
}

void CLCD::showVolume(const char vol, const bool perform_update)
{
	volume = vol;
	if ( //TODO: setup
	    ((mode == MODE_TVRADIO) && (LCD_SHOW_VOLUME)) ||
	    (mode == MODE_SCART) ||
	    (mode == MODE_AUDIO)
	    )
	{
		display.draw_fill_rect (11,53,73,61, CLCDDisplay::PIXEL_OFF);
		//strichlin
		if (muted)
		{
			display.draw_line (12,55,72,59, CLCDDisplay::PIXEL_ON);
		}
		else
		{
			int dp = int( vol/100.0*61.0+12.0);
			display.draw_fill_rect (11,54,dp,60, CLCDDisplay::PIXEL_ON);
		}
		if(mode == MODE_AUDIO)
		{
			display.draw_fill_rect (-1, 51, 10, 62, CLCDDisplay::PIXEL_OFF);
			display.draw_rectangle ( 1, 55,  3, 58, CLCDDisplay::PIXEL_ON, CLCDDisplay::PIXEL_OFF);
			display.draw_line      ( 3, 55,  6, 52, CLCDDisplay::PIXEL_ON);
			display.draw_line      ( 3, 58,  6, 61, CLCDDisplay::PIXEL_ON);
			display.draw_line      ( 6, 54,  6, 59, CLCDDisplay::PIXEL_ON);
		}

		if (perform_update)
		  displayUpdate();
	}
	wake_up();
}

void CLCD::showPercentOver(const unsigned char perc, const bool perform_update)
{

	percentOver = perc;
	if (mode == MODE_TVRADIO)
	{ 	//TODO: setup
		if (LCD_SHOW_VOLUME == 0)
		{
			display.draw_fill_rect (11,53,73,61, CLCDDisplay::PIXEL_OFF);
			//strichlin
			if (perc==255)
			{
				display.draw_line (12,55,72,59, CLCDDisplay::PIXEL_ON);
			}
			else
			{
				int dp = int( perc/100.0*61.0+12.0);
				display.draw_fill_rect (11,54,dp,60, CLCDDisplay::PIXEL_ON);
			}
			if (perform_update)
				displayUpdate();
		} //TODO: setup
		else if (LCD_SHOW_VOLUME == 2)
		{
			display.draw_fill_rect (11,2,117,8, CLCDDisplay::PIXEL_OFF);
			//strichlin
			if (perc==255)
			{
				display.draw_line (12,3,116,7, CLCDDisplay::PIXEL_ON);
			}
			else
			{
				int dp = int( perc/100.0*105.0+12.0);
				display.draw_fill_rect (11,2,dp,8, CLCDDisplay::PIXEL_ON);
			}
			if (perform_update)
				displayUpdate();
		}
	}
}

void CLCD::showMenuText(const int position, const char * text, const int highlight, const bool utf_encoded)
{
	if (mode != MODE_MENU_UTF8)
		return;

	// reload specified line
	display.draw_fill_rect(-1,35+14*position,120,35+14+14*position, CLCDDisplay::PIXEL_OFF);
	fonts.menu->RenderString(0,35+11+14*position, 140, text, CLCDDisplay::PIXEL_INV, highlight, utf_encoded);
	wake_up();
	displayUpdate();

}

void CLCD::showAudioTrack(const std::string & artist, const std::string & title, const std::string & album)
{
	if (mode != MODE_AUDIO) 
	{
		return;
	}

	// reload specified line
	display.draw_fill_rect (-1,10,121,24, CLCDDisplay::PIXEL_OFF);
	display.draw_fill_rect (-1,20,121,37, CLCDDisplay::PIXEL_OFF);
	display.draw_fill_rect (-1,33,121,50, CLCDDisplay::PIXEL_OFF);
	fonts.menu->RenderString(0,22, 125, artist.c_str() , CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
	fonts.menu->RenderString(0,35, 125, album.c_str() , CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
	fonts.menu->RenderString(0,48, 125, title.c_str() , CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
	wake_up();
	displayUpdate();

}

void CLCD::showAudioPlayMode(AUDIOMODES m)
{
	display.draw_fill_rect (-1,51,10,62, CLCDDisplay::PIXEL_OFF);
	switch(m)
	{
		case AUDIO_MODE_PLAY:
			{
				int x=3,y=53;
				display.draw_line(x  ,y  ,x  ,y+8, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+1,y+1,x+1,y+7, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+2,y+2,x+2,y+6, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+3,y+3,x+3,y+5, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+4,y+4,x+4,y+4, CLCDDisplay::PIXEL_ON);
				break;
			}
		case AUDIO_MODE_STOP:
			display.draw_fill_rect (1, 53, 8 ,61, CLCDDisplay::PIXEL_ON);
			break;
		case AUDIO_MODE_PAUSE:
			display.draw_line(1,54,1,60, CLCDDisplay::PIXEL_ON);
			display.draw_line(2,54,2,60, CLCDDisplay::PIXEL_ON);
			display.draw_line(6,54,6,60, CLCDDisplay::PIXEL_ON);
			display.draw_line(7,54,7,60, CLCDDisplay::PIXEL_ON);
			break;
		case AUDIO_MODE_FF:
			{
				int x=2,y=55;
				display.draw_line(x   ,y   , x  , y+4, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+1 ,y+1 , x+1, y+3, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+2 ,y+2 , x+2, y+2, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+3 ,y   , x+3, y+4, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+4 ,y+1 , x+4, y+3, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+5 ,y+2 , x+5, y+2, CLCDDisplay::PIXEL_ON);
			}
			break;
		case AUDIO_MODE_REV:
			{
				int x=2,y=55;
				display.draw_line(x   ,y+2 , x  , y+2, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+1 ,y+1 , x+1, y+3, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+2 ,y   , x+2, y+4, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+3 ,y+2 , x+3, y+2, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+4 ,y+1 , x+4, y+3, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+5 ,y   , x+5, y+4, CLCDDisplay::PIXEL_ON);
			}
			break;
	}
	wake_up();
	displayUpdate();
}

void CLCD::showAudioProgress(const char perc, bool isMuted)
{
	if (mode == MODE_AUDIO)
	{
		display.draw_fill_rect (11,53,73,61, CLCDDisplay::PIXEL_OFF);
		int dp = int( perc/100.0*61.0+12.0);
		display.draw_fill_rect (11,54,dp,60, CLCDDisplay::PIXEL_ON);
		if(isMuted)
		{
			if(dp > 12)
			{
				display.draw_line(12, 56, dp-1, 56, CLCDDisplay::PIXEL_OFF);
				display.draw_line(12, 58, dp-1, 58, CLCDDisplay::PIXEL_OFF);
			}
			else
				display.draw_line (12,55,72,59, CLCDDisplay::PIXEL_ON);
		}
		displayUpdate();
	}
}

void CLCD::setMode(const MODES m, const char * const title)
{
	mode = m;
	setlcdparameter();
//TODO:	setup
	int LCD_SHOW_VOLUME = 0;

	switch (m)
	{
	case MODE_TVRADIO: 
/*		switch (LCD_SHOW_VOLUME)
		{
		case 0:
			display.load_screen(&(background[BACKGROUND_LCD2]));
			showPercentOver(percentOver, false);
			break;
		case 1:
			display.load_screen(&(background[BACKGROUND_LCD]));
			showVolume(volume, false);
			break;
		case 2:
			display.load_screen(&(background[BACKGROUND_LCD3]));
			showVolume(volume, false);
			showPercentOver(percentOver, false);
			break;
		}
		showServicename(servicename);
		showclock = true;
*/
//		showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
	
		break;
	case MODE_AUDIO:
	{
		display.load_screen(&(background[BACKGROUND_LCD]));
		display.draw_fill_rect (0,14,120,48, CLCDDisplay::PIXEL_OFF);
		
		showAudioPlayMode(AUDIO_MODE_STOP);
		showVolume(volume, false);
		showclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
		break;
	}
	case MODE_SCART:
		display.load_screen(&(background[BACKGROUND_LCD]));
		showVolume(volume, false);
		showclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
		break;
	case MODE_MENU_UTF8:
		showclock = false;
		display.load_screen(&(background[BACKGROUND_SETUP]));
		fonts.menutitle->RenderString(0,28, 140, title, CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
		displayUpdate();
		break;
	case MODE_SHUTDOWN:
		showclock = false;
		display.load_screen(&(background[BACKGROUND_POWER]));
		displayUpdate();
		break;
	case MODE_STANDBY:
		showclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
		                 /* "showTime()" clears the whole lcd in MODE_STANDBY                         */
		break;
	}
	wake_up();
}


void CLCD::setBrightness(int bright)
{
	LCD_BRIGHTNESS = bright;
	setlcdparameter();
}

int CLCD::getBrightness()
{
	return LCD_BRIGHTNESS;
}

void CLCD::setBrightnessStandby(int bright)
{
	LCD_STANDBY_BRIGHTNESS = bright;
	setlcdparameter();
}

int CLCD::getBrightnessStandby()
{
	return LCD_STANDBY_BRIGHTNESS;
}

void CLCD::setContrast(int contrast)
{
	LCD_CONTRAST = contrast;
	setlcdparameter();
}

int CLCD::getContrast()
{
	return LCD_CONTRAST;
}

void CLCD::setPower(int power)
{
	LCD_POWER = power;
	setlcdparameter();
}

int CLCD::getPower()
{
	return LCD_POWER;
}

void CLCD::togglePower(void)
{
	last_toggle_state_power = 1 - last_toggle_state_power;
	setlcdparameter((mode == MODE_STANDBY) ? LCD_STANDBY_BRIGHTNESS : LCD_BRIGHTNESS,
			LCD_CONTRAST,
			last_toggle_state_power,
			LCD_INVERSE);
}

void CLCD::setInverse(int inverse)
{
	LCD_INVERSE = inverse;
	setlcdparameter();
}

int CLCD::getInverse()
{
	return LCD_INVERSE;
}

void CLCD::setAutoDimm(int autodimm)
{
	int fd;
	LCD_AUTODIMM = autodimm;

	if ((fd = open("/dev/dbox/fp0", O_RDWR)) == -1)
	{
		perror("[rbx lcdd] open '/dev/dbox/fp0' failed");
	}
	else
	{
		if( ioctl(fd, FP_IOCTL_LCD_AUTODIMM, &autodimm) < 0 )
		{
			perror("[rbx lcdd] set autodimm failed!");
		}

		close(fd);
	}
}

int CLCD::getAutoDimm()
{
	return LCD_AUTODIMM;
}

void CLCD::setMuted(bool mu)
{
	muted = mu;
	showVolume(volume);
}

void CLCD::resume()
{
	display.resume();
}

void CLCD::pause()
{
	display.pause();
}

//  MENU ////////////////////////////////////////////////////////////////////

void CLCD::setMenuLine( const std::string _line )
{
	int i;
	
	for( i = 0; i < NUM_MENU_LINES && menu_line_set[i]; i++ );

	if( i == NUM_MENU_LINES )
		return;

	menu_lines[i] = _line;
	menu_line_set[i] = true;
		
}

void CLCD::setMenuCursors( const bool _top, const bool _bottom )
{
	this->menu_top = _top;
	this->menu_bottom = _bottom;
}

void CLCD::setMenuSelected( const int _selected )
{
	if( _selected < 0 )
		this->menu_selected = 0;
	else if( _selected >= NUM_MENU_LINES )
		this->menu_selected = NUM_MENU_LINES - 1;
	else
		this->menu_selected = _selected;
}

void CLCD::ClearMenu()
{
	display.draw_fill_rect ( -1, -1 , 122, 70, CLCDDisplay::PIXEL_OFF );
	this->menu_title = "";
	this->menu_top = false;
	this->menu_bottom = false;
	this->menu_selected = 0;
	int i;
	for( i = 0; i < NUM_MENU_LINES; i++ )
	{
		this->menu_lines[i] = "";
		this->menu_line_set[i] = false;
	}
}

void CLCD::ShowMenu( unsigned long	_frame )
{
	int i;
	int title_width = fonts.menutitle->getRenderWidth( menu_title.c_str(), true );
	
	
	display.draw_fill_rect ( 0, 0 , 120, 16, CLCDDisplay::PIXEL_ON );

	if( title_width > 120 )		
		fonts.menutitle->RenderString( 5 , 12, 120, menu_title.c_str() , CLCDDisplay::PIXEL_OFF, 0, true ); // UTF-8
	else
	{
		int x = (120 - title_width) / 2;
		fonts.menutitle->RenderString( x , 12, 120, menu_title.c_str() , CLCDDisplay::PIXEL_OFF, 0, true ); // UTF-8
	}

	for( i = 0; i < 5; i++ )
	{
		
		display.draw_line( 0 * 2   ,16 + i , 54 , 16 + i, CLCDDisplay::PIXEL_ON);
		display.draw_line( 120   ,16 + i , 66  , 16 + i, CLCDDisplay::PIXEL_ON);

		display.draw_line( 0   ,59 - i , 54 , 59 - i, CLCDDisplay::PIXEL_ON);
		display.draw_line( 120   ,59 - i , 66  , 59  - i, CLCDDisplay::PIXEL_ON);

		if( menu_top )
		{
			display.draw_line( 60 - i   ,16 + i , 60 + i , 16 + i, CLCDDisplay::PIXEL_ON);
		}

		if( menu_bottom )
		{
			display.draw_line( 60 - i  ,59 - i, 60 + i  , 59 - i, CLCDDisplay::PIXEL_ON);	
		}
	}	


	for( i = 0; i < NUM_MENU_LINES; i++ )
	{
		int text = CLCDDisplay::PIXEL_ON;
		int background = CLCDDisplay::PIXEL_OFF;
		
		if( false == menu_line_set[i] ) break;
	
		if( i == menu_selected )
		{
			background = CLCDDisplay::PIXEL_ON;
			text = CLCDDisplay::PIXEL_OFF;
		}
	
		display.draw_fill_rect ( 1, i * 10 + 7 + 16, 119, i * 10 + 18 + 16, background );
		fonts.menu->RenderString( 5 , i * 10 + 16 + 16, 119, menu_lines[i].c_str() , text, 0, true ); // UTF-8
	}

//	display.draw_fill_rect ( 0, 0 , 119, 59, CLCDDisplay::PIXEL_ON );

	displayUpdate();
}

void CLCD::DrawString( 
					const int			_x,
					const int			_y,
					const int			_width,	
					const std::string	_text,
					LcdFont*		_font,
					int			_font_size,
					const bool			_invert,
					const unsigned long	_frame )
{
	static int x = 0;
	static int y = 0;
	static std::vector<int>	sizes;
	static int text_width = 0;
	int i = 0;
	int fg = (false==_invert)?CLCDDisplay::PIXEL_ON:CLCDDisplay::PIXEL_OFF;
	int bg = (false==_invert)?CLCDDisplay::PIXEL_OFF:CLCDDisplay::PIXEL_ON;
	unsigned long frame = 0;

	if( 0 == _frame )
	{

		
		x = 0;
		y = 0;
	
		//prepare sizes

		sizes.erase(sizes.begin(),sizes.end());

		for( i = 0; i < _text.size(); i++ )
		{
			int s = _font->getRenderWidth( ( _text.substr( 0, i ) ).c_str(), false );
			sizes.push_back( s );
		}		

		text_width = _font->getRenderWidth( _text.c_str(), false );
	}
	frame = _frame%text_width;

//	display.draw_fill_rect ( _x, _y , _x + 120-_width, _y + 64 - 17 , fg );
	display.draw_fill_rect ( _x, _y-_font_size+3 , _x+_width, _y+3 , bg );

	if( 120 < text_width )
	{
		int diff;

//		std::cout << "***************" << std::endl;
				
		for( i = 0; i < _text.size(); i++ )
		{
/*			std::cout << 
							"[" << i << "] " << 
							"size = " << sizes[i] << 
							" _frame = " << _frame << 
							" strlen = " << _filename.size() << 
							" n of sizes = " << sizes.size() << 
							" " << _filename.substr( i, _filename.size() - i ) <<  std::endl;
*/
			if( sizes[i] > frame )
			{
				diff = sizes[i] - frame;
				break;
			}
		}

		_font->RenderString( _x + 1 + diff , _y, _width, _text.substr( i, _text.size() - i ).c_str() , fg, 0, false ); // UTF-8
	
		if( text_width - frame < 120 )
		{
		_font->RenderString( text_width - frame + 5 + _x , _y, _width - ( text_width - frame ) , _text.c_str(), fg, 0, false ); // UTF-8
		}

		if( frame > text_width )
		{
		_font->RenderString( _x + 5 - (text_width - frame) * -1, _y, _width, _text.c_str(), fg, 0, false ); // UTF-8
		}
	}
	else
		_font->RenderString( _x, _y, _width, _text.c_str() , fg, 0, true ); // UTF-8
}


void CLCD::setMenuTitle( const std::string _title )
{
	this->menu_title = _title;
}

// PLAYING LOCATION SCREEN //////////////////////////////////////////////////////////////////

unsigned long CLCD::ShowPlayingFile( const std::string _filename, const char _percentover, std::string _tplayed, unsigned long _frame )
{
	static int x = 0;
	static int y = 0;
	static std::vector<int>	sizes;
	static int title_width = 0;
	int i = 0;

	if( 0 == _frame )
	{
		x = 0;
		y = 0;
	
		// clear screen
		display.draw_fill_rect ( -1, -1 , 122, 70, CLCDDisplay::PIXEL_OFF );
		display.draw_fill_rect (9,51,75,63, CLCDDisplay::PIXEL_ON);
		display.draw_fill_rect (11,53,73,61, CLCDDisplay::PIXEL_OFF);
	}

	display.draw_fill_rect ( 78, 50 , 120, 64, CLCDDisplay::PIXEL_OFF );
	fonts.time->RenderString( 78, 62, 52, _tplayed.c_str() , CLCDDisplay::PIXEL_ON, 0, false ); // UTF-8
	

// 	if( _frame >= title_width + 5)
// 	{
// 		_frame = 1;
// 	} 

	
	if( 0 == _frame%5 )
		showPercentOver( _percentover, false );

	DrawString( 0, 40, 120, _filename, fonts.channelname, fonts.channelname_size, false, _frame );


	displayUpdate();
	return _frame+1;

}

#if 0
// PLAYING LOCATION SCREEN //////////////////////////////////////////////////////////////////

unsigned long CLCD::ShowPlayingFile( const std::string _filename, const char _percentover, std::string _tplayed, unsigned long _frame )
{
	static int x = 0;
	static int y = 0;
	static std::vector<int>	sizes;
	static int title_width = 0;
	int i = 0;

	if( 0 == _frame )
	{

		
		x = 0;
		y = 0;
	
		//prepare sizes

		sizes.erase(sizes.begin(),sizes.end());

		for( i = 0; i < _filename.size(); i++ )
		{
			int s = fonts.channelname->getRenderWidth( ( _filename.substr( 0, i ) ).c_str(), false );
			sizes.push_back( s );
		}		

		title_width = fonts.channelname->getRenderWidth( _filename.c_str(), false );

		// clear screen
		display.draw_fill_rect ( -1, -1 , 122, 70, CLCDDisplay::PIXEL_OFF );
		display.draw_fill_rect (9,51,75,63, CLCDDisplay::PIXEL_ON);
		display.draw_fill_rect (11,53,73,61, CLCDDisplay::PIXEL_OFF);
	}

	display.draw_fill_rect ( 78, 50 , 120, 64, CLCDDisplay::PIXEL_OFF );
	fonts.time->RenderString( 78, 62, 52, _tplayed.c_str() , CLCDDisplay::PIXEL_ON, 0, false ); // UTF-8
	

	if( _frame >= title_width + 5)
	{
		_frame = 1;
	} 

	if( 120 < title_width )
	{
		int diff;

		display.draw_fill_rect ( 00, 27 , 121, 44, CLCDDisplay::PIXEL_OFF );
			
//		std::cout << "***************" << std::endl;
				
		for( i = 0; i < _filename.size(); i++ )
		{
/*			std::cout << 
							"[" << i << "] " << 
							"size = " << sizes[i] << 
							" _frame = " << _frame << 
							" strlen = " << _filename.size() << 
							" n of sizes = " << sizes.size() << 
							" " << _filename.substr( i, _filename.size() - i ) <<  std::endl;
*/
			if( sizes[i] > _frame )
			{
				diff = sizes[i] - _frame;
				break;
			}
		}

		fonts.channelname->RenderString( 1 + diff , 40, 120, _filename.substr( i, _filename.size() - i ).c_str() , CLCDDisplay::PIXEL_ON, 0, false ); // UTF-8
	
		if( title_width - _frame < 120 )
		{
		fonts.channelname->RenderString( title_width - _frame + 5 , 40, 120 - ( title_width - _frame ) , _filename.c_str(), CLCDDisplay::PIXEL_ON, 0, false ); // UTF-8
		}

		if( _frame > title_width )
		{
		fonts.channelname->RenderString( 5 - (title_width - _frame) * -1, 40, 120, _filename.c_str(), CLCDDisplay::PIXEL_ON, 0, false ); // UTF-8
		}



	}
	else
		fonts.channelname->RenderString( 0, 40, 120, _filename.c_str() , CLCDDisplay::PIXEL_ON, 0, true ); // UTF-8
	
	if( 0 == _frame%5 )
		showPercentOver( _percentover, false );

	displayUpdate();
	return _frame+1;

}

#endif



