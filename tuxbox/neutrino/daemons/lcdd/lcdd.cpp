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

#include "lcdd.h"

#include <config.h>

#include "bigclock.h"

/* Signal quality */
#include <ost/frontend.h>
#include <dbox/fp.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>
#include <signal.h>


CLCDD::CLCDD()
	: configfile('\t')
{
}

CLCDD::~CLCDD()
{
}

CLCDD* CLCDD::getInstance()
{
	static CLCDD* lcdd = NULL;
	if(lcdd == NULL)
	{
		lcdd = new CLCDD();
	}
	return lcdd;
}

void CLCDD::parse_command(int connfd, CLcddClient::commandHead rmsg)
{
	if(rmsg.version != CLcddClient::ACTVERSION)
	{
		printf("[lcdd] unknown version\n");
		return;
	}

	switch (rmsg.cmd)
	{
		case CLcddClient::CMD_SETSERVICENAME:
			CLcddClient::commandServiceName msg;
			read(connfd, &msg, sizeof(msg));
			strcpy(servicename, msg.servicename);
			show_servicename(servicename);
			break;
		case CLcddClient::CMD_SETVOLUME:
			CLcddClient::commandVolume msg2;
			read(connfd, &msg2, sizeof(msg2));
			volume = msg2.volume;
			show_volume(volume);
			break;
		case CLcddClient::CMD_SETMUTE:
			CLcddClient::commandMute msg3;
			read(connfd, &msg3, sizeof(msg3));
			muted = msg3.mute;
			show_volume(volume);
			break;
		case CLcddClient::CMD_SETMODE:
			CLcddClient::commandMode msg4;
			read(connfd, &msg4, sizeof(msg4));
			set_mode((CLcddClient::mode) msg4.mode, msg4.text);
			break;
		case CLcddClient::CMD_SETMENUTEXT:
			CLcddClient::commandMenuText msg5;
			read(connfd, &msg5, sizeof(msg5));
			show_menu(msg5.position, msg5.text, msg5.highlight);
			break;
		default:
			printf("unknown command %i\n", rmsg.cmd);
	}
}

void CLCDD::show_servicename( string name )
{

	if (mode!=CLcddClient::MODE_TVRADIO)
	{
		return;
	}
	display.draw_fill_rect (0,14,120,48, CLCDDisplay::PIXEL_OFF);

	if (fonts.channelname->getRenderWidth(name.c_str())>120)
	{
		int pos;
		string text1 = name;
    	do
    	{
			pos = text1.find_last_of("[ .]+");
			if ( pos!=-1 )
				text1 = text1.substr( 0, pos );
		} while ( ( pos != -1 ) && ( fonts.channelname->getRenderWidth(text1.c_str())> 120 ) );

		if ( fonts.channelname->getRenderWidth(text1.c_str())<= 120 )
			fonts.channelname->RenderString(1,29+16, 130, name.substr(text1.length()+ 1).c_str(), CLCDDisplay::PIXEL_ON);
		else
		{
			string text1 = name;
			while (fonts.channelname->getRenderWidth(text1.c_str())> 120)
				text1= text1.substr(0, text1.length()- 1);

			fonts.channelname->RenderString(1,29+16, 130, name.substr(text1.length()).c_str(), CLCDDisplay::PIXEL_ON);
		}

		fonts.channelname->RenderString(1,29, 130, text1.c_str(), CLCDDisplay::PIXEL_ON);
	}
	else
	{
		fonts.channelname->RenderString(1,37, 130, name.c_str(), CLCDDisplay::PIXEL_ON);
	}
	display.update();
}

void CLCDD::show_time()
{
	char timestr[50];
	struct timeb tm;
	//printf("[lcdd] clock event\n");
	if (showclock)
	{
		ftime(&tm);
		strftime((char*) &timestr, 20, "%H:%M", localtime(&tm.time) );

		if(mode!=CLcddClient::MODE_STANDBY)
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
			showBigClock(&display, t->tm_hour,t->tm_min);
			/*
			fonts.menutitle->RenderString(60,62, 60, timestr, CLCDDisplay::PIXEL_ON);
			*/
		}
		display.update();
	}
}

