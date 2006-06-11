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
#define CONFIG_FILE
//#define TRACE
#include <config.h>

#include <fcntl.h>
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
#include <irsend/irsend.h>

#include <fstream>

#include <basicserver.h>
#include <configfile.h>
#include <eventserver.h>
#ifdef USE_LIBTUXBOX
#include <tuxbox.h>
#endif

#include "eventwatchdog.h"
#include "driver/audio.h"


#define CONF_FILE CONFIGDIR "/controld.conf"
#define FORMAT_16_9_FILE CONFIGDIR "/16:9.start"
#define FORMAT_4_3_FILE  CONFIGDIR "/4:3.start"
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
	CControld::video_format vcroutput;
	CControld::video_format videooutput;
	int  videoformat;
	int  csync;
	CControld::volume_type volume_type;
	CControld::tuxbox_maker_t boxtype; // not part of the config - set by setBoxType()
} settings;

// A value that can be put into the AVS-Switch (0..7)
typedef unsigned char switchvalue;

struct tv_format {
  switchvalue cvbs;
  switchvalue rgb;
  switchvalue svideo;
  switchvalue yuv_vbs;
  switchvalue yuv_cvbs;
};

struct vcr_format {
  switchvalue cvbs;
  switchvalue svideo;
};

struct tv_vcr_format {
  vcr_format cvbs;
  vcr_format rgb;
  vcr_format svideo;
  vcr_format yuv_vbs;
  vcr_format yuv_cvbs;
};

struct  avs_settings {
  tv_format v1;
  switchvalue a1;
  tv_vcr_format v2;
  switchvalue a2;
  tv_format v3;
  switchvalue a3;
  switchvalue fblk;
};

struct avs_vendor_settings {
  avs_settings dvb_settings;
  avs_settings scart_settings;
};

avs_vendor_settings current_avs_settings;
char aspectRatio_vcr;
char aspectRatio_dvb;
bool vcr;
bool videoOutputDisabled;

void routeVideo();

void sig_catch(int);

#include "avs_settings.cpp"

#ifdef CONFIG_FILE

#ifdef TRACE
void print_avs_settings(avs_settings settings) {
  printf("{%d %d %d %d %d} %d {{%d %d} {%d %d} {%d %d} {%d %d} {%d %d}} %d {%d %d %d %d %d} %d %d\n",
	 (int) settings.v1.cvbs,
	 (int) settings.v1.rgb,
	 (int) settings.v1.svideo,
	 (int) settings.v1.yuv_vbs,
	 (int) settings.v1.yuv_cvbs,
	 (int) settings.a1,
	 (int) settings.v2.cvbs.cvbs,
	 (int) settings.v2.cvbs.svideo,
	 (int) settings.v2.rgb.cvbs,
	 (int) settings.v2.rgb.svideo,
	 (int) settings.v2.svideo.cvbs,
	 (int) settings.v2.svideo.svideo,
	 (int) settings.v2.yuv_vbs.cvbs,
	 (int) settings.v2.yuv_vbs.svideo,
	 (int) settings.v2.yuv_cvbs.cvbs,
	 (int) settings.v2.yuv_cvbs.svideo,
	 (int) settings.a2,
	 (int) settings.v3.cvbs,
	 (int) settings.v3.rgb,
	 (int) settings.v3.svideo,
	 (int) settings.v3.yuv_vbs,
	 (int) settings.v3.yuv_cvbs,
	 (int) settings.a3,
	 (int) settings.fblk);
}
#endif

void nuke_leading_whitespace(std::string &s) {
  int pos = s.find_first_not_of(" \t");
  s.erase(0, pos);
}

void setup_v1(tv_format &v1, switchvalue cvbs, switchvalue rgb, switchvalue svideo, switchvalue yuv_vbs, switchvalue yuv_cvbs) {
  v1.cvbs = cvbs;
  v1.rgb = rgb;
  v1.svideo = svideo;
  v1.yuv_vbs = yuv_vbs;
  v1.yuv_cvbs = yuv_cvbs;
}

void setup_v1(tv_format &v1, std::string v1_string) {
  int cvbs, rgb, svideo, yuv_vbs, yuv_cvbs;
  sscanf(v1_string.c_str(), "%d %d %d %d %d", &cvbs, &rgb, &svideo, &yuv_vbs, &yuv_cvbs);
  setup_v1(v1, cvbs, rgb, svideo, yuv_vbs, yuv_cvbs);
}

