/*
  Control-Daemon  -   DBoxII-Project

  Copyright (C) 2001 Steffen Hehn 'McClean',
  2002 dboxII-team,
  Copyright (C) 2007 Stefan Seyfried
	
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
#ifdef HAVE_DBOX_HARDWARE
#define USE_LIBTUXBOX 1
#endif
#define CONFIG_FILE
//#define TRACE

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

#include <zapit/audio.h>
#include <zapit/channel.h>

#include <zapit/client/zapitclient.h>

#include <controldclient/controldclient.h>
#include <controldclient/controldMsg.h>
#include "irsend/irsend.h"

#include <fstream>

#include <basicserver.h>
#include <configfile.h>
#include <eventserver.h>
#ifdef USE_LIBTUXBOX
#include <tuxbox.h>
#endif

#include "controld.h"
#include "eventwatchdog.h"

#include <zapit/video.h>

#define CONF_FILE CONFIGDIR "/controld.conf"
#define FORMAT_16_9_FILE CONFIGDIR "/16:9.start"
#define FORMAT_4_3_FILE  CONFIGDIR "/4:3.start"
#define AVS_DEVICE	"/dev/dbox/avs0"
#define SAA7126_DEVICE	"/dev/dbox/saa0"

extern CAudio *audioDecoder;
extern CVideo *videoDecoder;
extern CZapitChannel *cc;

// variables
struct Ssettings settings;
/* the configuration file */
CConfigFile * controldconfig = NULL;

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
// char aspectRatio_vcr;
// char aspectRatio_dvb;
// bool vcr;
// bool videoOutputDisabled;

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

void controldSaveSettings()
{
	/* does not really belong here? */
	controldconfig->setInt32("volume_type", settings.volume_type);
	controldconfig->saveConfig(CONF_FILE);
}

void shutdownBox()
{
	controldSaveSettings();
}

#ifdef HAVE_DBOX_HARDWARE
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
	controldconfig->setInt32("csync", settings.csync);
}
#else
void setRGBCsync(int)
{
	fprintf(stderr, "[controld] SAAIOSCSYNC only implemented on dbox\n");
}
#endif

char getRGBCsync()
{
#ifdef HAVE_DBOX_HARDWARE
	int fd, val=0;
	if ((fd = open(SAA7126_DEVICE, O_RDWR|O_NONBLOCK)) < 0)
		perror("[controld] " SAA7126_DEVICE);

	else {
		if ((ioctl(fd, SAAIOGCSYNC, &val) < 0))
			perror("[controld] SAAIOGCSYNC");
		
		close(fd);
	}
	return val;
#else
	fprintf(stderr, "[controld] SAAIOGCSYNC only implemented on dbox\n");
	return 0;
#endif
}

void setvcroutput(CControld::video_format format) {
  if ((format != CControld::FORMAT_CVBS) && (format != CControld::FORMAT_SVIDEO)) {
    printf("[controld] illegal format (=%d) specified for VCR output (using CVBS)!", format);
    format = CControld::FORMAT_CVBS;
  }
  settings.vcroutput = format;
  controldconfig->setInt32("vcroutput", settings.vcroutput);
  routeVideo();
}

void setvideooutput(CControld::video_format format, bool bSaveSettings)
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
		controldconfig->setInt32("videooutput", settings.videooutput);
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
#if defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
	case CControld::FORMAT_YUV_VBS:
	case CControld::FORMAT_YUV_CVBS:
		fprintf(stderr, "[controld] FORMAT_YUV_VBS/FORMAT_YUV_CVBS not supported on dreambox\n");
		return;
		break;
#else
	case CControld::FORMAT_YUV_VBS:
		arg = SAA_MODE_YUV_V;
		break;
	case CControld::FORMAT_YUV_CVBS:
		arg = SAA_MODE_YUV_C;
		break;
#endif
	}

	if ((fd = open(SAA7126_DEVICE, O_RDWR|O_NONBLOCK)) < 0)
		perror("[controld] " SAA7126_DEVICE);

	else {
		if ((ioctl(fd, SAAIOSMODE, &arg) < 0))
			perror("[controld] SAAIOSMODE");
		
		close(fd);
	}
	
