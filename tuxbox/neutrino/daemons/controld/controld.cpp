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

#include "controldMsg.h"
#include "dbox/avs_core.h"
#include "dbox/fp.h"
#include "dbox/saa7126_core.h"
#include "ost/video.h"
#include "eventwatchdog.h"
#include "controldclient.h"
#include "lcddclient.h"
#include "zapitclient.h"
#include "eventserver.h"

#define CONF_FILE CONFIGDIR "/controld.conf"
#define SAA7126_DEVICE "/dev/dbox/saa0"

CLcddClient		lcdd;
CZapitClient	zapit;
CEventServer    *eventServer;

struct Ssettings
{
	char volume;
	bool mute;
	char videooutput;
	char videoformat;

	char boxtype;
	char lastmode;
} settings;

int	nokia_scart[4];
int	nokia_dvb[4];
int	sagem_scart[4];
int	sagem_dvb[4];
int	philips_scart[4];
int	philips_dvb[4];
char aspectRatio;

void sig_catch(int);

class CControldAspectRatioNotifier : public CAspectRatioNotifier
{
	public:
		virtual void aspectRatioChanged( int newAspectRatio); //override;
};

CEventWatchDog* watchDog;
CControldAspectRatioNotifier* aspectRatioNotifier;

int loadSettings(Ssettings* lsettings=NULL)
{
	if(!lsettings)
	{
		lsettings = &settings;
	}
	int fd;
	fd = open(CONF_FILE, O_RDONLY );

	if (fd==-1)
	{
		printf("[controld] error while loading settings: %s\n", CONF_FILE );
		return 0;
	}
	if(read(fd, lsettings, sizeof(Ssettings))!=sizeof(Ssettings))
	{
		printf("[controld] error while loading settings: %s - config from old version?\n", CONF_FILE );
		return 0;
	}
	close(fd);

	return 1;
}

void saveSettings()
{
	bool tosave = false;

	Ssettings tmp;
	if(loadSettings(&tmp)==1)
	{
		//compare...
		if(memcmp(&tmp, &settings, sizeof(Ssettings))!=0)
		{
			tosave=true;
		}
	}
	else
	{
		tosave=true;
	}

	if(tosave)
	{
		int fd;
		fd = open(CONF_FILE, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR  |  S_IRGRP | S_IWGRP  |  S_IROTH | S_IWOTH );

		if (fd==-1)
		{
			printf("[controld] error while saving settings: %s\n", CONF_FILE );
			return;
		}
		write(fd, &settings,  sizeof(Ssettings) );
		close(fd);
		printf("[controld] settings saved\n");
	}
}

void shutdownBox()
{
	lcdd.shutdown();
	saveSettings();

	if (execlp("/sbin/halt", "/sbin/halt", 0)<0)
	{
		perror("exec failed - halt\n");
	}
}

void setvideooutput(int format, bool bSaveSettings = true)
{
	int fd;
	if (format < 0)
	{
		format=0;
	}
	if (format > 3)
	{
		format=3;
	}

	// 0 - COMPOSITE
	// 1 - RGB
	// 2 - SVIDEO

	if (bSaveSettings) // only set settings if we dont come from watchdog
		settings.videooutput = format;

	int	arg;

    switch ( format )
	{
	 	case 0:
	 		arg = 0;
	 		break;
		case 1:
	 		arg = 1;
	 		break;
		case 2:
	 		arg = 0;
	 		break;
    }
	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd, AVSIOSFBLK, &arg)< 0)
	{
		perror("AVSIOSFBLK:");
		return;
	}
	close(fd);


	switch ( format )
	{
	 	case 0:
	 		arg = SAA_MODE_FBAS;
	 		break;
		case 1:
	 		arg = SAA_MODE_RGB;
	 		break;
		case 2:
	 		arg = SAA_MODE_SVIDEO;
	 		break;
    }
	if((fd = open(SAA7126_DEVICE,O_RDWR|O_NONBLOCK)) < 0)
	{
		perror("[controld] SAA DEVICE: ");
		return;
	}

	if ( (ioctl(fd, SAAIOSMODE, &arg) < 0))
	{
		perror("[controld] IOCTL: ");
		close(fd);
		return;
	}
	close(fd);

}