void setup_tv(tv_format &v1, std::string &line) {
  nuke_leading_whitespace(line);
  if (line[0] == '{') {
    int pos = line.find("}");
    setup_v1(v1, line.substr(1, pos-1));
    line.erase(0,pos+1);
  } else {
    int n;
    sscanf(line.c_str(), "%d", &n);
    setup_v1(v1, n, n, n, n, n);
    line.erase(0,1);
  }
}

void setup_v2(tv_vcr_format &v2,
	      switchvalue cvbs_cvbs,
	      switchvalue cvbs_svideo,
	      switchvalue rgb_cvbs,
	      switchvalue rgb_svideo,
	      switchvalue svideo_cvbs,
	      switchvalue svideo_svideo,
	      switchvalue yuv_vbs_cvbs,
	      switchvalue yuv_vbs_svideo,
	      switchvalue yuv_cvbs_cvbs,
	      switchvalue yuv_cvbs_svideo) {	 
  v2.cvbs.cvbs = cvbs_cvbs;
  v2.cvbs.svideo = cvbs_svideo;
  v2.rgb.cvbs = rgb_cvbs;
  v2.rgb.svideo = rgb_svideo;
  v2.svideo.cvbs = svideo_cvbs;
  v2.svideo.svideo = svideo_svideo;
  v2.yuv_vbs.cvbs = yuv_vbs_cvbs;
  v2.yuv_vbs.svideo = yuv_vbs_svideo;
  v2.yuv_cvbs.cvbs = yuv_cvbs_cvbs;
  v2.yuv_cvbs.svideo = yuv_cvbs_svideo;
}

void setup_v2(tv_vcr_format &v2, std::string &line) {
  int cvbs_cvbs;
  int cvbs_svideo;
  int rgb_cvbs;
  int rgb_svideo;
  int svideo_cvbs;
  int svideo_svideo;
  int yuv_vbs_cvbs;
  int yuv_vbs_svideo;
  int yuv_cvbs_cvbs;
  int yuv_cvbs_svideo;

  nuke_leading_whitespace(line);
  sscanf(line.c_str(), "{%d %d} {%d %d} {%d %d} {%d %d} {%d %d}",
	 &cvbs_cvbs, &cvbs_svideo, &rgb_cvbs, &rgb_svideo, 
	 &svideo_cvbs, &svideo_svideo, &yuv_vbs_cvbs, &yuv_vbs_svideo, 
	 &yuv_cvbs_cvbs, &yuv_cvbs_svideo);
  setup_v2(v2, cvbs_cvbs, cvbs_svideo, rgb_cvbs, rgb_svideo,
	   svideo_cvbs, svideo_svideo, yuv_vbs_cvbs, yuv_vbs_svideo,
	   yuv_cvbs_cvbs, yuv_cvbs_svideo);
}

void setup_tv_vcr(tv_vcr_format &v2, std::string &line) {
  nuke_leading_whitespace(line);
  int pos = 0;
  if (line[0] == '{') {
    for (int i = 0; i < 5; i++) {
      pos = line.find("}", pos+1);
    }
    std::string v2_str = line.substr(1, pos);
    setup_v2(v2, v2_str);
    pos = line.find("}", pos+1);
    line.erase(0,pos+1);
  } else {
    int n;
    sscanf(line.c_str(), "%d", &n);
    setup_v2(v2, n, n, n, n, n, n, n, n, n, n);
    line.erase(0,1);
  }
}

void setup_scalar(switchvalue &s, std::string &str) {
  nuke_leading_whitespace(str);
  if (str.length() > 0) { 
    int n;
    sscanf(str.c_str(), "%d", &n);
    s = (switchvalue) n;
  } else
    s = 0;
  str.erase(0,1);
}

void setup_avs_settings(avs_settings &settings, std::string the_line) {
  nuke_leading_whitespace(the_line);
  setup_tv(settings.v1, the_line);
  setup_scalar(settings.a1, the_line);
  setup_tv_vcr(settings.v2, the_line);
  setup_scalar(settings.a2, the_line);
  setup_tv(settings.v3, the_line);
  setup_scalar(settings.a3, the_line);
  setup_scalar(settings.fblk, the_line);
};

