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

/* Signal quality */
#include <ost/frontend.h>

#include "lcdd.h"



CLCDDisplay		display;
fontRenderClass		*fontRenderer;
FontsDef		fonts;
pthread_t		thrTime;

lcdd_mode		mode;
raw_display_t		icon_lcd;
raw_display_t		icon_setup;
raw_display_t		icon_power;

char			channelname[30];
unsigned char		volume;
bool			muted, shall_exit;

void show_channelname(char *);
void show_volume(unsigned char);
void show_menu(lcdd_msg msg);
void set_mode(lcdd_mode, char *title);
void set_poweroff();

void parse_command(lcdd_msg rmsg) {
	//byteorder!!!!!!
	rmsg.param2 = ntohs(rmsg.param2);

	if(rmsg.version > LCDD_VERSION)
	{
		printf("unsupported protocol version %i, this lcdd"
		    " supports only <=%i\n", rmsg.version, LCDD_VERSION);
		return;
	}

	//printf("[LCDD] received cmd=%i param=%i\n", rmsg.cmd, rmsg.param);
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
	case LC_SET_MODE:
		set_mode((lcdd_mode)rmsg.param, rmsg.param3);
		break;
	case LC_MENU_MSG:
		show_menu(rmsg);
		break;
	case LC_POWEROFF:
		set_mode(LCDM_POWEROFF, NULL);
		shall_exit = true;
		break;
	default: 
		printf("unknown command %i\n", rmsg.cmd);
	}
}

void show_channelname( char * name)
{
	if (mode!=LCDM_TV) return;
	display.draw_fill_rect (0,26,120,50, CLCDDisplay::PIXEL_OFF);
	fonts.channelname->RenderString(1,43, 130, name, CLCDDisplay::PIXEL_ON);
	display.update();
}

void show_time()
{
	char timestr[50];
	struct timeb tm;
	if (mode!=LCDM_TV) return;
	ftime(&tm);
	strftime((char*) &timestr, 20, "%H:%M", localtime(&tm.time) );

	display.draw_fill_rect (90,54,120,64, CLCDDisplay::PIXEL_OFF);
	fonts.time->RenderString(92,63, 50, timestr, CLCDDisplay::PIXEL_ON);
	display.update();
}


void show_signal() {
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


void show_volume(unsigned char vol)
{
	if (mode!=LCDM_TV) return;
	display.draw_fill_rect (3,54,29,64, CLCDDisplay::PIXEL_OFF);
	display.draw_fill_rect (3,54,3+(vol>>2),64, CLCDDisplay::PIXEL_ON);
	if (muted) {
		display.draw_fill_rect (4,55,2+(vol>>2),63, CLCDDisplay::PIXEL_OFF);
	}
	display.update();
}

void show_menu(lcdd_msg msg) {
	int i;

	if (mode != LCDM_MENU) return;
	/* WARNING: interface change; if something doesn't work, read lcdd.h */
	// reload specified line
	i = msg.param;
	display.draw_fill_rect(-1,21+14*i,120,36+14*i, CLCDDisplay::PIXEL_OFF);
	fonts.menu->RenderString(0,33+14*i, 140, msg.param3,
	    CLCDDisplay::PIXEL_ON);
	display.update();
}


void set_mode(lcdd_mode m, char *title) {
	//int y, t;
	//raw_display_t s;
	switch (m) {
	case LCDM_TV:
		display.load_screen(&icon_lcd);
		mode = m;
		show_volume(volume);
		show_channelname(channelname);
		show_time();
		display.update();
		break;
	case LCDM_MENU:
		mode = m;
		display.load_screen(&icon_setup);
		fonts.menutitle->RenderString(-1,17, 140, title,
		    CLCDDisplay::PIXEL_ON);
		display.update();
		break;
	case LCDM_POWEROFF:
		mode = m;
		display.load_screen(&icon_power);
		display.update();
		break;
	default:
		printf("[lcdd] Unknown mode: %i\n", m);
		return;
	}
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

	fontRenderer = new fontRenderClass( &display );
	fonts.channelname=fontRenderer->getFont("Arial", "Regular", 12);
	fonts.time=fontRenderer->getFont("Arial", "Regular", 10);
	fonts.menutitle=fontRenderer->getFont("Arial", "Regular", 15);
	fonts.menu=fontRenderer->getFont("Arial", "Regular", 13);
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
	mode = LCDM_TV;
	//set_mode(LCDM_TV);

	show_channelname("");
	show_time();

	if (pthread_create (&thrTime, NULL, TimeThread, NULL) != 0 )
	{
		perror("timer thread create failed\n");
	}


	int listenfd, connfd;
	socklen_t clilen;
	SAI cliaddr, servaddr;
	struct lcdd_msg rmsg;

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

	/* alles geladen, daemonize Now! ;) */
	if (fork() != 0) return 0;

	shall_exit = false;
	while(!shall_exit)
	{
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (SA *) &cliaddr, &clilen);

		memset(&rmsg, 0, sizeof(rmsg));
		read(connfd,&rmsg,sizeof(rmsg));
		parse_command(rmsg);
		close(connfd);
	}
	close(listenfd);
}

