/*
  Control-Daemon  -   DBoxII-Project

  Copyright (C) 2001 Steffen Hehn 'McClean',
  2002 dboxII-team
	
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

#define USE_LIBTUXBOX 1

#include <config.h>

#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <dbox/avs_core.h>
#include <dbox/fp.h>
#include <dbox/saa7126_core.h>

#include <zapit/client/zapitclient.h>

#include <controldclient/controldclient.h>
#include <controldclient/controldMsg.h>
#include <timerdclient/timerdclient.h>

#include <basicserver.h>
#include <configfile.h>
#include <eventserver.h>
#ifdef USE_LIBTUXBOX
#include <tuxbox.h>
#endif

#include "eventwatchdog.h"
#include "driver/audio.h"


#define CONF_FILE CONFIGDIR "/controld.conf"
#define AVS_DEVICE	"/dev/dbox/avs0"
#define SAA7126_DEVICE	"/dev/dbox/saa0"


CZapitClient	zapit;
CTimerdClient	timerd;
CEventServer	*eventServer;

/* the configuration file */
CConfigFile * config = NULL;

struct Ssettings
{
	int  volume;
	int  volume_avs;
	bool mute;
	bool mute_avs;
	bool scale_logarithmic;
	bool scale_logarithmic_avs;
	int  videooutput;
	int  videoformat;
	int  csync;

	CControldClient::tuxbox_maker_t boxtype; // not part of the config - set by setBoxType()
} settings;

int	nokia_scart[7];
int	nokia_dvb[6];
int	sagem_scart[7];
int	sagem_dvb[6];
int	philips_scart[7];
int	philips_dvb[6];
char aspectRatio_vcr;
char aspectRatio_dvb;
bool vcr;
bool videoOutputDisabled;



void sig_catch(int);

class CControldAspectRatioNotifier : public CAspectRatioNotifier
{
public:
	virtual void aspectRatioChanged( int newAspectRatio); //override;
};

CEventWatchDog* watchDog;
CControldAspectRatioNotifier* aspectRatioNotifier;

void saveSettings()
{
	config->saveConfig(CONF_FILE);
}

void shutdownBox()
{
	timerd.shutdown();

	saveSettings();
}

void setRGBCsync(int val)
{
	int fd;
	settings.csync = val;
	if ((fd = open(SAA7126_DEVICE, O_RDWR|O_NONBLOCK)) < 0)
		perror("[controld] " SAA7126_DEVICE);

	else {
		if ((ioctl(fd, SAAIOSCSYNC, &settings.csync) < 0))
			perror("[controld] SAAIOSCSYNC");
		
		close(fd);
	}
	config->setInt32("csync", settings.csync);
}

char getRGBCsync()
{
	int fd, val=0;
	if ((fd = open(SAA7126_DEVICE, O_RDWR|O_NONBLOCK)) < 0)
		perror("[controld] " SAA7126_DEVICE);

	else {
		if ((ioctl(fd, SAAIOGCSYNC, &val) < 0))
			perror("[controld] SAAIOGCSYNC");
		
		close(fd);
	}
	return val;
}

void setvideooutput(int format, bool bSaveSettings = true)
{
	int fd;
	if (format < 0)
	{
		format=0;
	}
	if (format > 5)
	{
		format=5;
	}

	// 0 - CVBS only
	// 1 - RGB with CVBS
	// 2 - SVIDEO
	// 3 - YUV with VBS
	// 4 - YUV with CVBS
	
	if (bSaveSettings) // only set settings if we dont come from watchdog
	{
		settings.videooutput = format;
		config->setInt32("videooutput", settings.videooutput);
	}

	int	arg;

	switch (format)
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
	case 3:
		arg = 1;
		break;
	case 4:
		arg = 1;
		break;
	}

	if ((fd = open(AVS_DEVICE, O_RDWR)) < 0)
		perror("[controld] " AVS_DEVICE);
	else {
		if (ioctl(fd, AVSIOSFBLK, &arg)< 0)
			perror("[controld] AVSIOSFBLK");

		close(fd);
	}

	switch (format)
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
	case 3:
		arg = SAA_MODE_YUV_V;
		break;
	case 4:
		arg = SAA_MODE_YUV_C;
		break;
	}

	if ((fd = open(SAA7126_DEVICE, O_RDWR|O_NONBLOCK)) < 0)
		perror("[controld] " SAA7126_DEVICE);

	else {
		if ((ioctl(fd, SAAIOSMODE, &arg) < 0))
			perror("[controld] SAAIOSMODE");
		
		close(fd);
	}
	
	if(format == 1 || format == 3 || format == 4)
		setRGBCsync(settings.csync);
}