void setVideoFormat(int format, bool bSaveFormat = true )
{
	int fd;
	int videoDisplayFormat;

	/*
		16:9 : fnc 1
		4:3  : fnc 2
	*/


	if (bSaveFormat) // only set settings if we dont come from watchdog or video_off
	{
		if (format < 0)
			format=0;
		if (format > 3)
			format=3;

		settings.videoformat = format;
	}

	if (format==0) // automatic switch
	{
		printf("[controld] setting VideoFormat to auto \n");

		switch ( aspectRatio )
		{
			case 2 :
				format= 2;
				break;
			case 3 :
				format= 1;
				break;
			default:
				format= 2;
				// damits nicht ausgeht beim starten :)
		}
	}

	if ((fd = open("/dev/dbox/avs0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}
	if (format< 0)
		format= 0;

	int avsiosfncFormat = format;
	if (settings.boxtype == CControldClient::BOXTYPE_PHILIPS) // Philips
	{
		switch (format)
		{
			case 1 :
				avsiosfncFormat=2;
				break;
			case 2 :
				avsiosfncFormat=3;
				break;
		}
	}
	if (ioctl(fd,AVSIOSFNC,&avsiosfncFormat)< 0)
	{
		perror("AVSIOSFNC:");
		return;
	}
	close(fd);

    switch( format )
	{
		//	?	case AVS_FNCOUT_INTTV	: videoDisplayFormat = VIDEO_PAN_SCAN;
		case AVS_FNCOUT_EXT169	:
			videoDisplayFormat = VIDEO_CENTER_CUT_OUT;
			break;
		case AVS_FNCOUT_EXT43	:
			videoDisplayFormat = VIDEO_LETTER_BOX;
			break;
		default:
			videoDisplayFormat = VIDEO_LETTER_BOX;
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

}

void LoadScart_Settings()
{
	FILE* fd = fopen(CONFIGDIR"/scart.conf", "r");
	if(fd)
	{
		printf("[controld]: loading scart-config (scart.conf)\n");

		char buf[1000];
		fgets(buf,sizeof(buf),fd);

		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf( buf, "nokia_scart: %d %d %d %d\n", &nokia_scart[0], &nokia_scart[1], &nokia_scart[2], &nokia_scart[3] );
			//printf( buf );
		}
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf( buf, "nokia_dvb: %d %d %d %d\n", &nokia_dvb[0], &nokia_dvb[1], &nokia_dvb[2], &nokia_dvb[3] );
			//printf( buf );
		}
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf( buf, "sagem_scart: %d %d %d %d\n", &sagem_scart[0], &sagem_scart[1], &sagem_scart[2], &sagem_scart[3] );
			//printf( buf );
		}
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf( buf, "sagem_dvb: %d %d %d %d\n", &sagem_dvb[0], &sagem_dvb[1], &sagem_dvb[2], &sagem_dvb[3] );
			//printf( buf );
		}
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf( buf, "philips_scart: %d %d %d %d\n", &philips_scart[0], &philips_scart[1], &philips_scart[2], &philips_scart[3] );
			//printf( buf );
		}
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf( buf, "philips_dvb: %d %d %d %d\n", &philips_dvb[0], &philips_dvb[1], &philips_dvb[2], &philips_dvb[3] );
			//printf( buf );
		}
		fclose(fd);
	}
	else
	{
		printf("[controld]: failed to load scart-config (scart.conf), using standard-values\n");

		// scart
		sagem_scart[0]= 2;
		sagem_scart[1]= 1;
		sagem_scart[2]= 0;
		sagem_scart[3]= 0;

		nokia_scart[0]= 3;
		nokia_scart[1]= 2;
		nokia_scart[2]= 1;
		nokia_scart[3]= 0;

		philips_scart[0]= 2;
		philips_scart[1]= 2;
		philips_scart[2]= 3;
		philips_scart[3]= 0;

		// dvb
		sagem_dvb[0]= 0;
		sagem_dvb[1]= 0;
		sagem_dvb[2]= 0;
		sagem_dvb[3]= 0;

		nokia_dvb[0]= 5;
		nokia_dvb[1]= 1;
		nokia_dvb[2]= 1;
		nokia_dvb[3]= 0;

		philips_dvb[0]= 1;
		philips_dvb[1]= 1;
		philips_dvb[2]= 1;
		philips_dvb[3]= 0;
	}
}


