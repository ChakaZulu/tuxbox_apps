/*
	$Id: lcdd.cpp,v 1.53 2007/06/03 14:36:13 dbluelle Exp $

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
#include <neutrino.h>
#include <system/settings.h>

#include <driver/newclock.h>
#include <lcddisplay/lcddisplay.h>

#include <dbox/fp.h>
#include <fcntl.h>
#include <sys/timeb.h>
#include <time.h>
#include <unistd.h>

#include <daemonc/remotecontrol.h>
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

CLCD::CLCD()
	: configfile('\t')
{
#ifdef LCD_UPDATE
	m_fileList = NULL;
	m_fileListPos = 0;
	m_fileListHeader = "";
	m_infoBoxText = "";
	m_infoBoxAutoNewline = 0;
	m_progressShowEscape = 0;
	m_progressHeaderGlobal = "";
	m_progressHeaderLocal = "";
	m_progressGlobal = 0;
	m_progressLocal = 0;
#endif // LCD_UPDATE
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
			setlcdparameter();
		}
	} 
}

void CLCD::wake_up() {
	if (atoi(g_settings.lcd_setting_dim_time) > 0) {
		timeout_cnt = atoi(g_settings.lcd_setting_dim_time);
		setlcdparameter();
	}
}

void* CLCD::TimeThread(void *)
{
	while(1)
	{
		sleep(1);
		struct stat buf;
		if (stat("/tmp/lcd.locked", &buf) == -1) {
			CLCD::getInstance()->showTime();
			CLCD::getInstance()->count_down();
		} else
			CLCD::getInstance()->wake_up();
	}
	return NULL;
}

void CLCD::init(const char * fontfile, const char * fontname,
                const char * fontfile2, const char * fontname2,
                const char * fontfile3, const char * fontname3)
{
	InitNewClock();

	if (!lcdInit(fontfile, fontname, fontfile2, fontname2, fontfile3, fontname3 ))
	{
		printf("[lcdd] LCD-Init failed!\n");
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
	BACKGROUND_LCD   = 4,
    BACKGROUND_LCD4  = 5
};
const char * const background_name[LCD_NUMBER_OF_BACKGROUNDS] = {
	"setup",
	"power",
	"lcd2",
	"lcd3",
	"lcd",
    "lcd4"
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

	fonts.menu        = fontRenderer->getFont(fontname,  style_name , 12);
	fonts.time        = fontRenderer->getFont(fontname2, style_name2, 14);
	fonts.channelname = fontRenderer->getFont(fontname3, style_name3, 15);
	fonts.menutitle   = fonts.channelname;

	setAutoDimm(g_settings.lcd_setting[SNeutrinoSettings::LCD_AUTODIMM]);

	if (!display.isAvailable())
	{
		printf("[lcdd] exit...(no lcd-support)\n");
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

void CLCD::displayUpdate()
{
	struct stat buf;
	if (stat("/tmp/lcd.locked", &buf) == -1)
		display.update();
}

void CLCD::setlcdparameter(int dimm, const int contrast, const int power, const int inverse)
{
	if (!display.isAvailable())
		return;

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
	bool timeouted = (atoi(g_settings.lcd_setting_dim_time) > 0)
	  & (timeout_cnt == 0);
	last_toggle_state_power = g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER];
	setlcdparameter((mode == MODE_STANDBY)
			? g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS]
			: timeouted 
			? atoi(g_settings.lcd_setting_dim_brightness)
			: g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS],
			g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST],
			last_toggle_state_power
			& (! timeouted
			   || (atoi(g_settings.lcd_setting_dim_brightness) > 0)),
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

			ShowNewClock(&display, t->tm_hour, t->tm_min, t->tm_wday, t->tm_mday, t->tm_mon);
		}
		else
		{
			if (CNeutrinoApp::getInstance ()->recordingstatus && clearClock == 1)
			{
				strcpy(timestr,"  :  ");
				clearClock = 0;
			}
			else
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
	if (
	    ((mode == MODE_TVRADIO) && (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME])) ||
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
		}
		else if (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME] == 3)
		{
			display.draw_fill_rect (11,2,97,8, CLCDDisplay::PIXEL_OFF);
			//strichlin
			if (perc==255)
			{
				display.draw_line (12,3,96,7, CLCDDisplay::PIXEL_ON);
			}
			else
			{
				int dp = int( perc/100.0*86.0+12.0);
				display.draw_fill_rect (11,2,dp,8, CLCDDisplay::PIXEL_ON);
			}

            const char * icon;
            
            if( g_RemoteControl != NULL )
            {
                uint count = g_RemoteControl->current_PIDs.APIDs.size();
                if ( ( g_RemoteControl->current_PIDs.PIDs.selected_apid < count ) &&
                     ( g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].is_ac3 ) )
                    icon = DATADIR "/lcdd/icons/dd.raw";
                else
                    icon = DATADIR "/lcdd/icons/stereo.raw";

                display.paintIcon( icon, 101, 1, false );
            }
		}

		if (perform_update)
            displayUpdate();
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
		case 3:
			display.load_screen(&(background[BACKGROUND_LCD4]));
			showVolume(volume, false);
			showPercentOver(percentOver, false);
			break;
        default:
            break;
		}
		showServicename(servicename);
		showclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
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
#ifdef LCD_UPDATE
	case MODE_FILEBROWSER:
		showclock = true;
		display.draw_fill_rect(-1, -1, 120, 64, CLCDDisplay::PIXEL_OFF); // clear lcd
		showFilelist();
		break;
	case MODE_PROGRESSBAR:
		showclock = false;
		display.load_screen(&(background[BACKGROUND_SETUP]));
		showProgressBar();
		break;
	case MODE_PROGRESSBAR2:
		showclock = false;
		display.load_screen(&(background[BACKGROUND_SETUP]));
		showProgressBar2();
		break;
	case MODE_INFOBOX:
		showclock = false;
		showInfoBox();
		break;
#endif // LCD_UPDATE
	}
	wake_up();
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

void CLCD::togglePower(void)
{
	last_toggle_state_power = 1 - last_toggle_state_power;
	setlcdparameter((mode == MODE_STANDBY) ? g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS] : g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS],
			g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST],
			last_toggle_state_power,
			g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE]);
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
#ifndef HAVE_DREAMBOX_HARDWARE
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
#endif
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


#ifdef LCD_UPDATE
/*****************************************************************************************/
// showInfoBox
/*****************************************************************************************/
#define LCD_WIDTH 120
#define LCD_HEIGTH 64

