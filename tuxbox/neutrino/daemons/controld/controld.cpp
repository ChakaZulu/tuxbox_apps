/*
	Control-Daemon  -   DBoxII-Project

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "dbox/avs_core.h"
#include "ost/video.h"

#include "../lcdd/lcdd.h"

#include "eventwatchdog.h"
#include "controldclient.h"

#define CONF_FILE CONFIGDIR "/controld.conf"


struct Ssettings
{
	char volume;
	bool mute;
	char videotype;
	char videoformat;

	char boxtype;
	char lastmode;
}settings;


void sig_catch(int);

class CControldAspectRatioNotifier : public CAspectRatioNotifier
{
	public:
		virtual void aspectRatioChanged( int newAspectRatio); //override;
};

bool bNotifyRegistered = false;
CEventWatchDog* watchDog;
CControldAspectRatioNotifier* aspectRatioNotifier;

int loadSettings()
{
	int fd;
	fd = open(CONF_FILE, O_RDONLY );

	if (fd==-1)
	{
		printf("[controld] error while loading settings: %s\n", CONF_FILE );
		return 0;
	}
	if(read(fd, &settings, sizeof(settings))!=sizeof(settings))
	{
		printf("[controld] error while loading settings: %s - config from old version?\n", CONF_FILE );
		return 0;
	}
	close(fd);
	return 1;
}

void saveSettings()
{
	int fd;
	fd = open(CONF_FILE, O_WRONLY | O_CREAT );

	if (fd==-1)
	{
		printf("[controld] error while saving settings: %s\n", CONF_FILE );
		return;
	}
	write(fd, &settings,  sizeof(settings) );
	close(fd);
}


void sendto_lcdd(unsigned char cmd, unsigned char param)
{
	int sock_fd;
	SAI servaddr;
	struct lcdd_msg lmsg;

	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(LCDD_PORT);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))!=-1)
	{
		lmsg.version=LCDD_VERSION;
		lmsg.cmd=cmd;
		lmsg.param = param;
		write(sock_fd,&lmsg,sizeof(lmsg));
		close(sock_fd);
	}
}

void shutdownBox()
{
    sendto_lcdd(LC_POWEROFF, 0);
	sig_catch(1);
    if (execlp("/sbin/halt", "/sbin/halt", 0)<0)
    {
      perror("exec failed - halt\n");
    }
}

void setVideoType(int format)
{
	int fd;
	/*
		RGB : fblk 1
	*/
	if (format < 0)
	{
		format=0;
	}
	if (format > 3)
	{
		format=3;
	}

	settings.videotype = format;

	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd,AVSIOSFBLK,&format)< 0)
	{
		perror("AVSIOSFBLK:");
		return;
	}
	close(fd);
}

void setVideoFormat(int format, bool bUnregNotifier = true)
{
	int fd;
	int videoDisplayFormat;

	/*
		16:9 : fnc 1
		4:3  : fnc 2
	*/
	if (format < 0)
	{
		format=0;
	}
	if (format > 3)
	{
		format=3;
	}
	settings.videoformat = format;

	if (format==0) // automatic switch
	{
		if (!bNotifyRegistered)
		{
			printf("[controld] setting VideoFormat to auto \n");
			watchDog->registerNotifier(WDE_VIDEOMODE, aspectRatioNotifier);
			bNotifyRegistered = true;
		}
	}
	else
	{
		if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
		{
			perror("open");
			return;
		}

		if (ioctl(fd,AVSIOSFNC,&format)< 0)
		{
			perror("AVSIOSFNC:");
			return;
		}
		close(fd);

		switch( format)
		{
	//	?	case AVS_FNCOUT_INTTV	: videoDisplayFormat = VIDEO_PAN_SCAN;
			case AVS_FNCOUT_EXT169	: videoDisplayFormat = VIDEO_CENTER_CUT_OUT; break;
			case AVS_FNCOUT_EXT43	: videoDisplayFormat = VIDEO_LETTER_BOX; break;
			default: videoDisplayFormat = VIDEO_LETTER_BOX;
	//	?	case AVS_FNCOUT_EXT43_1	: videoDisplayFormat = VIDEO_PAN_SCAN;
		}

		if ((fd = open("/dev/ost/video0",O_RDWR)) <= 0)
		{
			perror("open");
			return;
		}

		if ( ioctl(fd, VIDEO_SET_DISPLAY_FORMAT, videoDisplayFormat))
		{
			perror("VIDEO SET DISPLAY FORMAT:");
			return;
		}
		close(fd);

		watchDog->lastVideoMode = format;
		if ((bNotifyRegistered) && (bUnregNotifier))
		{
			watchDog->unregisterNotifier(WDE_VIDEOMODE, aspectRatioNotifier);
			bNotifyRegistered = false;
		}

	}
}

