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


#include <config.h>

#include "lcdd.h"
#include "newclock.h"
#include <dbox/fp.h>
#include <sys/timeb.h>
#include <time.h>


CLCD::CLCD()
	: configfile('\t')
{
}

CLCD::~CLCD()
{
	saveConfig();
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

void CLCD::saveConfig()
{
	static bool inSave=false;
	if(inSave==false)
	{
		inSave=true;
		configfile.setInt32( "lcd_brightness", lcd_brightness );
		configfile.setInt32( "lcd_standbybrightness", lcd_standbybrightness );
		configfile.setInt32( "lcd_contrast", lcd_contrast );
		configfile.setInt32( "lcd_power", lcd_power );
		configfile.setInt32( "lcd_inverse", lcd_inverse );

		if(configfile.getModifiedFlag())
		{
			printf("[lcdd] save config\n");
			configfile.saveConfig(CONFIGDIR "/lcdd.conf");
		}
		inSave=false;
	}
}

void CLCD::loadConfig()
{
	printf("[lcdd] load config\n");
	if(!configfile.loadConfig(CONFIGDIR "/lcdd.conf"))
	{
		lcd_brightness = 0xff;
		lcd_standbybrightness = 0xaa;
		lcd_contrast = 0x0F;
		lcd_power = 0x01;
		lcd_inverse = 0x00;
		return;
	}

	lcd_brightness =  configfile.getInt32("lcd_brightness", 0xff);
	lcd_standbybrightness = configfile.getInt32("lcd_standbybrightness", 0xaa);
	lcd_contrast = configfile.getInt32("lcd_contrast", 0x0F);
	lcd_power = configfile.getInt32("lcd_power", 0x01);
	lcd_inverse = configfile.getInt32("lcd_inverse", 0x00);
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

void CLCD::init()
{
	printf("Doing LCD-Init\n");
	InitNewClock();

	loadConfig();

	if(!lcdInit())
	{
		//fehler beim lcd-init aufgetreten
		printf("LCD-Init failed!\n");
		return;
	}

	if (pthread_create (&thrTime, NULL, TimeThread, NULL) != 0 )
	{
		perror("[lcdd]: pthread_create(TimeThread)");
		return ;
	}
	printf("exit LCD-Init\n");
}

bool CLCD::lcdInit()
{
	fontRenderer = new fontRenderClass( &display );
	fontRenderer->AddFont(FONTDIR "/micron.ttf");
	fontRenderer->InitFontCache();

	#define FONTNAME "Micron"
	fonts.channelname=fontRenderer->getFont(FONTNAME, "Regular", 15);
	fonts.time=fontRenderer->getFont(FONTNAME, "Regular", 14);
	fonts.menutitle=fontRenderer->getFont(FONTNAME, "Regular", 15);
	fonts.menu=fontRenderer->getFont(FONTNAME, "Regular", 12);

	setlcdparameter(lcd_brightness, lcd_contrast, lcd_power, lcd_inverse);
	display.setIconBasePath( DATADIR "/lcdd/icons/");

	if(!display.isAvailable())
	{
		printf("exit...(no lcd-support)\n");
		return false;
	}

	if (!display.paintIcon("neutrino_setup.raw",0,0,false))
	{
		printf("exit...(no neutrino_setup.raw)\n");
		return false;
	}
	display.dump_screen(&icon_setup);

	if (!display.paintIcon("neutrino_power.raw",0,0,false))
	{
		printf("exit...(no neutrino_power.raw)\n");
		return false;
	}
	display.dump_screen(&icon_power);

	if (!display.paintIcon("neutrino_lcd.raw",0,0,false))
	{
		printf("exit...(no neutrino_lcd.raw)\n");
		return false;
	}
	display.dump_screen(&icon_lcd);

	mode = MODE_TVRADIO;
	showServicename("Booting...");
	showclock=true;
	return true;
}

void CLCD::setlcdparameter(int dimm, int contrast, int power, int inverse)
{
	int fp, fd;
	if (power==0)
		dimm=0;

	if ((fp = open("/dev/dbox/fp0",O_RDWR)) <= 0)
	{
		perror("[lcdd] pen '/dev/dbox/fp0' failed!");
	}

	if ((fd = open("/dev/dbox/lcd0",O_RDWR)) <= 0)
	{
		perror("[lcdd] open '/dev/dbox/lcd0' failed!");
	}

	if (ioctl(fp,FP_IOCTL_LCD_DIMM, &dimm) < 0)
	{
		perror("[lcdd] set dimm failed!");
	}
	
	if (ioctl(fd,LCD_IOCTL_SRV, &contrast) < 0)
	{
		perror("[lcdd] set contrast failed!");
	}

	if (ioctl(fd,LCD_IOCTL_ON, &power) < 0)
	{
		perror("[lcdd] set power failed!");
	}

	if (ioctl(fd,LCD_IOCTL_REVERSE, &inverse) < 0)
	{
		perror("[lcdd] set invert failed!");
	}

	close(fp);
	close(fd);
}

void CLCD::showServicename( string name )
{
	servicename = name;
	if (mode != MODE_TVRADIO)
		return;

	display.draw_fill_rect (0,14,120,48, CLCDDisplay::PIXEL_OFF);

	if (fonts.channelname->getRenderWidth(name.c_str(), true) > 120)
	{
		int pos;
		string text1 = name;
		do
		{
			pos = text1.find_last_of("[ .]+"); // <- characters are UTF-encoded!
			if (pos != -1)
				text1 = text1.substr( 0, pos );
		} while ( ( pos != -1 ) && ( fonts.channelname->getRenderWidth(text1.c_str(), true) > 120 ) );
		
		if ( fonts.channelname->getRenderWidth(text1.c_str(), true) <= 120 )
			fonts.channelname->RenderString(1,29+16, 130, name.substr(text1.length()+ 1).c_str(), CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
		else
		{
			string text1 = name;
			while (fonts.channelname->getRenderWidth(text1.c_str(), true) > 120)
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
	char timestr[50];
	struct timeb tm;
	if (showclock)
	{
		ftime(&tm);
		strftime((char*) &timestr, 20, "%H:%M", localtime(&tm.time) );

		if(mode!=MODE_STANDBY)
		{
			display.draw_fill_rect (77,50,120,64, CLCDDisplay::PIXEL_OFF);
			int pos = 122 - fonts.time->getRenderWidth(timestr);
			fonts.time->RenderString(pos,62, 50, timestr, CLCDDisplay::PIXEL_ON);
		}
		else
		{
			//big clock
			struct tm *t = localtime(&tm.time);

			display.draw_fill_rect (-1,-1,120,64, CLCDDisplay::PIXEL_OFF);
			ShowNewClock(&display, t->tm_hour, t->tm_min, t->tm_wday, t->tm_mday, t->tm_mon);
		}
		display.update();
	}
}

void CLCD::showVolume(char vol)
{
	volume = vol;
	if ((mode==MODE_TVRADIO) || (mode==MODE_SCART))
	{
		display.draw_fill_rect (11,53,73,61, CLCDDisplay::PIXEL_OFF);
		//strichlin
		if (muted)
		{
			display.draw_line (12,55,73,60, CLCDDisplay::PIXEL_ON);
		}
		else
		{
			int dp = int( vol/100.0*61.0+12.0);
			display.draw_fill_rect (11,54,dp,60, CLCDDisplay::PIXEL_ON);
		}

		display.update();
	}
}

void CLCD::showMenuText(int position, string text, int highlight )
{
	if (mode != MODE_MENU)
	{
		return;
	}
	// reload specified line
	display.draw_fill_rect(-1,35+14*position,120,35+14+14*position, CLCDDisplay::PIXEL_OFF);
	fonts.menu->RenderString(0,35+11+14*position, 140, text.c_str() , CLCDDisplay::PIXEL_INV, highlight);
	display.update();
}

void CLCD::setMode(MODES m, string title)
{
	switch (m)
	{
		case MODE_TVRADIO:
			setlcdparameter(lcd_brightness, lcd_contrast, lcd_power, lcd_inverse);
			//printf("[lcdd] mode: tvradio\n");
			display.load_screen(&icon_lcd);
			mode = m;
			showclock = true;
			showVolume(volume);
			showServicename(servicename);
			showTime();
			display.update();
			break;
		case MODE_SCART:
			setlcdparameter(lcd_brightness, lcd_contrast, lcd_power, lcd_inverse);
			//printf("[lcdd] mode: scart\n");
			display.load_screen(&icon_lcd);
			mode = m;
			showclock = true;
			showVolume(volume);
			showTime();
			display.update();
			break;
		case MODE_MENU:
		case MODE_MENU_UTF8:
			setlcdparameter(lcd_brightness, lcd_contrast, lcd_power, lcd_inverse);
			//printf("[lcdd] mode: menu\n");
			mode = m;
			showclock = false;
			display.load_screen(&icon_setup);
			fonts.menutitle->RenderString(-1,28, 140, title.c_str(), CLCDDisplay::PIXEL_ON, 0, m == MODE_MENU_UTF8);
			display.update();
			break;
		case MODE_SHUTDOWN:
			setlcdparameter(lcd_brightness, lcd_contrast, lcd_power, lcd_inverse);
			//printf("[lcdd] mode: shutdown\n");
			mode = m;
			showclock = false;
			display.load_screen(&icon_power);
			display.update();
			break;
		case MODE_STANDBY:
			//printf("[lcdd] mode: standby\n");
			setlcdparameter(lcd_standbybrightness, lcd_contrast, lcd_power, lcd_inverse);
			mode = m;
			showclock = true;
			display.draw_fill_rect (-1,0,120,64, CLCDDisplay::PIXEL_OFF);
			showTime();
			display.update();
			break;
		default:
			printf("[lcdd] Unknown mode: %i\n", m);
	}
}


void CLCD::setBrightness(int bright)
{
	lcd_brightness = bright;
}

int CLCD::getBrightness()
{
	return lcd_brightness;
}

void CLCD::setBrightnessStandby(int bright)
{
	lcd_standbybrightness = bright;
}

int CLCD::getBrightnessStandby()
{
	return lcd_standbybrightness;
}

void CLCD::setContrast(int contrast)
{
	lcd_contrast = contrast;
}

int CLCD::getContrast()
{
	return lcd_contrast;
}

void CLCD::setPower(int power)
{
	lcd_power = power;
}

int CLCD::getPower()
{
	return lcd_power;
}

void CLCD::setInverse(int inverse)
{
	lcd_inverse = inverse;
}

int CLCD::getInverse()
{
	return lcd_inverse;
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