void setVideoFormat(int format, bool bSaveFormat = true )
{
	int fd;
	video_display_format_t videoDisplayFormat;
	int avsiosfncFormat;
	int wss;

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
		config->setInt32("videoformat", settings.videoformat);
	}

	if (format == 0) // automatic switch
	{
		printf("[controld] setting VideoFormat to auto \n");
		int activeAspectRatio;
	      if (vcr)
		  activeAspectRatio = aspectRatio_vcr;
		else
		  activeAspectRatio = aspectRatio_dvb;

		switch (activeAspectRatio)
		{
		case 0 :	// 4:3
			format= 2;
			break;
		case 1 :	// 16:9
		case 2 :	// 21,1:1
			format= 1;
			break;
		default:
			format= 2;
			// damits nicht ausgeht beim starten :)
		}
	}

	if ((fd = open(AVS_DEVICE, O_RDWR)) < 0)
		perror("[controld] " AVS_DEVICE);
	else
	{
		if (format < 0)
			format= 0;

		avsiosfncFormat = format;

		if (settings.boxtype == CControldClient::TUXBOX_MAKER_PHILIPS)
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

		if (ioctl(fd,AVSIOSFNC, &avsiosfncFormat)< 0)
			perror("[controld] AVSIOSFNC");
		
		close(fd);
	}

	switch (format)
	{
		//	?	case AVS_FNCOUT_INTTV	: videoDisplayFormat = VIDEO_PAN_SCAN;
	case AVS_FNCOUT_EXT169:
		videoDisplayFormat = ZAPIT_VIDEO_CENTER_CUT_OUT;
		wss = SAA_WSS_169F;
		break;
	case AVS_FNCOUT_EXT43:
		videoDisplayFormat = ZAPIT_VIDEO_LETTER_BOX;
		wss = SAA_WSS_43F;
		break;
	case AVS_FNCOUT_EXT43_1: 
		videoDisplayFormat = ZAPIT_VIDEO_PAN_SCAN;
		wss = SAA_WSS_43F;
		break;
	default:
		videoDisplayFormat = ZAPIT_VIDEO_LETTER_BOX;
		wss = SAA_WSS_43F;
		break;
	}

	zapit.setDisplayFormat(videoDisplayFormat);

	if ((fd = open(SAA7126_DEVICE,O_RDWR)) < 0)
		perror("[controld] " SAA7126_DEVICE);

	else {
		if (ioctl(fd,SAAIOSWSS,&wss) < 0)
			perror("[controld] SAAIOSWSS");

		close(fd);
	}
}