void routeVideo(int a, int b, int nothing, int fblk)
{
	int fd;

	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd, AVSIOSFBLK, &fblk)< 0)
	{
		perror("AVSIOSFBLK:");
		return;
	}

	if (ioctl(fd,AVSIOSVSW1,&a)< 0)
	{
		perror("AVSIOSVSW1:");
		return;
	}

	if (ioctl(fd,AVSIOSASW1,&b)< 0)
	{
		perror("AVSIOSASW1:");
		return;
	}

	if (ioctl(fd,AVSIOSVSW2,&nothing)< 0)
	{
		perror("AVSIOSVSW2:");
		return;
	}

	close(fd);
}

char BoxNames[4][10] = {"","Nokia", "Sagem", "Philips"};
void switch_vcr( bool vcr_on)
{
	if (vcr_on)
	{
		//turn to scart-input
		printf("switch to scart-input... (%s)\n", BoxNames[settings.boxtype]);
		if (settings.boxtype == 2) // Sagem
		{
			routeVideo(2, 1, 7, 2);
		}
		else if (settings.boxtype == 1) // Nokia
		{
			routeVideo(3, 2, 7, 2);
		}
		else if (settings.boxtype == 3) // Philips
		{
			routeVideo(2, 2, 3, 2);
		}
	}
	else
	{	//turn to dvb...
		printf("switch to dvb-input...\n");
		if (settings.boxtype == 2) // Sagem
		{
			routeVideo(0, 0, 0, settings.videotype);
		}
		else if (settings.boxtype == 1) // Nokia
		{
			routeVideo(5, 1, 7, settings.videotype);
		}
		else if (settings.boxtype == 3) // Philips
		{
			routeVideo(1, 1, 1, settings.videotype);
		}
	}
}

void setScartMode(char onoff)
{

	switch_vcr( onoff );
}

void setBoxType(char type)
{
	settings.boxtype = type;
}

void setVolume(char volume)
{
	int fd;
	settings.volume = volume;

	int i = 64-int(volume*64.0/100.0);
	printf("[controld] set volume: %d\n", i );
	if (i < 0)
	{
		i=0;
	}
	else if (i > 63)
	{
		i=63;
	}

	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd,AVSIOSVOL,&i)< 0)
	{
		perror("AVSIOGVOL:");
		return;
	}
	close(fd);

	sendto_lcdd(LC_VOLUME, volume);
}

void Mute()
{
	settings.mute = 1;
	int i;
	int fd;
	i=AVS_MUTE;

	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd,AVSIOSMUTE,&i)< 0)
	{
		perror("AVSIOSMUTE:");
		return;
	}
	close(fd);

	sendto_lcdd(LC_MUTE, LC_MUTE_ON);
}

void UnMute()
{
	settings.mute = 0;
	int i;
	int fd;
	i=AVS_UNMUTE;

	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd,AVSIOSMUTE,&i)< 0)
	{
		perror("AVSIOSMUTE:");
		return;
	}
	close(fd);

	sendto_lcdd(LC_MUTE, LC_MUTE_OFF);
}