bool read_current_avs_settings_from_file() {
  std::ifstream scart_conf(CONFIGDIR "/scart.conf");
  if (! scart_conf.good()) {
    printf("[controld]: Could not open configuration file scart.conf, using defaults\n");
    return false;
  }

  std::string line;
  std::string vendor_string =
    settings.boxtype == CControld::TUXBOX_MAKER_NOKIA ? "nokia" 
    : settings.boxtype == CControld::TUXBOX_MAKER_SAGEM ? "sagem" 
    : "philips";
  while (getline(scart_conf, line)) {
    nuke_leading_whitespace(line);
    if (line[0] != '#') {	// Skip comments
      int s = line.find("_");
      string vendor = line.substr(0, s);
      if (vendor == vendor_string) {
	int s1 = line.find(":");
	if (line.substr(s+1, s1-s-1) == "dvb")
	  setup_avs_settings(current_avs_settings.dvb_settings, line.substr(s1+1));
	if (line.substr(s+1, s1-s-1) == "scart")
	  setup_avs_settings(current_avs_settings.scart_settings, line.substr(s1+1));	
      }
    }
  }
#ifdef TRACE
  print_avs_settings(current_avs_settings.scart_settings);
  print_avs_settings(current_avs_settings.dvb_settings);
#endif  
  return true;
}
#endif // CONFIG_FILE

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

void setvcroutput(CControld::video_format format) {
  if ((format != CControld::FORMAT_CVBS) && (format != CControld::FORMAT_SVIDEO)) {
    printf("[controld] illegal format (=%d) specified for VCR output (using CVBS)!", format);
    format = CControld::FORMAT_CVBS;
  }
  settings.vcroutput = format;
  config->setInt32("vcroutput", settings.vcroutput);
  routeVideo();
}