void LoadScart_Settings()
{
	// scart
	sagem_scart[0]= 2;
	sagem_scart[1]= 1;
	sagem_scart[2]= 0;
	sagem_scart[3]= 0;
	sagem_scart[4]= 0;
	sagem_scart[5]= 0;
	sagem_scart[6]= 0;

	nokia_scart[0]= 3;
	nokia_scart[1]= 2;
	nokia_scart[2]= 1;
	nokia_scart[3]= 0;
	nokia_scart[4]= 1;
	nokia_scart[5]= 1;
	nokia_scart[6]= 2;

	philips_scart[0]= 2;
	philips_scart[1]= 2;
	philips_scart[2]= 3;
	philips_scart[3]= 0;
	philips_scart[4]= 3;
	philips_scart[5]= 0;
	philips_scart[6]= 2;

	// dvb
	sagem_dvb[0]= 0;
	sagem_dvb[1]= 0;
	sagem_dvb[2]= 0;
	sagem_dvb[3]= 0;
	sagem_dvb[4]= 0;
	sagem_dvb[5]= 0;

	nokia_dvb[0]= 5;
	nokia_dvb[1]= 1;
	nokia_dvb[2]= 1;
	nokia_dvb[3]= 0;
	nokia_dvb[4]= 1;
	nokia_dvb[5]= 0;

	philips_dvb[0]= 1;
	philips_dvb[1]= 1;
	philips_dvb[2]= 1;
	philips_dvb[3]= 0;
	philips_dvb[4]= 1;
	philips_dvb[5]= 0;

	FILE* fd = fopen(CONFIGDIR"/scart.conf", "r");
	if(fd)
	{
		printf("[controld]: loading scart-config (scart.conf)\n");

		char buf[1000];
		fgets(buf,sizeof(buf),fd);

		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			if(strlen(buf) > 26)
				sscanf( buf, "nokia_scart: %d %d %d %d %d %d %d\n", &nokia_scart[0], &nokia_scart[1], &nokia_scart[2], &nokia_scart[3], &nokia_scart[4], &nokia_scart[5], &nokia_scart[6] );
			else
				sscanf( buf, "nokia_scart: %d %d %d %d %d %d\n", &nokia_scart[0], &nokia_scart[1], &nokia_scart[2], &nokia_scart[3], &nokia_scart[4], &nokia_scart[5] );
			//printf( buf );
		}
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf( buf, "nokia_dvb: %d %d %d %d %d %d\n", &nokia_dvb[0], &nokia_dvb[1], &nokia_dvb[2], &nokia_dvb[3], &nokia_dvb[4], &nokia_dvb[5] );
			//printf( buf );
		}
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			if(strlen(buf) > 26)
				sscanf( buf, "sagem_scart: %d %d %d %d %d %d %d\n", &sagem_scart[0], &sagem_scart[1], &sagem_scart[2], &sagem_scart[3], &sagem_scart[4], &sagem_scart[5], &sagem_scart[6] );
			else
				sscanf( buf, "sagem_scart: %d %d %d %d %d %d\n", &sagem_scart[0], &sagem_scart[1], &sagem_scart[2], &sagem_scart[3], &sagem_scart[4], &sagem_scart[5] );
			//printf( buf );
		}
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf( buf, "sagem_dvb: %d %d %d %d %d %d\n", &sagem_dvb[0], &sagem_dvb[1], &sagem_dvb[2], &sagem_dvb[3], &sagem_dvb[4], &sagem_dvb[5] );
			//printf( buf );
		}
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			if(strlen(buf) > 26)
				sscanf( buf, "philips_scart: %d %d %d %d %d %d %d\n", &philips_scart[0], &philips_scart[1], &philips_scart[2], &philips_scart[3], &philips_scart[4], &philips_scart[5], &philips_scart[6] );
			else
				sscanf( buf, "philips_scart: %d %d %d %d %d %d\n", &philips_scart[0], &philips_scart[1], &philips_scart[2], &philips_scart[3], &philips_scart[4], &philips_scart[5] );
			//printf( buf );
		}
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			sscanf( buf, "philips_dvb: %d %d %d %d %d %d\n", &philips_dvb[0], &philips_dvb[1], &philips_dvb[2], &philips_dvb[3], &philips_dvb[4], &philips_dvb[5] );
			//printf( buf );
		}
		fclose(fd);
	}
	else
	{
		printf("[controld]: failed to load scart-config (scart.conf), using standard-values\n");
	}
}