void CLCDD::show_signal()
{
	int fd, status, signal, res;

	if((fd = open("/dev/ost/qpskfe0", O_RDONLY)) < 0)
		return;
	if (ioctl(fd,FE_READ_STATUS,&status)<0)
		return;

	res=ioctl(fd,FE_READ_SIGNAL_STRENGTH, &signal);
	if (res<0) signal=0;

	printf("%i\n", signal);
	close(fd);
}

void CLCDD::show_volume(char vol)
{
	if ((mode==CLcddClient::MODE_TVRADIO) || (mode==CLcddClient::MODE_SCART))
	{
		display.draw_fill_rect (1,52,73,61, CLCDDisplay::PIXEL_OFF);
		//strichlin
		if (muted)
		{
			display.draw_line (1,52,73,61, CLCDDisplay::PIXEL_ON);
		}
		else
		{
			int dp = int( vol/100.0*72.0);
			display.draw_fill_rect (0,52,dp,61, CLCDDisplay::PIXEL_ON);
		}

		display.update();
	}
}

void CLCDD::show_menu(int position, char* text, int highlight )
{
	if (mode != CLcddClient::MODE_MENU)
	{
		return;
	}
	// reload specified line
	display.draw_fill_rect(-1,35+14*position,120,35+14+14*position, CLCDDisplay::PIXEL_OFF);
	fonts.menu->RenderString(0,35+11+14*position, 140, text , CLCDDisplay::PIXEL_INV, highlight);
	display.update();
}

void CLCDD::dimmlcd(int val)
{
	int fd;

	if ((fd = open("/dev/dbox/fp0",O_RDWR)) <= 0)
	{
		perror("open");
	}

	if (ioctl(fd,FP_IOCTL_LCD_DIMM, &val) < 0)
	{
		perror("dimm");
	}
	close(fd);
}


void CLCDD::set_mode(CLcddClient::mode m, char *title)
{
	switch (m)
	{
		case CLcddClient::MODE_TVRADIO:
			dimmlcd(lcd_brightness);
			//printf("[lcdd] mode: tvradio\n");
			display.load_screen(&icon_lcd);
			mode = m;
			showclock = true;
			show_volume(volume);
			show_servicename(servicename);
			show_time();
			display.update();
			break;
		case CLcddClient::MODE_SCART:
			dimmlcd(lcd_brightness);
			//printf("[lcdd] mode: scart\n");
			display.load_screen(&icon_lcd);
			mode = m;
			showclock = true;
			show_volume(volume);
			show_time();
			display.update();
			break;
		case CLcddClient::MODE_MENU:
			dimmlcd(lcd_brightness);
			//printf("[lcdd] mode: menu\n");
			mode = m;
			showclock = false;
			display.load_screen(&icon_setup);
			fonts.menutitle->RenderString(-1,28, 140, title,
				CLCDDisplay::PIXEL_ON);
			display.update();
			break;
		case CLcddClient::MODE_SHUTDOWN:
			dimmlcd(lcd_brightness);
			//printf("[lcdd] mode: shutdown\n");
			mode = m;
			showclock = false;
			display.load_screen(&icon_power);
			display.update();
			shall_exit = true;
			break;
		case CLcddClient::MODE_STANDBY:
			//printf("[lcdd] mode: standby\n");
			dimmlcd(lcd_standbybrightness);
			mode = m;
			showclock = true;
			display.draw_fill_rect (-1,0,120,64, CLCDDisplay::PIXEL_OFF);
			show_time();
			display.update();
			break;

		default:
			printf("[lcdd] Unknown mode: %i\n", m);
			return;
	}
}

void CLCDD::saveConfig()
{
	static bool inSave=false;
	if(inSave==false)
	{
		inSave=true;
		configfile.setInt( "lcd_brightness", lcd_brightness );
		configfile.setInt( "lcd_standbybrightness", lcd_standbybrightness );

		if(configfile.getModifiedFlag())
		{
			printf("[lcdd] save config\n");
			configfile.saveConfig(CONFIGDIR "/lcdd.conf");
		}
		inSave=false;
	}
}