void routeVideo(int v1, int a1, int v2, int a2, int fblk)
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

	if (ioctl(fd,AVSIOSVSW1,&v1)< 0)
	{
		perror("AVSIOSVSW1:");
		return;
	}

	if (ioctl(fd,AVSIOSASW1,&a1)< 0)
	{
		perror("AVSIOSASW1:");
		return;
	}

	if (ioctl(fd,AVSIOSVSW2,&v2)< 0)
	{
		perror("AVSIOSVSW2:");
		return;
	}

	if (ioctl(fd,AVSIOSASW2,&a2)< 0)
	{
		perror("AVSIOSASW2:");
		return;
	}

	close(fd);
}

char BoxNames[4][10] = {"","Nokia", "Sagem", "Philips"};
void switch_vcr( bool vcr_on)
{
	LoadScart_Settings();

	if (vcr_on)
	{
		//turn to scart-input
		printf("[controld]: switch to scart-input... (%s)\n", BoxNames[settings.boxtype]);
		if (settings.boxtype == 2) // Sagem
		{
			routeVideo(sagem_scart[0], sagem_scart[1], sagem_scart[2], sagem_scart[3], 0);
		}
		else if (settings.boxtype == 1) // Nokia
		{
			routeVideo(nokia_scart[0], nokia_scart[1], nokia_scart[2], nokia_scart[3], 2);
		}
		else if (settings.boxtype == 3) // Philips
		{
			routeVideo(philips_scart[0], philips_scart[1], philips_scart[2], philips_scart[3], 2);
		}
	}
	else
	{	//turn to dvb...
		printf("[controld]: switch to dvb-input... (%s)\n", BoxNames[settings.boxtype]);
		if (settings.boxtype == 2) // Sagem
		{
			routeVideo( sagem_dvb[0], sagem_dvb[1], sagem_dvb[2], sagem_dvb[3], settings.videooutput);
		}
		else if (settings.boxtype == 1) // Nokia
		{
			routeVideo( nokia_dvb[0], nokia_dvb[1], nokia_dvb[2], nokia_dvb[3], settings.videooutput);
		}
		else if (settings.boxtype == 3) // Philips
		{
			routeVideo( philips_dvb[0], philips_dvb[1], philips_dvb[2], philips_dvb[3], settings.videooutput);
		}
	}
}

void setScartMode(bool onoff)
{
	if(onoff)
	{
		lcdd.setMode(CLcddClient::MODE_SCART);
	}
	else
	{
		lcdd.setMode(CLcddClient::MODE_TVRADIO);
	}
	switch_vcr( onoff );
}

void disableVideoOutput(bool disable)
{
	int arg=disable?1:0;
	int fd;
	printf("[controld] videoOutput %s\n", disable?"off":"on");

	if((fd = open(SAA7126_DEVICE,O_RDWR|O_NONBLOCK)) < 0)
	{
		perror("[controld] SAA DEVICE: ");
		return;
	}

	if ( (ioctl(fd,SAAIOSPOWERSAVE,&arg) < 0))
	{
		perror("[controld] IOCTL: ");
		close(fd);
		return;
	}
	close(fd);
	/*
		arg=disable?0:0xf;
		if((fd = open("/dev/dbox/fp0",O_RDWR|O_NONBLOCK)) < 0)
		{
			perror("[controld] FP DEVICE: ");
			return;
		}

		if ( (ioctl(fd,FP_IOCTL_LCD_DIMM,&arg) < 0))
		{
			perror("[controld] IOCTL: ");
			close(fd);
			return;
		}
		close(fd);
	*/
	if(!disable)
	{
		zapit.startPlayBack();
		setvideooutput(settings.videooutput, false);
		setVideoFormat(settings.videoformat, false);
	}
	else
	{
		int fd;

		setvideooutput(0, false);
		setVideoFormat(-1, false);
		zapit.stopPlayBack();
	}
}

