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

#define SA struct sockaddr
#define SAI struct sockaddr_in


struct rmsg {
  unsigned char version;
  unsigned char cmd;
  unsigned short param;
  unsigned short param2;
  char param3[30];

} rmsg;

CLCDDisplay			display;
fontRenderClass		*fontRenderer;
FontsDef			fonts;
pthread_t			thrTime;

void show_channelname(char *);
void parse_command()
{
  //byteorder!!!!!!
  rmsg.param = ((rmsg.param & 0x00ff) << 8) | ((rmsg.param & 0xff00) >> 8);
  rmsg.param2 = ((rmsg.param2 & 0x00ff) << 8) | ((rmsg.param2 & 0xff00) >> 8);

  if(rmsg.version!=1)
  {
    perror("unknown version\n");
    return;
  }

  switch (rmsg.cmd)
  {
    case 1:
      show_channelname( rmsg.param3 );
      break;
    default:  
	    printf("unknown command\n");
  }
}


void show_channelname( char * name)
{
	display.draw_fill_rect (0,26,120,50, CLCDDisplay::PIXEL_OFF);
	fonts.channelname->RenderString(1,43, 130, name, CLCDDisplay::PIXEL_ON);
	display.update();
}

void show_time()
{
	char timestr[50];
	struct timeb tm;
	ftime(&tm);
	strftime((char*) &timestr, 20, "%H:%M", localtime(&tm.time) );

	display.draw_fill_rect (90,54,120,64, CLCDDisplay::PIXEL_OFF);
	fonts.time->RenderString(92,62, 50, timestr, CLCDDisplay::PIXEL_ON);
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

	if (!display.paintIcon("neutrino_lcd.raw",0,0,0))
	{
		printf("exit...(no icon)\n");
		exit(-1);
	}

	show_time();

	show_channelname("");

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
  servaddr.sin_port = htons(1510);

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

