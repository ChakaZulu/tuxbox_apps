/*
	LCD-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://mcclean.cyberphoria.org/



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

#include "lcddisplay.h"
#include "fontrenderer.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>
#include "pthread.h"

#include "lcdd.h"


struct lcdd_msg rmsg;

CLCDDisplay		display;
fontRenderClass		*fontRenderer;
FontsDef		fonts;
pthread_t		thrTime;

bool			setup_mode;
raw_display_t		icon_lcd;
raw_display_t		icon_setup;
raw_display_t		icon_power;

char			channelname[30];
unsigned char		volume;
bool			muted;

void show_channelname(char *);
void show_volume(unsigned char);
void set_setup(unsigned char);
void set_poweroff();

void parse_command() {
	//byteorder!!!!!!
	rmsg.param2 = ntohs(rmsg.param2);

	if(rmsg.version > LCDD_VERSION)
	{
		printf("unsupported protocol version %i, this lcdd"
		    " supports only <=%i\n", rmsg.version, LCDD_VERSION);
		return;
	}

	switch (rmsg.cmd)
	{
	case LC_CHANNEL:
		channelname = rmsg.param3;
		show_channelname(channelname);
		break;
	case LC_VOLUME:
		volume = rmsg.param;
		show_volume(volume);
		break;
	case LC_MUTE:
		if (rmsg.param == LC_MUTE_ON)
			muted = true;
		else
			muted = false;
		show_volume(volume);
		break;
	case LC_SET_SETUP:
		set_setup(rmsg.param);
		break;
	case LC_POWEROFF:
		set_poweroff();
		break;
	default: 
		printf("unknown command %i\n", rmsg.cmd);
	}
}

void show_channelname( char * name)
{
	if (setup_mode) return;
	display.draw_fill_rect (0,26,120,50, CLCDDisplay::PIXEL_OFF);
	fonts.channelname->RenderString(1,43, 130, name, CLCDDisplay::PIXEL_ON);
	display.update();
}

void show_time()
{
	char timestr[50];
	struct timeb tm;
	if (setup_mode) return;
	ftime(&tm);
	strftime((char*) &timestr, 20, "%H:%M", localtime(&tm.time) );

	display.draw_fill_rect (90,54,120,64, CLCDDisplay::PIXEL_OFF);
	fonts.time->RenderString(92,62, 50, timestr, CLCDDisplay::PIXEL_ON);
	display.update();
}

void show_volume(unsigned char vol)
{
	if (setup_mode) return;
	display.draw_fill_rect (3,54,29,64, CLCDDisplay::PIXEL_OFF);
	display.draw_fill_rect (3,54,3+(vol>>2),64, CLCDDisplay::PIXEL_ON);
	if (muted) {
		display.draw_fill_rect (4,55,2+(vol>>2),63, CLCDDisplay::PIXEL_OFF);
	}
	display.update();
}

void set_setup(unsigned char mode) {
	//int y, t;
	//raw_display_t s;
	if (mode == LC_SET_SETUP_OFF) {
		display.load_screen(&icon_lcd);
		show_volume(volume);
		show_channelname(channelname);
		show_time();
		setup_mode = false;
	} else {
		setup_mode = true;
		/*display.dump_screen(&s);
		for (t=0; t<23; t++) {
			for (y=0; y<27-t; y++) {
				memcpy(s[y], icon_lcd[y+t], LCD_COLS);
			}
			memset(s[27-t], 0, LCD_COLS);
			display.load_screen(&s);
			display.update();
			usleep(10*1000);
		}*/
		display.load_screen(&icon_setup);
		display.update();
	}
} 

void set_poweroff() {
	// verhindern, dass irgendwelche ausgaben stattfinden
	setup_mode = true;
	display.load_screen(&icon_power);
	display.update();
}

void * TimeThread (void *)
{
	while(1)
	{
		sleep(10);
		show_time();
	}
	return NULL;
}

int main(int argc, char **argv)
{
	printf("Network LCD-Driver 0.1\n\n");

	if (fork() != 0) return 0;

	fontRenderer = new fontRenderClass( &display );
	fonts.channelname=fontRenderer->getFont("Arial", "Regular", 12);
	fonts.time=fontRenderer->getFont("Arial", "Regular", 8);
	display.setIconBasePath("/usr/lib/icons/");

	if(!display.isAvailable())
	{
		printf("exit...(no lcd-support)\n");
		exit(-1);
	}

	if (!display.paintIcon("neutrino_setup.raw",0,0,false))
	{
		printf("exit...(no neutrino_setup.raw)\n");
		exit(-1);
	}
	display.dump_screen(&icon_setup);

	if (!display.paintIcon("neutrino_power.raw",0,0,false))
	{
		printf("exit...(no neutrino_power.raw)\n");
		exit(-1);
	}
	display.dump_screen(&icon_power);

	if (!display.paintIcon("neutrino_lcd.raw",0,0,false))
	{
		printf("exit...(no neutrino_lcd.raw)\n");
		exit(-1);
	}
	display.dump_screen(&icon_lcd);
	setup_mode = false;

	show_channelname("");
	show_time();

	if (pthread_create (&thrTime, NULL, TimeThread, NULL) != 0 )
	{
		perror("timer thread create failed\n");
	}


	int listenfd, connfd;
	socklen_t clilen;
	SAI cliaddr, servaddr;

	//network-setup
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(LCDD_PORT);

	if ( bind(listenfd, (SA *) &servaddr, sizeof(servaddr)) !=0)
	{
		perror("bind failed...\n");
		exit(-1);
	}


	if (listen(listenfd, 5) !=0)
	{
		perror("listen failed...\n");
		exit( -1 );
	}

	printf("\n");
	while(1)
	{
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (SA *) &cliaddr, &clilen);

		memset(&rmsg, 0, sizeof(rmsg));
		read(connfd,&rmsg,sizeof(rmsg));
		parse_command();
		close(connfd);
	}

}