void CLCDD::loadConfig()
{
	printf("[lcdd] load config\n");
	if(!configfile.loadConfig(CONFIGDIR "/lcdd.conf"))
	{
		lcd_brightness = 0xff;
		lcd_standbybrightness = 0xaa;
		return;
	}

	lcd_brightness = configfile.getInt("lcd_brightness", 0xff);
	lcd_standbybrightness = configfile.getInt("lcd_standbybrightness", 0xaa);
}

void* CLCDD::TimeThread(void *)
{
	while(1)
	{
		sleep(10);
		CLCDD::getInstance()->show_time();
	}
	return NULL;
}

void CLCDD::sig_catch(int)
{
	 CLCDD::getInstance()->saveConfig();
	 exit(0);
}

int CLCDD::main(int argc, char **argv)
{
	debugoutput = true;
	printf("Network LCD-Driver $Id: lcdd.cpp,v 1.47 2002/05/14 23:58:15 McClean Exp $\n\n");

	fontRenderer = new fontRenderClass( &display );
	fontRenderer->AddFont(FONTDIR "/micron.ttf");
	fontRenderer->InitFontCache();

	#define FONTNAME "Micron"
	fonts.channelname=fontRenderer->getFont(FONTNAME, "Regular", 15);
	fonts.time=fontRenderer->getFont(FONTNAME, "Regular", 14);
	fonts.menutitle=fontRenderer->getFont(FONTNAME, "Regular", 15);
	fonts.menu=fontRenderer->getFont(FONTNAME, "Regular", 12);

	loadConfig();
	dimmlcd(lcd_brightness);
	display.setIconBasePath( DATADIR "/lcdd/icons/");

	if(!display.isAvailable())
	{
		printf("exit...(no lcd-support)\n");
		return -1;
	}

	if (!display.paintIcon("neutrino_setup.raw",0,0,false))
	{
		printf("exit...(no neutrino_setup.raw)\n");
		return -1;	}
	display.dump_screen(&icon_setup);

	if (!display.paintIcon("neutrino_power.raw",0,0,false))
	{
		printf("exit...(no neutrino_power.raw)\n");
		return -1;	}
	display.dump_screen(&icon_power);

	if (!display.paintIcon("neutrino_lcd.raw",0,0,false))
	{
		printf("exit...(no neutrino_lcd.raw)\n");
		return -1;	}
	display.dump_screen(&icon_lcd);

	mode = CLcddClient::MODE_TVRADIO;
	show_servicename("Booting...");
	showclock=true;
	//show_time();


	//network-setup
	int listenfd, connfd;
	struct sockaddr_un servaddr;
	int clilen;

	std::string filename = LCDD_UDS_NAME;
	filename += ".";
	filename += CLcddClient::getSystemId();

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, filename.c_str());
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	unlink(filename.c_str());

	//network-setup
	if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return -1;
	}

	if ( bind(listenfd, (struct sockaddr*) &servaddr, clilen) <0 )
	{
		perror("[lcdd] bind failed...\n");
		return -1;
	}

	if (listen(listenfd, 5) !=0)
	{
		perror("[lcdd] listen failed...\n");
		return -1;
	}

	switch (fork())
	{
		case -1: /* can't fork */
			perror("[lcdd] fork");
			return -1;

		case 0: /* child, process becomes a daemon */
			if (setsid() == -1)
			{
				perror("[lcdd] setsid");
				return -1;
			}
			break;

		default: /* parent returns to calling process */
			return 0;
	}

	signal(SIGHUP,sig_catch);
	signal(SIGINT,sig_catch);
	signal(SIGQUIT,sig_catch);
	signal(SIGTERM, sig_catch);

	/* Thread erst nach dem forken erstellen, da sonst Abbruch */
	if (pthread_create (&thrTime, NULL, TimeThread, NULL) != 0 )
	{
		perror("[lcdd]: pthread_create(TimeThread)");
		return -1;
	}

	shall_exit = false;
	while(!shall_exit)
	{
		connfd = accept(listenfd, (struct sockaddr*) &servaddr, (socklen_t*) &clilen);
		CLcddClient::commandHead rmsg;
		read(connfd,&rmsg,sizeof(rmsg));
		parse_command(connfd, rmsg);
		close(connfd);
	}
	close(listenfd);
	saveConfig();
	return 0;
}


int main(int argc, char **argv)
{
	return CLCDD::getInstance()->main(argc, argv);
}