void setBoxType()
{
	FILE* fd = fopen("/proc/bus/dbox", "rt");
	if (fd==NULL)
	{
		printf("[controld] error while opening /proc/bus/dbox\n" );
		return;
	}

	int mID;

	char *tmpptr,buf[100], buf2[100];
	int value, pos=0;
	if(!feof(fd))
	{
		if(fgets(buf,29,fd)!=NULL)
		{
			buf[strlen(buf)-1]=0;
			tmpptr=buf;
			strsep(&tmpptr,"=");
			mID=atoi(tmpptr);
			//printf("%s: %d\n",buf,mID);
		}
	}
	fclose(fd);

	switch ( mID )
	{
		case 3:	settings.boxtype= CControldClient::BOXTYPE_SAGEM;
				break;
		case 2:	settings.boxtype= CControldClient::BOXTYPE_PHILIPS;
				break;
		default:
			settings.boxtype= CControldClient::BOXTYPE_NOKIA;
	}
	//printf("settings.boxtype: %d\n", settings.boxtype);

}

void setVolume(char volume)
{
	int fd;
	settings.volume = volume;

	int i = 64-int(volume*64.0/100.0);
	//printf("[controld] set volume: %d\n", i );
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


	lcdd.setVolume(volume);
	eventServer->sendEvent( CControldClient::EVT_VOLUMECHANGED, CEventServer::INITID_CONTROLD, &volume, sizeof(volume) );
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

	lcdd.setMute(true);
	eventServer->sendEvent(CControldClient::EVT_MUTECHANGED, CEventServer::INITID_CONTROLD, &settings.mute, sizeof(settings.mute));
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

	lcdd.setMute(false);
	eventServer->sendEvent(CControldClient::EVT_MUTECHANGED, CEventServer::INITID_CONTROLD, &settings.mute, sizeof(settings.mute));
}


void parse_command(int connfd, CControld::commandHead* rmessage)
{

	if(rmessage->version!=CControld::ACTVERSION)
	{
		perror("[controld] unknown version\n");
		return;
	}
	switch (rmessage->cmd)
	{
		case CControld::CMD_SHUTDOWN:
			//printf("[controld] shutdown\n");
			shutdownBox();
			break;
		case CControld::CMD_SETVOLUME:
			//printf("[controld] set volume\n");
			CControld::commandVolume msg;
			read(connfd, &msg, sizeof(msg));
			setVolume(msg.volume);
			break;
		case CControld::CMD_MUTE:
			//printf("[controld] mute\n");
			Mute();
			break;
		case CControld::CMD_UNMUTE:
			//printf("[controld] unmute\n");
			UnMute();
			break;
		case CControld::CMD_SETVIDEOFORMAT:
			//printf("[controld] set videoformat\n");
			CControld::commandVideoFormat msg2;
			read(connfd, &msg2, sizeof(msg2));
			setVideoFormat(msg2.format);
			break;
		case CControld::CMD_SETVIDEOOUTPUT:
			//printf("[controld] set videooutput\n");
			CControld::commandVideoOutput msg3;
			read(connfd, &msg3, sizeof(msg3));
			setvideooutput(msg3.output);
			break;
		case CControld::CMD_SETBOXTYPE:
			//printf("[controld] set boxtype\n");
			CControld::commandBoxType msg4;
			read(connfd, &msg4, sizeof(msg4));
			setBoxType(msg4.boxtype);
			break;
		case CControld::CMD_SETSCARTMODE:
			//printf("[controld] set scartmode\n");
			CControld::commandScartMode msg5;
			read(connfd, &msg5, sizeof(msg5));
			setScartMode(msg5.mode);
			break;
		case CControld::CMD_SETVIDEOPOWERDOWN:
			//printf("[controld] set scartmode\n");
			CControld::commandVideoPowerSave msg10;
			read(connfd, &msg10, sizeof(msg10));
			disableVideoOutput(msg10.powerdown);
			break;
		case CControld::CMD_SAVECONFIG:
			saveSettings();
			break;

		case CControld::CMD_GETVOLUME:
			//printf("[controld] get volume\n");
			CControld::responseVolume msg6;
			msg6.volume = settings.volume;
			write(connfd,&msg6,sizeof(msg6));
			break;
		case CControld::CMD_GETMUTESTATUS:
			//printf("[controld] get mute\n");
			CControld::responseMute msg7;
			msg7.mute = settings.mute;
			write(connfd,&msg7,sizeof(msg7));
			break;
		case CControld::CMD_GETVIDEOFORMAT:
			//printf("[controld] get videoformat (fnc)\n");
			CControld::responseVideoFormat msg8;
			msg8.format = settings.videoformat;
			write(connfd,&msg8,sizeof(msg8));
			break;
		case CControld::CMD_GETASPECTRATIO:
			//printf("[controld] get videoformat (fnc)\n");
			CControld::responseAspectRatio msga;
			msga.aspectRatio = aspectRatio;
			write(connfd,&msga,sizeof(msga));
			break;
		case CControld::CMD_GETVIDEOOUTPUT:
			//printf("[controld] get videooutput (fblk)\n");
			CControld::responseVideoOutput msg9;
			msg9.output = settings.videooutput;
			write(connfd,&msg9,sizeof(msg9));
			break;
		case CControld::CMD_GETBOXTYPE:
			//printf("[controld] get boxtype\n");
			CControld::responseBoxType msg0;
			msg0.boxtype = settings.boxtype;
			write(connfd,&msg0,sizeof(msg0));
			break;

		case CControld::CMD_REGISTEREVENT:
			eventServer->registerEvent(connfd);
			break;
		case CControld::CMD_UNREGISTEREVENT:
			eventServer->unRegisterEvent(connfd);
			break;

		default:
			printf("[controld] unknown command\n");
	}
}


