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

#include <driver/lcdd.h>

#include <global.h>
#include <system/settings.h>

#include <driver/newclock.h>
#include <lcddisplay/lcddisplay.h>

#include <dbox/fp.h>
#include <fcntl.h>
#include <sys/timeb.h>
#include <time.h>
#include <unistd.h>


CLCD::CLCD()
	: configfile('\t')
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

void* CLCD::TimeThread(void *)
{
	while(1)
	{
		sleep(10);
		CLCD::getInstance()->showTime();
	}
	return NULL;
}

void CLCD::init(const char * fontfile, const char * fontname)
{
	InitNewClock();

	if (!lcdInit(fontfile, fontname))
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

bool CLCD::lcdInit(const char * fontfile, const char * fontname)
{
	fontRenderer = new LcdFontRenderClass(&display);
	fontRenderer->AddFont(fontfile);
	fontRenderer->InitFontCache();

	fonts.channelname = fontRenderer->getFont(fontname, "Regular", 15);
	fonts.time        = fontRenderer->getFont(fontname, "Regular", 14);
	fonts.menutitle   = fontRenderer->getFont(fontname, "Regular", 15);
	fonts.menu        = fontRenderer->getFont(fontname, "Regular", 12);

	setAutoDimm(g_settings.lcd_setting[SNeutrinoSettings::LCD_AUTODIMM]);

	if (!display.isAvailable())
	{
		printf("exit...(no lcd-support)\n");
		return false;
	}

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

	setMode(MODE_TVRADIO);

	return true;
}

void CLCD::setlcdparameter(int dimm, const int contrast, const int power, const int inverse)
{
	int fd;
	if (power == 0)
		dimm = 0;

	if ((fd = open("/dev/dbox/fp0", O_RDWR)) == -1)
	{
		perror("[lcdd] open '/dev/dbox/fp0' failed");
	}
	else
	{
		if (ioctl(fd, FP_IOCTL_LCD_DIMM, &dimm) < 0)
		{
			perror("[lcdd] set dimm failed!");
		}

		close(fd);
	}
	
	if ((fd = open("/dev/dbox/lcd0", O_RDWR)) == -1)
	{
		perror("[lcdd] open '/dev/dbox/lcd0' failed");
	}
	else
	{
		if (ioctl(fd, LCD_IOCTL_SRV, &contrast) < 0)
		{
			perror("[lcdd] set contrast failed!");
		}

		if (ioctl(fd, LCD_IOCTL_ON, &power) < 0)
		{
			perror("[lcdd] set power failed!");
		}

		if (ioctl(fd, LCD_IOCTL_REVERSE, &inverse) < 0)
		{
			perror("[lcdd] set invert failed!");
		}

		close(fd);
	}
}

void CLCD::setlcdparameter(void)
{
	setlcdparameter((mode == MODE_STANDBY) ? g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS] : g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS],
			g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST],
			g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER],
			g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE]);
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
	display.update();
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

			ShowNewClock(&display, t->tm_hour, t->tm_min, t->tm_wday, t->tm_mday, t->tm_mon);
		}
		else
		{
			strftime((char*) &timestr, 20, "%H:%M", t);

			display.draw_fill_rect (77, 50, 120, 64, CLCDDisplay::PIXEL_OFF);

			fonts.time->RenderString(122 - fonts.time->getRenderWidth(timestr), 62, 50, timestr, CLCDDisplay::PIXEL_ON);
		}
		display.update();
	}
}

void CLCD::showVolume(const char vol, const bool perform_update)
{
	volume = vol;
	if (
	    ((mode == MODE_TVRADIO) && (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME])) ||
	    (mode == MODE_SCART) ||
	    (mode == MODE_MP3)
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

		if (perform_update)
		  display.update();
	}
}
void CLCD::showPercentOver(const unsigned char perc, const bool perform_update)
{
	percentOver = perc;
	if (mode == MODE_TVRADIO)
	{
		if (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME] == 0)
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
				display.update();
		}
		else if (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME] == 2)
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
				display.update();
		}
	}
}

void CLCD::showMenuText(const int position, const std::string & text, const int highlight, const bool utf_encoded)
{
	if (mode != MODE_MENU_UTF8)
		return;

	// reload specified line
	display.draw_fill_rect(-1,35+14*position,120,35+14+14*position, CLCDDisplay::PIXEL_OFF);
	fonts.menu->RenderString(0,35+11+14*position, 140, text.c_str() , CLCDDisplay::PIXEL_INV, highlight, utf_encoded);
	display.update();
}