void routeVideo(int v1, int a1, int v2, int a2, int v3, int a3, int fblk)
{
	int fd;

	if ((fd = open(AVS_DEVICE, O_RDWR)) < 0)
		perror("[controld] " AVS_DEVICE);

	else if (ioctl(fd, AVSIOSFBLK, &fblk) < 0)
		perror("[controld] AVSIOSFBLK");

	else if (ioctl(fd, AVSIOSVSW1, &v1) < 0)
		perror("[controld] AVSIOSVSW1");

	else if (ioctl(fd, AVSIOSASW1, &a1) < 0)
		perror("[controld] AVSIOSASW1");

	else if (ioctl(fd, AVSIOSVSW2, &v2) < 0)
		perror("[controld] AVSIOSVSW2");

	else if (ioctl(fd, AVSIOSASW2, &a2) < 0)
		perror("[controld] AVSIOSASW2");

	else if (ioctl(fd, AVSIOSVSW3, &v3) < 0)
		perror("[controld] AVSIOSVSW3");

	else if (ioctl(fd, AVSIOSASW3, &a3) < 0)
		perror("[controld] AVSIOSASW3");

	if (fd != -1)
		close(fd);
}

void switch_vcr( bool vcr_on)
{
	int activeAspectRatio;
	LoadScart_Settings();
	vcr = vcr_on;
	if (vcr_on)
	{
		//turn to scart-input
	   activeAspectRatio = aspectRatio_vcr;
		printf("[controld]: switch to scart-input... (%d)\n", settings.boxtype);
		if (settings.boxtype == CControldClient::TUXBOX_MAKER_SAGEM)
		{
			routeVideo(sagem_scart[0], sagem_scart[1], sagem_scart[2], sagem_scart[3], sagem_scart[4], sagem_scart[5], sagem_scart[6]);
		}
		else if (settings.boxtype == CControldClient::TUXBOX_MAKER_NOKIA)
		{
			routeVideo(nokia_scart[0], nokia_scart[1], nokia_scart[2], nokia_scart[3], nokia_scart[4], nokia_scart[5], nokia_scart[6]);
		}
		else if (settings.boxtype == CControldClient::TUXBOX_MAKER_PHILIPS)
		{
			routeVideo(philips_scart[0], philips_scart[1], philips_scart[2], philips_scart[3], philips_scart[4], philips_scart[5], philips_scart[6]);
		}
	}
	else
	{	//turn to dvb...
	   activeAspectRatio = aspectRatio_dvb;
		printf("[controld]: switch to dvb-input... (%d)\n", settings.boxtype);
		if (settings.boxtype == CControldClient::TUXBOX_MAKER_SAGEM)
		{
			routeVideo( sagem_dvb[0], sagem_dvb[1], sagem_dvb[2], sagem_dvb[3], sagem_dvb[4], sagem_dvb[5], settings.videooutput);
		}
		else if (settings.boxtype == CControldClient::TUXBOX_MAKER_NOKIA)
		{
			routeVideo( nokia_dvb[0], nokia_dvb[1], nokia_dvb[2], nokia_dvb[3], nokia_dvb[4], nokia_dvb[5], settings.videooutput);
		}
		else if (settings.boxtype == CControldClient::TUXBOX_MAKER_PHILIPS)
		{
			routeVideo( philips_dvb[0], philips_dvb[1], philips_dvb[2], philips_dvb[3], philips_dvb[4], philips_dvb[5], settings.videooutput);
		}
	}

	// recall AspectRatio when switching between DVB and VCR
	if ( settings.videoformat == 0 )
	{
		switch (activeAspectRatio)
		{
		case 0 :	// 4:3
			setVideoFormat( 2, false );
			break;
		case 1 :	// 16:9
		case 2 :	// 2,21:1
			setVideoFormat( 1, false );
			break;
		default:
			printf("[controld] Unknown aspectRatio: %d", activeAspectRatio);
		}
	}
}