void setvideooutput(CControld::video_format format, bool bSaveSettings = true)
{
	int fd;
	if ((format < 0) || (format >=  CControld::no_video_formats))
	{
	  	printf("[controld] illegal format (= %d) specified (using CVBS)!", format);
		format=CControld::FORMAT_CVBS;
	}

	if (bSaveSettings) // only set settings if we dont come from watchdog
	{
		settings.videooutput = format;
		config->setInt32("videooutput", settings.videooutput);
	}

	int	arg;

	routeVideo();

	switch (format)
	{
	case CControld::FORMAT_CVBS:
		arg = SAA_MODE_FBAS;
		break;
	case CControld::FORMAT_RGB:
		arg = SAA_MODE_RGB;
		break;
	case CControld::FORMAT_SVIDEO:
		arg = SAA_MODE_SVIDEO;
		break;
	case CControld::FORMAT_YUV_VBS:
		arg = SAA_MODE_YUV_V;
		break;
	case CControld::FORMAT_YUV_CVBS:
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
	
	if(format == CControld::FORMAT_RGB || format == CControld::FORMAT_YUV_VBS || format == CControld::FORMAT_YUV_VBS)
		setRGBCsync(settings.csync);
}

void execute_start_file(const char *filename)
{
  struct stat statbuf;
  if (stat(filename, &statbuf) == 0) {
    printf("[controld] executing %s\n", filename);
    int result = system(filename);
    if (result)
      printf("[controld] %s failed with return code = %d\n", filename, result);
  }
}

void setVideoFormat(int format, bool bSaveFormat = true )
{
        static int last_videoformat = AVS_FNCOUT_EXT43;
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

		if (settings.boxtype == CControld::TUXBOX_MAKER_PHILIPS)
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

	if (format != last_videoformat) {
	  switch (format) {
	  case AVS_FNCOUT_EXT169:
	    {
	      execute_start_file(FORMAT_16_9_FILE);
	      CIRSend irs("16:9");
	      irs.Send();
	    }
	    break;
	  case AVS_FNCOUT_EXT43:
	    {
	      execute_start_file(FORMAT_4_3_FILE);
	      CIRSend irs("4:3");
	      irs.Send();
	    }
	    break;
	  default:
	    break;
	  }
	  last_videoformat = format;
	}
}

void routeVideo(int v1, int a1,
		int v2, int a2,
		int v3, int a3, int fblk)
{
	if (settings.boxtype == CControld::TUXBOX_MAKER_SAGEM)
		printf("[controld]: ROUTEVIDEO v1 = %d a1 = %d v2 = %d a2 = %d fblk=%d\n", v1, a1, v2, a2, fblk);
	else
		printf("[controld]: ROUTEVIDEO v1 = %d a1 = %d v2 = %d a2 = %d v3 = %d a3 = %d fblk=%d\n", v1, a1, v2, a2, v3, a3, fblk);

	int fd = open(AVS_DEVICE, O_RDWR);
	if (fd < 0)
		perror("[controld] " AVS_DEVICE);
	else
        {
		if (ioctl(fd, AVSIOSFBLK, &fblk) < 0)
			perror("[controld] AVSIOSFBLK");

		if (ioctl(fd, AVSIOSVSW1, &v1) < 0)
			perror("[controld] AVSIOSVSW1");

		if (ioctl(fd, AVSIOSASW1, &a1) < 0)
			perror("[controld] AVSIOSASW1");

		if (ioctl(fd, AVSIOSVSW2, &v2) < 0)
			perror("[controld] AVSIOSVSW2");

		if (ioctl(fd, AVSIOSASW2, &a2) < 0)
			perror("[controld] AVSIOSASW2");

		// Sagem does not have v3 and a3, see CXA2126 data sheet
		if (settings.boxtype != CControld::TUXBOX_MAKER_SAGEM) {
			if (ioctl(fd, AVSIOSVSW3, &v3) < 0)
				perror("[controld] AVSIOSVSW3");

			if (ioctl(fd, AVSIOSASW3, &a3) < 0)
				perror("[controld] AVSIOSASW3");
		}
	}

	if (fd != -1)
		close(fd);
}

void routeVideo() {
#ifdef CONFIG_FILE
  read_current_avs_settings_from_file();
#endif
  avs_settings f = vcr ? current_avs_settings.scart_settings : current_avs_settings.dvb_settings;

  switchvalue v1, v3;
  vcr_format vcr_switches;

  switch (settings.videooutput) {
  case CControld::FORMAT_RGB:
    v1 = f.v1.rgb;
    vcr_switches = f.v2.rgb;
    v3 = f.v3.rgb;
    break;
  case CControld::FORMAT_SVIDEO:
    v1 = f.v1.svideo;
    vcr_switches = f.v2.svideo;
    v3 = f.v3.svideo; 
    break;
  case CControld::FORMAT_YUV_VBS:
    v1 = f.v1.yuv_vbs;
    vcr_switches = f.v2.yuv_vbs;
    v3 = f.v3.yuv_vbs; 
    break;
  case CControld::FORMAT_YUV_CVBS:
    v1 = f.v1.yuv_cvbs;
    vcr_switches = f.v2.yuv_cvbs;
    v3 = f.v3.yuv_cvbs; 
    break;
  default:
    // case CControld::FORMAT_CVBS:
    v1 = f.v1.cvbs;
    vcr_switches = f.v2.cvbs;
    v3 = f.v3.cvbs; 
    break;
  };

  switchvalue v2 = settings.vcroutput == CControld::FORMAT_SVIDEO ? vcr_switches.svideo : vcr_switches.cvbs;

  switchvalue fblk = 
    videoOutputDisabled ? 0 
    : vcr ? f.fblk
    : (settings.videooutput == CControld::FORMAT_RGB) ? 1 : 0;

  routeVideo(v1, f.a1, v2, f.a2, v3, f.a3, fblk);
}

void switch_vcr( bool vcr_on)
{
	int activeAspectRatio;
	vcr = vcr_on;
	if (vcr_on)
	{
		//turn to scart-input
	        activeAspectRatio = aspectRatio_vcr;
		printf("[controld]: switch to scart-input... (%d)\n", settings.boxtype);
	}
	else
	{	//turn to dvb...
	 	activeAspectRatio = aspectRatio_dvb;
		printf("[controld]: switch to dvb-input... (%d)\n", settings.boxtype);
	}

	routeVideo();

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
		setvideooutput(CControld::FORMAT_CVBS, false);
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
		settings.boxtype = CControld::TUXBOX_MAKER_SAGEM;
		break;
	case TUXBOX_VENDOR_PHILIPS:
		settings.boxtype = CControld::TUXBOX_MAKER_PHILIPS;
		break;
	case TUXBOX_VENDOR_NOKIA:
		settings.boxtype = CControld::TUXBOX_MAKER_NOKIA;
		break;
	case TUXBOX_VENDOR_DREAM_MM:
		settings.boxtype = CControld::TUXBOX_MAKER_DREAM_MM;
		break;
	case TUXBOX_VENDOR_TECHNOTREND:
		settings.boxtype = CControld::TUXBOX_MAKER_TECHNOTREND;
		break;
	default:
		settings.boxtype = CControld::TUXBOX_MAKER_UNKNOWN;
	}
	// fallback to old way ( via env. var)
	if (settings.boxtype==CControld::TUXBOX_MAKER_UNKNOWN)
	{
#endif
		char * strmID = getenv("mID");

		if (strmID == NULL)
			settings.boxtype = CControld::TUXBOX_MAKER_UNKNOWN;
		else
		{
			int mID = atoi(strmID);

			switch (mID)
			{
			case 3:	
				settings.boxtype= CControld::TUXBOX_MAKER_SAGEM;
				break;
			case 2:	
				settings.boxtype= CControld::TUXBOX_MAKER_PHILIPS;
				break;
			default:
				settings.boxtype= CControld::TUXBOX_MAKER_NOKIA;
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
	const unsigned char invlog63[101]={
	 63, 61, 58, 56, 55, 53, 51, 50, 48, 47, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36,
	 35, 34, 33, 32, 32, 31, 30, 29, 29, 28, 27, 27, 26, 25, 25, 24, 23, 23, 22, 22,
	 21, 21, 20, 20, 19, 19, 18, 18, 17, 17, 16, 16, 15, 15, 15, 14, 14, 13, 13, 13,
	 12, 12, 11, 11, 11, 10, 10, 10,  9,  9,  9,  8,  8,  8,  7,  7,  7,  6,  6,  6,
	  5,  5,  5,  5,  4,  4,  4,  3,  3,  3,  3,  2,  2,  2,  2,  1,  1,  1,  0,  0,
	  0
	};
	const unsigned char log255[101]={	/* "harmonized" -63dB version (same as AVS) */
	143,147,151,155,158,161,164,167,169,172,174,176,179,181,183,185,186,188,190,191,
	193,195,196,198,199,200,202,203,204,205,207,208,209,210,211,212,213,214,215,216,
	217,218,219,220,221,222,223,223,224,225,226,227,227,228,229,230,230,231,232,233,
	233,234,235,235,236,237,237,238,238,239,240,240,241,241,242,243,243,244,244,245,
	245,246,246,247,247,248,248,249,249,250,250,251,251,252,252,253,253,254,254,255,
	255
	};
	if (to_AVS)
	{
		if (volume>100) 
			return invlog63[0];
		else
			return settings.scale_logarithmic_avs ? invlog63[volume] : 63 - ((((unsigned int)volume) * 63) / 100);
	}
	else
	{
		if (volume>100) 
			return log255[0];
		else
			return (volume ? (settings.scale_logarithmic ? log255[volume] : ((((unsigned int)volume) * 255) / 100)) : 0);
	}
}

bool parse_command(CBasicMessage::Header &rmsg, int connfd)
{
	switch (rmsg.cmd)
	{
	case CControldMsg::CMD_SHUTDOWN:
		return false;
		break;
		
	case CControldMsg::CMD_SAVECONFIG:
		saveSettings();
		break;
		
	case CControldMsg::CMD_SETVOLUME:
		CControldMsg::commandVolume msg_commandVolume;
		CBasicServer::receive_data(connfd, &msg_commandVolume, sizeof(msg_commandVolume));
		if (msg_commandVolume.type == CControld::TYPE_UNKNOWN)
			msg_commandVolume.type = settings.volume_type;
		else
			settings.volume_type = msg_commandVolume.type;			
			
		if (msg_commandVolume.type == CControld::TYPE_OST)
		{
			settings.volume = msg_commandVolume.volume;
			config->setInt32("volume", settings.volume);
			zapit.setVolume(map_volume(msg_commandVolume.volume, false), map_volume(msg_commandVolume.volume, false));
		}
		else if (msg_commandVolume.type == CControld::TYPE_AVS)
		{
			settings.volume_avs = msg_commandVolume.volume;
			config->setInt32("volume_avs", settings.volume_avs);
			audioControl::setVolume(map_volume(msg_commandVolume.volume, true));
		}
		else if (msg_commandVolume.type == CControld::TYPE_LIRC)
		{
			if (msg_commandVolume.volume > 50)
			{
				CIRSend irs("volplus");
				irs.Send();
			}
			else if (msg_commandVolume.volume < 50)
			{
				CIRSend irs("volminus");
				irs.Send();
			}
		}
		eventServer->sendEvent(CControldClient::EVT_VOLUMECHANGED, CEventServer::INITID_CONTROLD);
		break;

	case CControldMsg::CMD_SETMUTE:
		CControldMsg::commandMute msg_commandMute;
		CBasicServer::receive_data(connfd, &msg_commandMute, sizeof(msg_commandMute));
		if (msg_commandMute.type == CControld::TYPE_UNKNOWN)
			msg_commandMute.type = settings.volume_type;
		else
			settings.volume_type = msg_commandMute.type;			
			
		if (msg_commandMute.type == CControld::TYPE_OST)
		{
			settings.mute = msg_commandMute.mute;
			config->setBool("mute", settings.mute);
			zapit.muteAudio(settings.mute);
		}
		else if (msg_commandMute.type == CControld::TYPE_AVS)
		{
			settings.mute_avs =  msg_commandMute.mute;
			config->setBool("mute_avs", settings.mute_avs);
			audioControl::setMute(settings.mute_avs);
		}
		else if (msg_commandMute.type == CControld::TYPE_LIRC)
		{
			CIRSend irs("mute");
			irs.Send();
		}
		eventServer->sendEvent(CControldClient::EVT_MUTECHANGED, CEventServer::INITID_CONTROLD, &msg_commandMute, sizeof(msg_commandMute));
		break;

	case CControldMsg::CMD_GETVOLUME:
		CControldMsg::commandVolume msg_responseVolume;
		CBasicServer::receive_data(connfd, &msg_responseVolume, sizeof(msg_responseVolume));
		if (msg_responseVolume.type == CControld::TYPE_UNKNOWN)
			msg_responseVolume.type = settings.volume_type;
		if (msg_responseVolume.type == CControld::TYPE_OST)
			msg_responseVolume.volume = settings.volume;
		else if (msg_responseVolume.type == CControld::TYPE_AVS)
			msg_responseVolume.volume = settings.volume_avs;
		else if (msg_responseVolume.type == CControld::TYPE_LIRC)
			msg_responseVolume.volume = 50; //we donnot really know...
		CBasicServer::send_data(connfd, &msg_responseVolume, sizeof(msg_responseVolume));
		break;

	case CControldMsg::CMD_GETMUTESTATUS:
		CControldMsg::commandMute msg_responseMute;
		CBasicServer::receive_data(connfd, &msg_responseMute, sizeof(msg_responseMute));
		if (msg_responseMute.type == CControld::TYPE_UNKNOWN)
			msg_responseMute.type = settings.volume_type;
		if (msg_responseMute.type == CControld::TYPE_OST)
			msg_responseMute.mute = settings.mute;
		else if (msg_responseMute.type == CControld::TYPE_AVS)
			msg_responseMute.mute = settings.mute_avs;
		else if (msg_responseMute.type == CControld::TYPE_LIRC)
			msg_responseMute.mute = false; //we donnot really know...
		CBasicServer::send_data(connfd, &msg_responseMute, sizeof(msg_responseMute));
		break;

	case CControldMsg::CMD_SETVIDEOFORMAT:
		//printf("[controld] set videoformat\n");
		CControldMsg::commandVideoFormat msg2;
		CBasicServer::receive_data(connfd, &msg2, sizeof(msg2));
		setVideoFormat(msg2.format);
		break;
	case CControldMsg::CMD_SETVIDEOOUTPUT:
		//printf("[controld] set videooutput\n");
		CControldMsg::commandVideoOutput msg3;
		CBasicServer::receive_data(connfd, &msg3, sizeof(msg3));
		setvideooutput((CControld::video_format)msg3.output);
		break;
	case CControldMsg::CMD_SETBOXTYPE:
		//printf("[controld] set boxtype\n");    //-------------------dummy!!!!!!!!!!
		CControldMsg::commandBoxType msg4;
		CBasicServer::receive_data(connfd, &msg4, sizeof(msg4));
		setBoxType();
		break;
	case CControldMsg::CMD_SETSCARTMODE:
		//printf("[controld] set scartmode\n");
		CControldMsg::commandScartMode msg5;
		CBasicServer::receive_data(connfd, &msg5, sizeof(msg5));
		setScartMode(msg5.mode);
		break;
	case CControldMsg::CMD_GETSCARTMODE:
		//printf("[controld] get scartmode\n");
		CControldMsg::commandScartMode msg51;
		msg51.mode = vcr;
		CBasicServer::send_data(connfd, &msg51, sizeof(CControldMsg::responseScartMode));
		break;
	case CControldMsg::CMD_SETVIDEOPOWERDOWN:
		//printf("[controld] set scartmode\n");
		CControldMsg::commandVideoPowerSave msg10;
		CBasicServer::receive_data(connfd, &msg10, sizeof(msg10));
		disableVideoOutput(msg10.powerdown);
		break;
		
	case CControldMsg::CMD_GETVIDEOFORMAT:
		//printf("[controld] get videoformat (fnc)\n");
		CControldMsg::responseVideoFormat msg8;
		msg8.format = settings.videoformat;
		CBasicServer::send_data(connfd,&msg8,sizeof(msg8));
		break;
	case CControldMsg::CMD_GETASPECTRATIO:
		//printf("[controld] get videoformat (fnc)\n");
		CControldMsg::responseAspectRatio msga;
		if (vcr)
		  msga.aspectRatio = aspectRatio_vcr;
		else
		  msga.aspectRatio = aspectRatio_dvb;
		CBasicServer::send_data(connfd,&msga,sizeof(msga));
		break;
	case CControldMsg::CMD_GETVIDEOOUTPUT:
		//printf("[controld] get videooutput (fblk)\n");
		CControldMsg::responseVideoOutput msg9;
		msg9.output = settings.videooutput;
		CBasicServer::send_data(connfd,&msg9,sizeof(msg9));
		break;
	case CControldMsg::CMD_GETBOXTYPE:
		//printf("[controld] get boxtype\n");
		CControldMsg::responseBoxType msg0;
		msg0.boxtype = settings.boxtype;
		CBasicServer::send_data(connfd,&msg0,sizeof(msg0));
		break;

	case CControldMsg::CMD_SETCSYNC:
		CControldMsg::commandCsync msg11;
		CBasicServer::receive_data(connfd, &msg11, sizeof(msg11));
		setRGBCsync(msg11.csync);
		break;
	case CControldMsg::CMD_GETCSYNC:
		CControldMsg::commandCsync msg12;
		msg12.csync = getRGBCsync();
		CBasicServer::send_data(connfd, &msg12, sizeof(msg12));
		break;

	case CControldMsg::CMD_REGISTEREVENT:
		eventServer->registerEvent(connfd);
		break;
	case CControldMsg::CMD_UNREGISTEREVENT:
		eventServer->unRegisterEvent(connfd);
		break;

	case CControldMsg::CMD_SETVCROUTPUT:
	        //printf("[controld] set vcroutput\n");
		CControldMsg::commandVCROutput msg13;
		CBasicServer::receive_data(connfd, &msg13, sizeof(msg13));
		setvcroutput((CControld::video_format)msg13.vcr_output);
		break;
	case CControldMsg::CMD_GETVCROUTPUT:
	        //printf("[controld] get vcroutput\n");
		CControldMsg::responseVCROutput msg14;
		msg14.vcr_output = settings.vcroutput;
		CBasicServer::send_data(connfd,&msg14,sizeof(msg14));
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

	printf("$Id: controld.cpp,v 1.121 2006/06/11 12:18:17 barf Exp $\n\n");

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
	settings.vcroutput             = (CControld::video_format) config->getInt32("vcroutput", CControld::FORMAT_CVBS);
	settings.videooutput           = (CControld::video_format) config->getInt32("videooutput", CControld::FORMAT_RGB);
	settings.videoformat           = config->getInt32("videoformat", 2); // fnc2 - 4:3
	settings.csync                 = config->getInt32("csync", 0);
	settings.volume_type           = (CControld::volume_type) config->getInt32("volume_type", CControld::TYPE_OST);
	setBoxType(); // dummy set - liest den aktuellen Wert aus!

	watchDog = new CEventWatchDog();
	aspectRatioNotifier = new CControldAspectRatioNotifier();
	watchDog->registerNotifier(WDE_VIDEOMODE, aspectRatioNotifier);

	//init
	current_avs_settings = 
	  settings.boxtype == CControld::TUXBOX_MAKER_NOKIA ? nokia_settings :
	  settings.boxtype == CControld::TUXBOX_MAKER_SAGEM ? sagem_settings :
	  philips_settings;

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
	
	controld_server.run(parse_command, CControldMsg::ACTVERSION);

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