void CLCD::showMP3(const std::string & artist, const std::string & title, const std::string & album)
{
	if (mode != MODE_MP3) 
	{
		return;
	}
	// reload specified line
	display.draw_fill_rect (-1,10,104,24, CLCDDisplay::PIXEL_OFF);
	display.draw_fill_rect (-1,20,104,37, CLCDDisplay::PIXEL_OFF);
	display.draw_fill_rect (-1,33,121,50, CLCDDisplay::PIXEL_OFF);
	fonts.menu->RenderString(0,22, 107, artist.c_str() , CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
	fonts.menu->RenderString(0,35, 107, album.c_str() , CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
	fonts.menu->RenderString(0,48, 125, title.c_str() , CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
	display.update();
}

void CLCD::showMP3Play(MP3MODES m)
{
	display.draw_fill_rect (109,21,119,31, CLCDDisplay::PIXEL_OFF);
	switch(m)
	{
		case MP3_PLAY:
			{
				int x=112,y=22;
				display.draw_line(x  ,y  ,x  ,y+8, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+1,y+1,x+1,y+7, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+2,y+2,x+2,y+6, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+3,y+3,x+3,y+5, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+4,y+4,x+4,y+4, CLCDDisplay::PIXEL_ON);
				break;
			}
		case MP3_STOP:
			display.draw_fill_rect (110,22,118,30, CLCDDisplay::PIXEL_ON);
			break;
		case MP3_PAUSE:
			display.draw_line(111,23,111,29, CLCDDisplay::PIXEL_ON);
			display.draw_line(112,23,112,29, CLCDDisplay::PIXEL_ON);
			display.draw_line(116,23,116,29, CLCDDisplay::PIXEL_ON);
			display.draw_line(117,23,117,29, CLCDDisplay::PIXEL_ON);
			break;
		case MP3_FF:
			display.draw_line(111,23,114,26, CLCDDisplay::PIXEL_ON);
			display.draw_line(114,26,110,29, CLCDDisplay::PIXEL_ON);
			display.draw_line(114,23,117,26, CLCDDisplay::PIXEL_ON);
			display.draw_line(117,26,113,29, CLCDDisplay::PIXEL_ON);
			break;
		case MP3_REV:
			break;
	}
	display.update();
}
void CLCD::setMode(const MODES m, const char * const title)
{
	mode = m;
	setlcdparameter();

	switch (m)
	{
	case MODE_TVRADIO:
		switch (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME])
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
		showTime();      /* "showclock = true;" implies that "showTime();" does a "display.update();" */
		break;
	case MODE_MP3:
	{
		display.load_screen(&(background[BACKGROUND_LCD]));
		display.draw_fill_rect (0,14,120,48, CLCDDisplay::PIXEL_OFF);
		int x=106,y=14;
		display.draw_line(x  ,y  ,x  ,y+6,CLCDDisplay::PIXEL_ON);
		display.draw_line(x  ,y  ,x+2,y+2,CLCDDisplay::PIXEL_ON);
		display.draw_line(x+2,y+2,x+4,y  ,CLCDDisplay::PIXEL_ON);
		display.draw_line(x+4,y  ,x+4,y+6,CLCDDisplay::PIXEL_ON);  
		
		display.draw_line(x+6,y  ,x+6,y+6,CLCDDisplay::PIXEL_ON);  
		display.draw_line(x+6,y  ,x+9,y  ,CLCDDisplay::PIXEL_ON);
		display.draw_line(x+6,y+3,x+9,y+3,CLCDDisplay::PIXEL_ON);
		display.draw_line(x+9,y  ,x+9,y+3,CLCDDisplay::PIXEL_ON);  
		
		display.draw_line(x+13,y  ,x+13,y+6,CLCDDisplay::PIXEL_ON);
		display.draw_line(x+11,y  ,x+13,y  ,CLCDDisplay::PIXEL_ON);
		display.draw_line(x+11,y+3,x+13,y+3,CLCDDisplay::PIXEL_ON);
		display.draw_line(x+11,y+6,x+13,y+6,CLCDDisplay::PIXEL_ON);
		
		display.draw_line(x-2,12,x-2,32, CLCDDisplay::PIXEL_ON);
		display.draw_line(x-2,32,x+14,32, CLCDDisplay::PIXEL_ON);
		
		showMP3Play(MP3_STOP);
		showVolume(volume, false);
		showclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "display.update();" */
		break;
	}
	case MODE_SCART:
		display.load_screen(&(background[BACKGROUND_LCD]));
		showVolume(volume, false);
		showclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "display.update();" */
		break;
	case MODE_MENU_UTF8:
		showclock = false;
		display.load_screen(&(background[BACKGROUND_SETUP]));
		fonts.menutitle->RenderString(-1,28, 140, title, CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
		display.update();
		break;
	case MODE_SHUTDOWN:
		showclock = false;
		display.load_screen(&(background[BACKGROUND_POWER]));
		display.update();
		break;
	case MODE_STANDBY:
		showclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "display.update();" */
		                 /* "showTime()" clears the whole lcd in MODE_STANDBY                         */
		break;
	}
}


void CLCD::setBrightness(int bright)
{
	g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS] = bright;
	setlcdparameter();
}

int CLCD::getBrightness()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS];
}

void CLCD::setBrightnessStandby(int bright)
{
	g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS] = bright;
	setlcdparameter();
}

int CLCD::getBrightnessStandby()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS];
}

void CLCD::setContrast(int contrast)
{
	g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST] = contrast;
	setlcdparameter();
}

int CLCD::getContrast()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST];
}

void CLCD::setPower(int power)
{
	g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER] = power;
	setlcdparameter();
}

int CLCD::getPower()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER];
}

void CLCD::setInverse(int inverse)
{
	g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE] = inverse;
	setlcdparameter();
}

int CLCD::getInverse()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE];
}

void CLCD::setAutoDimm(int autodimm)
{
	int fd;
	g_settings.lcd_setting[SNeutrinoSettings::LCD_AUTODIMM] = autodimm;

	if ((fd = open("/dev/dbox/fp0", O_RDWR)) == -1)
	{
		perror("[lcdd] open '/dev/dbox/fp0' failed");
	}
	else
	{
		if( ioctl(fd, FP_IOCTL_LCD_AUTODIMM, &autodimm) < 0 )
		{
			perror("[lcdd] set autodimm failed!");
		}

		close(fd);
	}
}

int CLCD::getAutoDimm()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_AUTODIMM];
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