void setScartMode(bool onoff)
{
	if(onoff)
	{
		audioControl::setMute(settings.mute_avs); // unmute AVS
		//lcdd.setMode(CLcddTypes::MODE_SCART);
	}
	else
	{
		//lcdd.setMode(CLcddTypes::MODE_TVRADIO);
	}
	switch_vcr( onoff );
}

void disableVideoOutput(bool disable)
{
	int arg=disable?1:0;
	videoOutputDisabled=disable;
	int fd;
	printf("[controld] videoOutput %s\n", disable?"off":"on");

	if ((fd = open(SAA7126_DEVICE,O_RDWR|O_NONBLOCK)) < 0)
		perror("[controld] " SAA7126_DEVICE);

	else 
	{
		if ((ioctl(fd,SAAIOSPOWERSAVE,&arg) < 0))
			perror("[controld] SAAIOSPOWERSAVE");

		close(fd);
	}

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

	if (!disable)
	{
		//zapit.setStandby(false);
		zapit.muteAudio(false);
		audioControl::setMute(settings.mute_avs);
		setvideooutput(settings.videooutput, false);
		setVideoFormat(settings.videoformat, false);
	}
	else
	{
		setvideooutput(0, false);
		setVideoFormat(-1, false);
		//zapit.setStandby(true);
		zapit.muteAudio(true);
		audioControl::setMute(true);
	}
}

void setBoxType()
{
#ifdef USE_LIBTUXBOX
	switch ( tuxbox_get_vendor() )
	{
	case TUXBOX_VENDOR_SAGEM:
		settings.boxtype = CControldClient::TUXBOX_MAKER_SAGEM;
		break;
	case TUXBOX_VENDOR_PHILIPS:
		settings.boxtype = CControldClient::TUXBOX_MAKER_PHILIPS;
		break;
	case TUXBOX_VENDOR_NOKIA:
		settings.boxtype = CControldClient::TUXBOX_MAKER_NOKIA;
		break;
	case TUXBOX_VENDOR_DREAM_MM:
		settings.boxtype = CControldClient::TUXBOX_MAKER_DREAM_MM;
		break;
	case TUXBOX_VENDOR_TECHNOTREND:
		settings.boxtype = CControldClient::TUXBOX_MAKER_TECHNOTREND;
		break;
	default:
		settings.boxtype = CControldClient::TUXBOX_MAKER_UNKNOWN;
	}
	// fallback to old way ( via env. var)
	if (settings.boxtype==CControldClient::TUXBOX_MAKER_UNKNOWN)
	{
#endif
		char * strmID = getenv("mID");

		if (strmID == NULL)
			settings.boxtype = CControldClient::TUXBOX_MAKER_UNKNOWN;
		else
		{
			int mID = atoi(strmID);

			switch (mID)
			{
			case 3:	
				settings.boxtype= CControldClient::TUXBOX_MAKER_SAGEM;
				break;
			case 2:	
				settings.boxtype= CControldClient::TUXBOX_MAKER_PHILIPS;
				break;
			default:
				settings.boxtype= CControldClient::TUXBOX_MAKER_NOKIA;
			}
		}
		printf("[controld] Boxtype detected: (%d)\n", settings.boxtype);
#ifdef USE_LIBTUXBOX
	}
	else
		printf("[controld] Boxtype detected: (%d, %s %s)\n", settings.boxtype, tuxbox_get_vendor_str(), tuxbox_get_model_str());
#endif
}


// input:   0 (min volume) <=     volume           <= 100 (max volume)
// output: 63 (min volume) >= map_volume(., true)  >=   0 (max volume)
// output:  0 (min volume) <= map_volume(., false) <= 255 (max volume)
const unsigned char map_volume(const unsigned char volume, const bool to_AVS)
{
	if (to_AVS)
	{
		return settings.scale_logarithmic_avs ? 124 - (int)(61 * log10(10 + volume)) : 63 - ((((unsigned int)volume) * 63) / 100);
	}
	else
	{
		return settings.scale_logarithmic ? (int)(245 * log10(10 + volume)) - 245 : ((((unsigned int)volume) * 255) / 100);
	}
}