#if defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
	if(format == CControld::FORMAT_RGB || format == CControld::FORMAT_YUV_VBS || format == CControld::FORMAT_YUV_VBS)
		setRGBCsync(settings.csync);
#endif
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

		// Sagem does not have v3, see CXA2126 data sheet
		if (settings.boxtype != CControld::TUXBOX_MAKER_SAGEM)
			if (ioctl(fd, AVSIOSVSW3, &v3) < 0)
				perror("[controld] AVSIOSVSW3");

		// Only Nokia has a3
		if (settings.boxtype == CControld::TUXBOX_MAKER_NOKIA)
			if (ioctl(fd, AVSIOSASW3, &a3) < 0)
				perror("[controld] AVSIOSASW3");
	}

	if (fd != -1)
		close(fd);
}

void routeVideo() {
#ifdef CONFIG_FILE
  read_current_avs_settings_from_file();
#endif
  avs_settings f = settings.vcr ? current_avs_settings.scart_settings : current_avs_settings.dvb_settings;

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
    settings.videoOutputDisabled ? 0
    : settings.vcr ? f.fblk
    : (settings.videooutput == CControld::FORMAT_RGB) ? 1 : 0;

  routeVideo(v1, f.a1, v2, f.a2, v3, f.a3, fblk);
}