#define EPG_INFO_FONT_HEIGHT 9
#define EPG_INFO_SHADOW_WIDTH 1
#define EPG_INFO_LINE_WIDTH 1
#define EPG_INFO_BORDER_WIDTH 2

#define EPG_INFO_WINDOW_POS 4
#define EPG_INFO_LINE_POS 	EPG_INFO_WINDOW_POS + EPG_INFO_SHADOW_WIDTH
#define EPG_INFO_BORDER_POS EPG_INFO_WINDOW_POS + EPG_INFO_SHADOW_WIDTH + EPG_INFO_LINE_WIDTH
#define EPG_INFO_TEXT_POS 	EPG_INFO_WINDOW_POS + EPG_INFO_SHADOW_WIDTH + EPG_INFO_LINE_WIDTH + EPG_INFO_BORDER_WIDTH

#define EPG_INFO_TEXT_WIDTH LCD_WIDTH - (2*EPG_INFO_WINDOW_POS)

// timer 0: OFF, timer>0 time to show in seconds,  timer>=999 endless
void CLCD::showInfoBox(const char * const title, const char * const text ,int autoNewline,int timer)
{
	//printf("[lcdd] Info: \n");
	if(text != NULL)
		m_infoBoxText = text;
	if(text != NULL)
		m_infoBoxTitle = title;
	if(timer != -1)
		m_infoBoxTimer = timer;
	if(autoNewline != -1)
		m_infoBoxAutoNewline = autoNewline;

	//printf("[lcdd] Info: %s,%s,%d,%d\n",m_infoBoxTitle.c_str(),m_infoBoxText.c_str(),m_infoBoxAutoNewline,m_infoBoxTimer);
	if( mode == MODE_INFOBOX &&
	    !m_infoBoxText.empty())
	{
		// paint empty box
		display.draw_fill_rect (EPG_INFO_WINDOW_POS, EPG_INFO_WINDOW_POS, 	LCD_WIDTH-EPG_INFO_WINDOW_POS+1, 	  LCD_HEIGTH-EPG_INFO_WINDOW_POS+1,    CLCDDisplay::PIXEL_OFF);
		display.draw_fill_rect (EPG_INFO_LINE_POS, 	 EPG_INFO_LINE_POS, 	LCD_WIDTH-EPG_INFO_LINE_POS-1, 	  LCD_HEIGTH-EPG_INFO_LINE_POS-1, 	 CLCDDisplay::PIXEL_ON);
		display.draw_fill_rect (EPG_INFO_BORDER_POS, EPG_INFO_BORDER_POS, 	LCD_WIDTH-EPG_INFO_BORDER_POS-3,  LCD_HEIGTH-EPG_INFO_BORDER_POS-3, CLCDDisplay::PIXEL_OFF);
	
		// paint title
		if(!m_infoBoxTitle.empty())
		{
			int width = fonts.menu->getRenderWidth(m_infoBoxTitle.c_str(),true);
			if(width > 100)
				width = 100;
			int start_pos = (120-width) /2;
			display.draw_fill_rect (start_pos, EPG_INFO_WINDOW_POS-4, 	start_pos+width+5, 	  EPG_INFO_WINDOW_POS+10,    CLCDDisplay::PIXEL_OFF);
			fonts.menu->RenderString(start_pos+4,EPG_INFO_WINDOW_POS+5, width+5, m_infoBoxTitle.c_str(), CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
		}

		// paint info 
		std::string text_line;
		int line;
		int pos = 0;
		int length = m_infoBoxText.size();
		for(line = 0; line < 5; line++)
		{
			text_line.clear();
			while ( m_infoBoxText[pos] != '\n' &&
					((fonts.menu->getRenderWidth(text_line.c_str(), true) < EPG_INFO_TEXT_WIDTH-10) || !m_infoBoxAutoNewline )&& 
					(pos < length)) // UTF-8
			{
				if ( m_infoBoxText[pos] >= ' ' && m_infoBoxText[pos] <= '~' )  // any char between ASCII(32) and ASCII (126)
					text_line += m_infoBoxText[pos];
				pos++;
			} 
			//printf("[lcdd] line %d:'%s'\r\n",line,text_line.c_str());
			fonts.menu->RenderString(EPG_INFO_TEXT_POS+1,EPG_INFO_TEXT_POS+(line*EPG_INFO_FONT_HEIGHT)+EPG_INFO_FONT_HEIGHT+3, EPG_INFO_TEXT_WIDTH, text_line.c_str(), CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
			if ( m_infoBoxText[pos] == '\n' )
				pos++; // remove new line
		}
		displayUpdate();
	}
}

/*****************************************************************************************/
//showFilelist
/*****************************************************************************************/
#define BAR_POS_X 		114
#define BAR_POS_Y 		 10
#define BAR_POS_WIDTH 	  6
#define BAR_POS_HEIGTH 	 40

void CLCD::showFilelist(int flist_pos,CFileList* flist,const char * const mainDir)
{
	//printf("[lcdd] FileList\n");
	if(flist != NULL)
		m_fileList = flist;
	if(flist_pos != -1)
		m_fileListPos = flist_pos;
	if(mainDir != NULL)
		m_fileListHeader = mainDir;
		
	if (mode == MODE_FILEBROWSER && 
	    m_fileList != NULL &&
	    m_fileList->size() > 0)
	{    
		
		printf("[lcdd] FileList:OK\n");
		int size = m_fileList->size();
		
		display.draw_fill_rect(-1, -1, 120, 52, CLCDDisplay::PIXEL_OFF); // clear lcd
		
		if(m_fileListPos > size)
			m_fileListPos = size-1;
		
		int width = fonts.menu->getRenderWidth(m_fileListHeader.c_str(), true); 
		if(width>110) 
			width=110;
		fonts.menu->RenderString((120-width)/2, 11, width+5, m_fileListHeader.c_str(), CLCDDisplay::PIXEL_ON);
		
		//printf("list%d,%d\r\n",m_fileListPos,(*m_fileList)[m_fileListPos].Marked);
		std::string text;
		int marked;
		if(m_fileListPos >  0)
		{
			if ( (*m_fileList)[m_fileListPos-1].Marked == false )
			{
				text ="";
				marked = CLCDDisplay::PIXEL_ON;
			}
			else
			{
				text ="*";
				marked = CLCDDisplay::PIXEL_INV;
			}
			text += (*m_fileList)[m_fileListPos-1].getFileName();
			fonts.menu->RenderString(1, 12+12, BAR_POS_X+5, text.c_str(), marked);
		}
		if(m_fileListPos <  size)
		{
			if ((*m_fileList)[m_fileListPos-0].Marked == false )
			{
				text ="";
				marked = CLCDDisplay::PIXEL_ON;
			}
			else
			{
				text ="*";
				marked = CLCDDisplay::PIXEL_INV;
			}
			text += (*m_fileList)[m_fileListPos-0].getFileName();
			fonts.time->RenderString(1, 12+12+14, BAR_POS_X+5, text.c_str(), marked);
		}
		if(m_fileListPos <  size-1)
		{
			if ((*m_fileList)[m_fileListPos+1].Marked == false )
			{
				text ="";
				marked = CLCDDisplay::PIXEL_ON;
			}
			else
			{
				text ="*";
				marked = CLCDDisplay::PIXEL_INV;
			}
			text += (*m_fileList)[m_fileListPos+1].getFileName();
			fonts.menu->RenderString(1, 12+12+14+12, BAR_POS_X+5, text.c_str(), marked);
		}
		// paint marker
		int pages = (((size-1)/3 )+1);
		int marker_length = (BAR_POS_HEIGTH-2) / pages;
		if(marker_length <4)
			marker_length=4;// not smaller than 4 pixel
		int marker_offset = ((BAR_POS_HEIGTH-2-marker_length) * m_fileListPos) /size  ;
		//printf("%d,%d,%d\r\n",pages,marker_length,marker_offset);
		
		display.draw_fill_rect (BAR_POS_X,   BAR_POS_Y,   BAR_POS_X+BAR_POS_WIDTH,   BAR_POS_Y+BAR_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
		display.draw_fill_rect (BAR_POS_X+1, BAR_POS_Y+1, BAR_POS_X+BAR_POS_WIDTH-1, BAR_POS_Y+BAR_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
		display.draw_fill_rect (BAR_POS_X+1, BAR_POS_Y+1+marker_offset, BAR_POS_X+BAR_POS_WIDTH-1, BAR_POS_Y+1+marker_offset+marker_length, CLCDDisplay::PIXEL_ON);
	
		displayUpdate();
	}
}

/*****************************************************************************************/
//showProgressBar
/*****************************************************************************************/
#define PROG_GLOB_POS_X 10
#define PROG_GLOB_POS_Y 30
#define PROG_GLOB_POS_WIDTH 100
#define PROG_GLOB_POS_HEIGTH 20
void CLCD::showProgressBar(int global, const char * const text,int show_escape,int timer)
{
	if(text != NULL)
		m_progressHeaderGlobal = text;
		
	if(timer != -1)
		m_infoBoxTimer = timer;
		
	if(global >= 0)
	{
		if(global > 100)
			m_progressGlobal =100;
		else
			m_progressGlobal = global;
	}
	
	if(show_escape != -1)
		m_progressShowEscape = show_escape;

	if (mode == MODE_PROGRESSBAR)
	{
		//printf("[lcdd] prog:%s,%d,%d\n",m_progressHeaderGlobal.c_str(),m_progressGlobal,m_progressShowEscape);
		// Clear Display
		display.draw_fill_rect (0,12,120,64, CLCDDisplay::PIXEL_OFF);
	
		// paint progress header 
		int width = fonts.menu->getRenderWidth(m_progressHeaderGlobal.c_str(),true);
		if(width > 100)
			width = 100;
		int start_pos = (120-width) /2;
		fonts.menu->RenderString(start_pos, 12+12, width+10, m_progressHeaderGlobal.c_str(), CLCDDisplay::PIXEL_ON,0,true);
	
		// paint global bar 
		int marker_length = (PROG_GLOB_POS_WIDTH * m_progressGlobal)/100;
		
		display.draw_fill_rect (PROG_GLOB_POS_X,   				 PROG_GLOB_POS_Y,   PROG_GLOB_POS_X+PROG_GLOB_POS_WIDTH,   PROG_GLOB_POS_Y+PROG_GLOB_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
		display.draw_fill_rect (PROG_GLOB_POS_X+1+marker_length, PROG_GLOB_POS_Y+1, PROG_GLOB_POS_X+PROG_GLOB_POS_WIDTH-1, PROG_GLOB_POS_Y+PROG_GLOB_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
	
		// paint foot 
		if(m_progressShowEscape  == true)
		{
			fonts.menu->RenderString(90, 64, 40, "Home", CLCDDisplay::PIXEL_ON);
		}
		displayUpdate();
	}
}

/*****************************************************************************************/
// showProgressBar2
/*****************************************************************************************/
#define PROG2_GLOB_POS_X 10
#define PROG2_GLOB_POS_Y 24
#define PROG2_GLOB_POS_WIDTH 100
#define PROG2_GLOB_POS_HEIGTH 10

#define PROG2_LOCAL_POS_X 10
#define PROG2_LOCAL_POS_Y 37
#define PROG2_LOCAL_POS_WIDTH PROG2_GLOB_POS_WIDTH
#define PROG2_LOCAL_POS_HEIGTH PROG2_GLOB_POS_HEIGTH

void CLCD::showProgressBar2(int local,const char * const text_local ,int global ,const char * const text_global ,int show_escape )
{
	//printf("[lcdd] prog2\n");
	if(text_local != NULL)
		m_progressHeaderLocal = text_local;
		
	if(text_global != NULL)
		m_progressHeaderGlobal = text_global;
		
	if(global >= 0)
	{
		if(global > 100)
			m_progressGlobal =100;
		else
			m_progressGlobal = global;
	}
	if(local >= 0)
	{
		if(local > 100)
			m_progressLocal =100;
		else
			m_progressLocal = local;
	}
	if(show_escape != -1)
		m_progressShowEscape = show_escape;

	if (mode == MODE_PROGRESSBAR2)
	{
	
		//printf("[lcdd] prog2:%s,%d,%d\n",m_progressHeaderGlobal.c_str(),m_progressGlobal,m_progressShowEscape);
		// Clear Display
		display.draw_fill_rect (0,12,120,64, CLCDDisplay::PIXEL_OFF);
	
		// paint  global header 
		int width = fonts.menu->getRenderWidth(m_progressHeaderGlobal.c_str(),true);
		if(width > 100)
			width = 100;
		int start_pos = (120-width) /2;
		fonts.menu->RenderString(start_pos, PROG2_GLOB_POS_Y-3, width+10, m_progressHeaderGlobal.c_str(), CLCDDisplay::PIXEL_ON,0,true);
	
		// paint global bar 
		int marker_length = (PROG2_GLOB_POS_WIDTH * m_progressGlobal)/100;
		
		display.draw_fill_rect (PROG2_GLOB_POS_X,   				PROG2_GLOB_POS_Y,   PROG2_GLOB_POS_X+PROG2_GLOB_POS_WIDTH,   PROG2_GLOB_POS_Y+PROG2_GLOB_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
		display.draw_fill_rect (PROG2_GLOB_POS_X+1+marker_length, PROG2_GLOB_POS_Y+1, PROG2_GLOB_POS_X+PROG2_GLOB_POS_WIDTH-1, PROG2_GLOB_POS_Y+PROG2_GLOB_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
	
		// paint  local header 
		width = fonts.menu->getRenderWidth(m_progressHeaderLocal.c_str(),true);
		if(width > 100)
			width = 100;
		start_pos = (120-width) /2;
		fonts.menu->RenderString(start_pos, PROG2_LOCAL_POS_Y + PROG2_LOCAL_POS_HEIGTH +10 , width+10, m_progressHeaderLocal.c_str(), CLCDDisplay::PIXEL_ON,0,true);
		// paint local bar 
		marker_length = (PROG2_LOCAL_POS_WIDTH * m_progressLocal)/100;
		
		display.draw_fill_rect (PROG2_LOCAL_POS_X,   				PROG2_LOCAL_POS_Y,   PROG2_LOCAL_POS_X+PROG2_LOCAL_POS_WIDTH,   PROG2_LOCAL_POS_Y+PROG2_LOCAL_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
		display.draw_fill_rect (PROG2_LOCAL_POS_X+1+marker_length,   PROG2_LOCAL_POS_Y+1, PROG2_LOCAL_POS_X+PROG2_LOCAL_POS_WIDTH-1, PROG2_LOCAL_POS_Y+PROG2_LOCAL_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
		// paint foot 
		if(m_progressShowEscape  == true)
		{
			fonts.menu->RenderString(90, 64, 40, "Home", CLCDDisplay::PIXEL_ON);
		}
		displayUpdate();
	}
}
/*****************************************************************************************/
#endif // LCD_UPDATE