void parse_command(int connfd, CControldClient::commandHead* rmessage)
{

  if(rmessage->version!=CControldClient::ACTVERSION)
  {
    perror("[controld] unknown version\n");
    return;
  }
  switch (rmessage->cmd)
  {
    case CControldClient::CMD_SHUTDOWN:
      printf("[controld] shutdown\n");
      shutdownBox();
      break;
    case CControldClient::CMD_SETVOLUME:
      printf("[controld] set volume\n");
      CControldClient::commandVolume msg;
	  read(connfd, &msg, sizeof(msg));
      setVolume(msg.volume);
      break;
	case CControldClient::CMD_MUTE:
      printf("[controld] mute\n");
      Mute();
      break;
    case CControldClient::CMD_UNMUTE:
      printf("[controld] unmute\n");
      UnMute();
      break;
    case CControldClient::CMD_SETVIDEOFORMAT:
      printf("[controld] set videoformat\n");
      CControldClient::commandVideoFormat msg2;
	  read(connfd, &msg2, sizeof(msg2));
	  setVideoFormat(msg2.format);
      break;
    case CControldClient::CMD_SETVIDEOOUTPUT:
      printf("[controld] set videooutput\n");
      CControldClient::commandVideoOutput msg3;
	  read(connfd, &msg3, sizeof(msg3));
	  setVideoType(msg3.output);
      break;
	  
    case CControldClient::CMD_SETBOXTYPE:
      printf("[controld] set boxtype\n");
      CControldClient::commandBoxType msg4;
	  read(connfd, &msg4, sizeof(msg4));
	  setBoxType(msg4.boxtype);
      break;
    case CControldClient::CMD_SETSCARTMODE:
      printf("[controld] set scartmode\n");
      CControldClient::commandScartMode msg5;
	  read(connfd, &msg5, sizeof(msg5));
      setScartMode(msg5.mode);
      break;

	case CControldClient::CMD_GETVOLUME:
		printf("[controld] get volume\n");
		CControldClient::responseVolume msg6;
		msg6.volume = settings.volume;
		write(connfd,&msg6,sizeof(msg6));
		break;
	case CControldClient::CMD_GETMUTESTATUS:
		printf("[controld] get mute\n");
		CControldClient::responseMute msg7;
		msg7.mute = settings.mute;
		write(connfd,&msg7,sizeof(msg7));
		break;
	case CControldClient::CMD_GETVIDEOFORMAT:
		printf("[controld] get videoformat (fnc)\n");
		CControldClient::responseVideoFormat msg8;
		msg8.format = settings.videoformat;
		write(connfd,&msg8,sizeof(msg8));
		break;
	case CControldClient::CMD_GETVIDEOOUTPUT:
		printf("[controld] get videooutput (fblk)\n");
		CControldClient::responseVideoOutput msg9;
		msg9.output = settings.videotype;
		write(connfd,&msg9,sizeof(msg9));
		break;
	case CControldClient::CMD_GETBOXTYPE:
		printf("[controld] get boxtype\n");
		CControldClient::responseBoxType msg0;
		msg0.boxtype = settings.boxtype;
		write(connfd,&msg0,sizeof(msg0));
		break;
    default:
		printf("[controld] unknown command\n");
  }
}


void sig_catch(int)
{
	printf("[controld] shutdown\n");
	saveSettings();
	printf("[controld] data saved\n");
}


int main(int argc, char **argv)
{
	int listenfd, connfd;
	printf("Controld  0.1\n\n");

	if (fork() != 0) return 0;

	struct sockaddr_un servaddr;
	int clilen;
	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, CONTROLD_UDS_NAME);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	unlink(CONTROLD_UDS_NAME);

	//network-setup
	if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
	}

	if ( bind(listenfd, (struct sockaddr*) &servaddr, clilen) <0 )
	{
		perror("[controld] bind failed...\n");
		exit(-1);
	}


	if (listen(listenfd, 5) !=0)
	{
		perror("[controld] listen failed...\n");
		exit( -1 );
	}

	signal(SIGHUP,sig_catch);
	signal(SIGKILL,sig_catch);
	signal(SIGTERM,sig_catch);

	if (!loadSettings())
	{
		printf("[controld] using defaults\n");
		settings.volume = 100;
		settings.mute = 0;
		settings.videotype = 1; // fblk1 - rgb
		settings.videoformat = 2; // fnc2 - 4:3
		settings.boxtype = 1; //nokia
	}

	watchDog = new CEventWatchDog();
	aspectRatioNotifier = new CControldAspectRatioNotifier();
	//init
	setVolume(settings.volume);
	setVideoType(settings.videotype);
	setVideoFormat(settings.videoformat);

	struct CControldClient::commandHead rmessage;
	while(1)
	{
		connfd = accept(listenfd, (struct sockaddr*) &servaddr, (socklen_t*) &clilen);
		memset(&rmessage, 0, sizeof(rmessage));
		read(connfd,&rmessage,sizeof(rmessage));

		parse_command(connfd, &rmessage);

		close(connfd);
  }

}

void CControldAspectRatioNotifier::aspectRatioChanged( int newAspectRatio)
{
//	printf("[controld] CControldAspectRatioNotifier::aspectRatioChanged( %d) \n", newAspectRatio);
	switch (newAspectRatio)
	{
		case 2 : setVideoFormat(2, false); break;
		case 3 : setVideoFormat(1, false); break;
		default: printf("[controld] Unknown apsectRatio: %d", newAspectRatio);
	}
}