void switch_vcr( bool vcr_on)
{
	int activeAspectRatio;
	settings.vcr = vcr_on;
	if (vcr_on)
	{
		//turn to scart-input
		activeAspectRatio = settings.aspectRatio_vcr;
		disableVideoOutput(false);
		printf("[controld]: switch to scart-input... (%d)\n", settings.boxtype);
	}
	else
	{	//turn to dvb...
	 	activeAspectRatio = settings.aspectRatio_dvb;
		printf("[controld]: switch to dvb-input... (%d)\n", settings.boxtype);
	}

	routeVideo();

	// recall AspectRatio when switching between DVB and VCR
	if ( settings.videoformat == 0 )
	{
		switch (activeAspectRatio)
		{
		case 0 :	// 4:3
			videoDecoder->setVideoFormat(2);
			break;
		case 1 :	// 16:9
		case 2 :	// 2,21:1
			videoDecoder->setVideoFormat(1);
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
		/* we might need to mute / unmute the AVS */
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
	settings.videoOutputDisabled = disable;
	printf("[controld] videoOutput %s\n", disable?"off":"on");

#ifdef HAVE_DBOX_HARDWARE
	int arg=disable?1:0;
	int fd;
	if ((fd = open(SAA7126_DEVICE,O_RDWR|O_NONBLOCK)) < 0)
		perror("[controld] " SAA7126_DEVICE);

	else 
	{
		if ((ioctl(fd,SAAIOSPOWERSAVE,&arg) < 0))
			perror("[controld] SAAIOSPOWERSAVE");

		close(fd);
	}
#endif

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
		audioDecoder->unmute();
#if defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
		startPlayBack(cc);
#endif
		setvideooutput(settings.videooutput, false);
		videoDecoder->setVideoFormat(settings.videoformat);
	}
	else
	{
		setvideooutput(CControld::FORMAT_CVBS, false);
		videoDecoder->setVideoFormat(-1);
		//zapit.setStandby(true);
		audioDecoder->mute();
#if defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
		stopPlayBack();
#endif
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
		const char * maker_str[] = {
			"unknown",
			"Nokia",
			"Philips",
			"Sagem",
			"Dream Multimedia",
			"Technotrend"
		 };

		char * strmID = getenv("mID");

		if (strmID == NULL)
			settings.boxtype = CControld::TUXBOX_MAKER_UNKNOWN;
		else
		{
			int mID = atoi(strmID);

			switch (mID)
			{
			case 5 ... 9:	// Dreambox
			case 11:
			case 12:
				settings.boxtype= CControld::TUXBOX_MAKER_DREAM_MM;
				break;
			case 3:	
				settings.boxtype= CControld::TUXBOX_MAKER_SAGEM;
				break;
			case 2:	
				settings.boxtype= CControld::TUXBOX_MAKER_PHILIPS;
				break;
			case 1:
				settings.boxtype= CControld::TUXBOX_MAKER_NOKIA;
				break;
			default:
				settings.boxtype = CControld::TUXBOX_MAKER_UNKNOWN;
				printf("[controld] ERROR: unknown mID %d (%s)\n", mID, strmID);
			}
		}
		printf("[controld] Boxtype detected from mID: %s (%d)\n", maker_str[settings.boxtype], settings.boxtype);
#ifdef USE_LIBTUXBOX
	}
	else
		printf("[controld] Boxtype detected: (%d, %s %s)\n", settings.boxtype, tuxbox_get_vendor_str(), tuxbox_get_model_str());
#endif
}

void controld_main(void)
{
	/* load configuration */
	controldconfig = new CConfigFile(',');

	if (!controldconfig->loadConfig(CONF_FILE))
	{
		/* set defaults if no configuration file exists */
		printf("[controld] %s not found\n", CONF_FILE);
	}

	settings.volume                = controldconfig->getInt32("volume", 100);
	settings.volume_avs            = controldconfig->getInt32("volume_avs", 100);
	settings.mute                  = controldconfig->getBool("mute", false);
//	settings.mute_avs              = controldconfig->getBool("mute_avs", false);
	settings.scale_logarithmic     = controldconfig->getBool("scale_logarithmic", true);
//	settings.scale_logarithmic_avs = controldconfig->getBool("scale_logarithmic_avs", true);
	settings.vcroutput             = (CControld::video_format) controldconfig->getInt32("vcroutput", CControld::FORMAT_CVBS);
	settings.videooutput           = (CControld::video_format) controldconfig->getInt32("videooutput", CControld::FORMAT_RGB);
	settings.videoformat           = controldconfig->getInt32("videoformat", 2); // fnc2 - 4:3
	settings.csync                 = controldconfig->getInt32("csync", 0);
	settings.volume_type           = (CControld::volume_type) controldconfig->getInt32("volume_type", CControld::TYPE_OST);
	setBoxType(); // dummy set - liest den aktuellen Wert aus!

	watchDog = new CEventWatchDog();
	aspectRatioNotifier = new CControldAspectRatioNotifier();
	watchDog->registerNotifier(WDE_VIDEOMODE, aspectRatioNotifier);

	//init
	current_avs_settings = 
		settings.boxtype == CControld::TUXBOX_MAKER_NOKIA ? nokia_settings :
		settings.boxtype == CControld::TUXBOX_MAKER_SAGEM ? sagem_settings :
		philips_settings;

	setvideooutput(settings.videooutput);

	settings.vcr = false;
	settings.videoOutputDisabled = false;
}

void controld_end()
{
	shutdownBox();

	delete aspectRatioNotifier;
	delete watchDog;
	delete controldconfig;
}

void CControldAspectRatioNotifier::aspectRatioChanged( int newAspectRatio )
{
	//printf("[controld] CControldAspectRatioNotifier::aspectRatioChanged( %x ) \n", newAspectRatio);
	/* the videodecoder gets initialized after the watchdog thread is started */
	if (!videoDecoder)
		return;
	int activeAspectRatio;
	settings.aspectRatio_dvb = newAspectRatio & 0xFF;
	settings.aspectRatio_vcr = (newAspectRatio & 0xFF00) >> 8;
	if (settings.vcr)
		activeAspectRatio = settings.aspectRatio_vcr;
	else
		activeAspectRatio = settings.aspectRatio_dvb;

	if (settings.videoformat == 0 && (settings.vcr || !settings.videoOutputDisabled))
	{
		switch (activeAspectRatio)
		{
		case 0 :	// 4:3
			videoDecoder->setVideoFormat(2);
			break;
		case 1 :	// 16:9
		case 2 :	// 2,21:1
			videoDecoder->setVideoFormat(1);
			break;
		default:
			printf("[controld] Unknown aspectRatio: %d", activeAspectRatio);
		}
	}
#if HAVE_DVB_API_VERSION < 3
	else
		videoDecoder->setVideoFormat(settings.videoformat);
#endif
}
