/*
	Control-Daemon  -   DBoxII-Project

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
 
#define SA struct sockaddr
#define SAI struct sockaddr_in



struct rmsg {
  unsigned char version;
  unsigned char cmd;
  unsigned short param;
  unsigned short param2;
  char param3[30];
 
} rmsg;

void shutdownBox()
{
    if (execlp("/sbin/halt", "/sbin/halt", 0)<0)
    {
      perror("exec failed - halt\n");
    }
}


void setVideoRGB()
{
	if(fork()==0)
	{
		if (execlp("/bin/switch", "/bin/switch", "-fnc", "2", "-fblk", "1", 0)<0)
		{
			perror("exec failed - /bin/switch\n");
		}
	}
}

char gen_Volume;
void setVolume(char volume)
{
	gen_Volume = volume;
	char val[10];
	if(fork()==0)
	{
		char vol = 64-char(volume*64.0/100.0);
		printf("volume set: %d-----------------------\n", vol);
		sprintf((char*) val, "%d", vol);
		if (execlp("/bin/switch", "/bin/switch", "-v", val, 0)<0)
		{
			perror("exec failed - /bin/switch\n");
		}
	}

	struct
	{
	  unsigned char version;
	  unsigned char cmd;
	  unsigned char param;
	  unsigned short param2;
	  char param3[30];
	} rmsg;

	int sock_fd;
	SAI servaddr;
	
	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(1510);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))!=-1)
	{
		rmsg.version=1;
		rmsg.cmd=2;
		rmsg.param = volume;
		write(sock_fd,&rmsg,sizeof(rmsg));
		close(sock_fd);
	}
}

void Mute()
{
	char tmp = gen_Volume;
	setVolume(0);
	gen_Volume = tmp;
}

void UnMute()
{
	setVolume(gen_Volume);
}


void parse_command()
{
  //byteorder!!!!!!
  rmsg.param = ((rmsg.param & 0x00ff) << 8) | ((rmsg.param & 0xff00) >> 8);
  rmsg.param2 = ((rmsg.param2 & 0x00ff) << 8) | ((rmsg.param2 & 0xff00) >> 8);
 
 
  printf("Command received\n");
  printf("  Version: %d\n", rmsg.version);
  printf("  Command: %d\n", rmsg.cmd);
  printf("  Param: %d\n", rmsg.param);
  printf("  Param2: %d\n", rmsg.param2);
  printf("  Param3: %s\n", rmsg.param3);
 
 
  if(rmsg.version!=1)
  {
    perror("unknown version\n");
    return;
  }
 
  switch (rmsg.cmd)
  {
    case 1:
      printf("shutdown\n");
      shutdownBox();
      break;
    case 2:
      printf("set volume\n");
      setVolume(rmsg.param);
      break;
    case 3:
      printf("mute\n");
      Mute();
      break;
    case 4:
      printf("unmute\n");
      UnMute();
      break;
    default:
    printf("unknown command\n");
  }
 
}





int main(int argc, char **argv)
{
  int listenfd, connfd;
  socklen_t clilen;
  SAI cliaddr, servaddr;
 
  printf("Controld  0.1\n\n");
 
  if (fork() != 0) return 0;
 
  //network-setup
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
 
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(1610);
 
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


	//init 
	gen_Volume = 100;
	setVolume(gen_Volume);

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