bool parse_command(CBasicMessage::Header &rmsg, int connfd)
{
	switch (rmsg.cmd)
	{
	case CControld::CMD_SHUTDOWN:
		return false;
		break;
		
	case CControld::CMD_SAVECONFIG:
		saveSettings();
		break;
		
	case CControld::CMD_SETVOLUME:
	case CControld::CMD_SETVOLUME_AVS:
		CControld::commandVolume msg_commandVolume;
		CBasicServer::receive_data(connfd, &msg_commandVolume, sizeof(msg_commandVolume));

		if (rmsg.cmd == CControld::CMD_SETVOLUME)
		{
			settings.volume = msg_commandVolume.volume;
			config->setInt32("volume", settings.volume);
			zapit.setVolume(map_volume(msg_commandVolume.volume, false), map_volume(msg_commandVolume.volume, false));
		}
		else
		{
			settings.volume_avs = msg_commandVolume.volume;
			config->setInt32("volume_avs", settings.volume_avs);
			audioControl::setVolume(map_volume(msg_commandVolume.volume, true));
		}
		//lcdd.setVolume(msg_commandVolume.volume);
#warning FIXME: generation of event is okay - however: event message is junk (what do i care about the new volume if i do not know which volume changed?)
		eventServer->sendEvent(CControldClient::EVT_VOLUMECHANGED, CEventServer::INITID_CONTROLD, &msg_commandVolume.volume, sizeof(msg_commandVolume.volume));
		break;

	case CControld::CMD_MUTE:
		settings.mute = true;
		config->setBool("mute", settings.mute);
		zapit.muteAudio(true);
		//lcdd.setMute(true);
		eventServer->sendEvent(CControldClient::EVT_MUTECHANGED, CEventServer::INITID_CONTROLD, &settings.mute, sizeof(settings.mute));
		break;
	case CControld::CMD_MUTE_AVS:
		settings.mute_avs = true;
		config->setBool("mute_avs", settings.mute_avs);
		audioControl::setMute(true);
		//lcdd.setMute(true);
		eventServer->sendEvent(CControldClient::EVT_MUTECHANGED, CEventServer::INITID_CONTROLD, &settings.mute_avs, sizeof(settings.mute_avs));
		break;
	case CControld::CMD_UNMUTE:
		settings.mute = false;
		config->setBool("mute", settings.mute);
		zapit.muteAudio(false);
		//lcdd.setMute(settings.mute_avs);
		eventServer->sendEvent(CControldClient::EVT_MUTECHANGED, CEventServer::INITID_CONTROLD, &settings.mute_avs, sizeof(settings.mute_avs));
		break;
	case CControld::CMD_UNMUTE_AVS:
		settings.mute_avs = false;
		config->setBool("mute_avs", settings.mute_avs);
		audioControl::setMute(false);
		//lcdd.setMute(settings.mute);
		eventServer->sendEvent(CControldClient::EVT_MUTECHANGED, CEventServer::INITID_CONTROLD, &settings.mute, sizeof(settings.mute));
		break;
		
	case CControld::CMD_SETVIDEOFORMAT:
		//printf("[controld] set videoformat\n");
		CControld::commandVideoFormat msg2;
		CBasicServer::receive_data(connfd, &msg2, sizeof(msg2));
		setVideoFormat(msg2.format);
		break;
	case CControld::CMD_SETVIDEOOUTPUT:
		//printf("[controld] set videooutput\n");
		CControld::commandVideoOutput msg3;
		CBasicServer::receive_data(connfd, &msg3, sizeof(msg3));
		setvideooutput(msg3.output);
		break;
	case CControld::CMD_SETBOXTYPE:
		//printf("[controld] set boxtype\n");    //-------------------dummy!!!!!!!!!!
		CControld::commandBoxType msg4;
		CBasicServer::receive_data(connfd, &msg4, sizeof(msg4));
		setBoxType();
		break;
	case CControld::CMD_SETSCARTMODE:
		//printf("[controld] set scartmode\n");
		CControld::commandScartMode msg5;
		CBasicServer::receive_data(connfd, &msg5, sizeof(msg5));
		setScartMode(msg5.mode);
		break;
	case CControld::CMD_SETVIDEOPOWERDOWN:
		//printf("[controld] set scartmode\n");
		CControld::commandVideoPowerSave msg10;
		CBasicServer::receive_data(connfd, &msg10, sizeof(msg10));
		disableVideoOutput(msg10.powerdown);
		break;
		
	case CControld::CMD_GETVOLUME:
	case CControld::CMD_GETVOLUME_AVS:
		CControld::responseVolume msg_responseVolume;
		msg_responseVolume.volume = (rmsg.cmd == CControld::CMD_GETVOLUME) ? settings.volume : settings.volume_avs;
		CBasicServer::send_data(connfd, &msg_responseVolume, sizeof(msg_responseVolume));
		break;

	case CControld::CMD_GETMUTESTATUS:
	case CControld::CMD_GETMUTESTATUS_AVS:
		CControld::responseMute msg_responseMute;
		msg_responseMute.mute = (rmsg.cmd == CControld::CMD_GETMUTESTATUS) ? settings.mute : settings.mute_avs;
		CBasicServer::send_data(connfd, &msg_responseMute, sizeof(msg_responseMute));
		break;

	case CControld::CMD_GETVIDEOFORMAT:
		//printf("[controld] get videoformat (fnc)\n");
		CControld::responseVideoFormat msg8;
		msg8.format = settings.videoformat;
		CBasicServer::send_data(connfd,&msg8,sizeof(msg8));
		break;
	case CControld::CMD_GETASPECTRATIO:
		//printf("[controld] get videoformat (fnc)\n");
		CControld::responseAspectRatio msga;
		if (vcr)
		  msga.aspectRatio = aspectRatio_vcr;
		else
		  msga.aspectRatio = aspectRatio_dvb;
		CBasicServer::send_data(connfd,&msga,sizeof(msga));
		break;
	case CControld::CMD_GETVIDEOOUTPUT:
		//printf("[controld] get videooutput (fblk)\n");
		CControld::responseVideoOutput msg9;
		msg9.output = settings.videooutput;
		CBasicServer::send_data(connfd,&msg9,sizeof(msg9));
		break;
	case CControld::CMD_GETBOXTYPE:
		//printf("[controld] get boxtype\n");
		CControld::responseBoxType msg0;
		msg0.boxtype = settings.boxtype;
		CBasicServer::send_data(connfd,&msg0,sizeof(msg0));
		break;

	case CControld::CMD_SETCSYNC:
		CControld::commandCsync msg11;
		CBasicServer::receive_data(connfd, &msg11, sizeof(msg11));
		setRGBCsync(msg11.csync);
		break;
	case CControld::CMD_GETCSYNC:
		CControld::commandCsync msg12;
		msg12.csync = getRGBCsync();
		CBasicServer::send_data(connfd, &msg12, sizeof(msg12));
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
	return true;
}


void sig_catch(int signal)
{
	switch (signal)
	{
	case SIGHUP:
		saveSettings();
		break;
	default:
		saveSettings();
		exit(EXIT_SUCCESS);
	}
}

void usage(FILE *dest)
{
	fprintf(dest, "controld\n");
	fprintf(dest, "commandline parameters:\n");
	fprintf(dest, "-d, --debug    enable debugging code\n");
	fprintf(dest, "-h, --help     display this text and exit\n\n");
}

int main(int argc, char **argv)
{
	bool debug = false;

	CBasicServer controld_server;

	printf("$Id: controld.cpp,v 1.114 2004/01/01 18:49:49 thegoodguy Exp $\n\n");

	for (int i = 1; i < argc; i++)
	{
		if ((!strncmp(argv[i], "-d", 2)) || (!strncmp(argv[i], "--debug", 7))) {
			debug = true;
		}
		else if ((!strncmp(argv[i], "-h", 2)) || (!strncmp(argv[i], "--help", 6))) {
			usage(stdout);
			return EXIT_SUCCESS;
		}
		else {
			usage(stderr);
			return EXIT_FAILURE;
		}
	}

	if (!controld_server.prepare(CONTROLD_UDS_NAME))
		return -1;

	if (!debug)
	{
		switch (fork())
		{
		case -1:
			perror("[controld] fork");
			return EXIT_FAILURE;
		case 0:
			break;
		default:
			return EXIT_SUCCESS;
		}
	
		if (setsid() == -1)
		{
			perror("[controld] setsid");
			return EXIT_FAILURE;
		}
	}

	eventServer = new CEventServer;

	signal(SIGHUP,sig_catch);
	signal(SIGINT,sig_catch);
	signal(SIGQUIT,sig_catch);
	signal(SIGTERM,sig_catch);

	/* load configuration */
	config = new CConfigFile(',');

	if (!config->loadConfig(CONF_FILE))
	{
		/* set defaults if no configuration file exists */
		printf("[controld] %s not found\n", CONF_FILE);
	}

	settings.volume                = config->getInt32("volume", 100);
	settings.volume_avs            = config->getInt32("volume_avs", 100);
	settings.mute                  = config->getBool("mute", false);
	settings.mute_avs              = config->getBool("mute_avs", false);
	settings.scale_logarithmic     = config->getBool("scale_logarithmic", true);
	settings.scale_logarithmic_avs = config->getBool("scale_logarithmic_avs", true);
	settings.videooutput           = config->getInt32("videooutput", 1); // fblk1 - rgb
	settings.videoformat           = config->getInt32("videoformat", 2); // fnc2 - 4:3
	settings.csync                 = config->getInt32("csync", 0);
	printf("cync %d\n",settings.csync);
	setBoxType(); // dummy set - liest den aktuellen Wert aus!

	watchDog = new CEventWatchDog();
	aspectRatioNotifier = new CControldAspectRatioNotifier();
	watchDog->registerNotifier(WDE_VIDEOMODE, aspectRatioNotifier);

	//init
	audioControl::setVolume(map_volume(settings.volume_avs, true));
	zapit.setVolume(map_volume(settings.volume, false), map_volume(settings.volume, false));
	//lcdd.setVolume(settings.volume_avs);    // we could also display settings.volume at startup

	audioControl::setMute(settings.mute_avs);
	zapit.muteAudio(settings.mute);
	//lcdd.setMute(settings.mute || settings.mute_avs);

	setvideooutput(settings.videooutput);
	setVideoFormat(settings.videoformat, false);

	vcr=false;
	videoOutputDisabled=false;
	
	controld_server.run(parse_command, CControld::ACTVERSION);

	shutdownBox();

	delete aspectRatioNotifier;
	delete watchDog;
	delete config;
	delete eventServer;

	return EXIT_SUCCESS;
}

void CControldAspectRatioNotifier::aspectRatioChanged( int newAspectRatio )
{
	//printf("[controld] CControldAspectRatioNotifier::aspectRatioChanged( %x ) \n", newAspectRatio);
	int activeAspectRatio;
	aspectRatio_dvb = newAspectRatio & 0xFF;
	aspectRatio_vcr = (newAspectRatio & 0xFF00) >> 8;
	if (vcr)
	   activeAspectRatio = aspectRatio_vcr;
	else
	   activeAspectRatio = aspectRatio_dvb;

	if ( settings.videoformat == 0 && (vcr || !videoOutputDisabled))
	{
		switch (activeAspectRatio)
		{
		case 0 :	// 4:3
			setVideoFormat( 2, false );
			break;
		case 1 :	// 16:9
		case 2 :	// 2,21:1
			setVideoFormat( 1, false );
			break;
		default:
			printf("[controld] Unknown aspectRatio: %d", activeAspectRatio);
		}
	}
}