void sig_catch(int)
{
	saveSettings();
}



int main(int argc, char **argv)
{
	int listenfd, connfd;
	printf("Controld  $Id: controld.cpp,v 1.51 2002/03/15 17:07:29 McClean Exp $\n\n");

	//printf("[controld] mainThread-pid: %d\n", getpid());
	if (fork() != 0)
		return 0;

    //printf("[controld] forkedThread-pid: %d\n", getpid());
	eventServer = new CEventServer;

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

	//busyBox
	signal(SIGHUP,sig_catch);
	signal(SIGINT,sig_catch);
	signal(SIGQUIT,sig_catch);
	signal(SIGTERM,sig_catch);

	if (!loadSettings())
	{
		printf("[controld] using defaults\n");
		settings.volume = 100;
		settings.mute = 0;
		settings.videooutput = 1; // fblk1 - rgb
		settings.videoformat = 2; // fnc2 - 4:3
		settings.boxtype = 1; //nokia
	}

	setBoxType(); // dummy set - liest den aktuellen Wert aus!

	watchDog = new CEventWatchDog();
	aspectRatioNotifier = new CControldAspectRatioNotifier();
	watchDog->registerNotifier(WDE_VIDEOMODE, aspectRatioNotifier);

	//init
	setVolume(settings.volume);
	setvideooutput(settings.videooutput);
	setVideoFormat(settings.videoformat, false);
	if (settings.mute== 1)
		Mute();


    try
    {
		struct CControld::commandHead rmessage;
		while(1)
		{
			connfd = accept(listenfd, (struct sockaddr*) &servaddr, (socklen_t*) &clilen);
			memset(&rmessage, 0, sizeof(rmessage));
			read(connfd,&rmessage,sizeof(rmessage));

			parse_command(connfd, &rmessage);
			close(connfd);
		}
	}
	catch (std::exception& e)
	{
		printf("[controld] caught std-exception in main-thread %s!\n", e.what());
	}
	catch (...)
	{
	    printf("[controld] caught exception in main-thread!\n");
  	}
}

void CControldAspectRatioNotifier::aspectRatioChanged( int newAspectRatio )
{
	//printf("[controld] CControldAspectRatioNotifier::aspectRatioChanged( %d ) \n", newAspectRatio);
	aspectRatio= newAspectRatio;

	if ( settings.videoformat == 0 )
	{
		switch (newAspectRatio)
		{
			case 2 :
				setVideoFormat( 2, false );
				break;
			case 3 :
				setVideoFormat( 1, false );
				break;
			default:
				printf("[controld] Unknown apsectRatio: %d", newAspectRatio);
		}
	}
}

