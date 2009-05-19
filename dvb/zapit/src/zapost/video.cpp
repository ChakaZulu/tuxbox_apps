/*
 * $Id: video.cpp,v 1.16 2009/05/19 18:27:53 seife Exp $
 *
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <zapit/zapit.h>
#include <zapit/debug.h>
#include <zapit/settings.h>
#include <zapit/video.h>
#if defined HAVE_DBOX_HARDWARE || defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
#include <dbox/avs_core.h>
#include <dbox/saa7126_core.h>
#endif

extern struct Ssettings settings;

CVideo::CVideo(void)
{
	if ((fd = open(VIDEO_DEVICE, O_RDWR)) < 0)
		ERROR(VIDEO_DEVICE);

#ifdef HAVE_DBOX_HARDWARE
	/* setBlank is not _needed_ on the Dreambox. I don't know about the
	   dbox, so i ifdef'd it out. Not blanking the fb leaves the bootlogo
	   on screen until the video starts. --seife */
	setBlank(true);
#endif
}

CVideo::~CVideo(void)
{
	if (fd >= 0)
		close(fd);
}

int CVideo::setAspectRatio(video_format_t format)
{
	return fop(ioctl, VIDEO_SET_FORMAT, format);
}

video_format_t CVideo::getAspectRatio(void)
{
	struct video_status status;
	fop(ioctl, VIDEO_GET_STATUS, &status);
#if HAVE_DVB_API_VERSION < 3
	return status.videoFormat;
#else
	return status.video_format;
#endif
}

int CVideo::setCroppingMode(video_displayformat_t format)
{
	const char *format_string[] = { "panscan", "letterbox", "center_cutout", "unknown" };
	DBG("setting cropping mode to %s", format_string[format]);
	return fop(ioctl, VIDEO_SET_DISPLAY_FORMAT, format);
}

video_displayformat_t CVideo::getCroppingMode(void)
{
	struct video_status status;
	fop(ioctl, VIDEO_GET_STATUS, &status);
#if HAVE_DVB_API_VERSION < 3
	return status.displayFormat;
#else
	return status.display_format;
#endif
}

/* set the format:
   0 auto
   1 16:9
   2 4:3(LB)
   3 4:3(PS)
   format == -1 disables 12V on SCART pin 8 */
void CVideo::setVideoFormat(int format)
{
#if defined HAVE_DREAMBOX_HARDWARE || defined HAVE_DBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
	video_displayformat_t videoDisplayFormat;
	int _fd;
	int avsiosfncFormat;
	int wss;

	const char *format_string[] = { "auto", "16:9", "4:3(LB)", "4:3(PS)" };
	/*
	  16:9 : fnc 1
	  4:3  : fnc 2
	*/

	if (format == 0) // automatic switch
	{
		DBG("setting VideoFormat to auto");
		int activeAspectRatio;
		if (settings.vcr)
			activeAspectRatio = settings.aspectRatio_vcr;
		else
			activeAspectRatio = settings.aspectRatio_dvb;

		switch (activeAspectRatio)
		{
		case 0:	// 4:3
			format= 2;
			break;
		case 1:	// 16:9
		case 2:	// 21,1:1
			format = 1;
			break;
		default:
			format = 2;
			// damits nicht ausgeht beim starten :)
		}
	}

	printf("[CVideo::%s] output format: ", __FUNCTION__);
	if (format >= 0 && format <= 3)
		printf("%s\n", format_string[format]);
	else
		printf("unknown (%d)\n", format);

	if ((_fd = open(AVS_DEVICE, O_RDWR)) < 0)
		perror("[CVideo] " AVS_DEVICE);
	else
	{
		if (format < 0)
			format = 0;

		avsiosfncFormat = format;

		if (settings.boxtype == CControld::TUXBOX_MAKER_PHILIPS)
		{
			switch (format)
			{
			case 1:
				avsiosfncFormat = 2;
				break;
			case 2:
				avsiosfncFormat = 3;
				break;
			}
		}

		if (ioctl(_fd, AVSIOSFNC, &avsiosfncFormat) < 0)
			perror("[CVideo] AVSIOSFNC");

		close(_fd);
	}

	switch (format)
	{
		//	?	case AVS_FNCOUT_INTTV	: videoDisplayFormat = VIDEO_PAN_SCAN;
	case AVS_FNCOUT_EXT169:
		videoDisplayFormat = VIDEO_CENTER_CUT_OUT;
		wss = SAA_WSS_169F;
		break;
	case AVS_FNCOUT_EXT43:
		videoDisplayFormat = VIDEO_LETTER_BOX;
		wss = SAA_WSS_43F;
		break;
	case AVS_FNCOUT_EXT43_1:
		videoDisplayFormat = VIDEO_PAN_SCAN;
		wss = SAA_WSS_43F;
		break;
	default:
		videoDisplayFormat = VIDEO_LETTER_BOX;
		wss = SAA_WSS_43F;
		break;
	}
	setCroppingMode(videoDisplayFormat);

	if ((_fd = open(SAA7126_DEVICE, O_RDWR)) < 0)
		perror("[CVideo] " SAA7126_DEVICE);
	else {
		if (ioctl(_fd, SAAIOSWSS, &wss) < 0)
			perror("[CVideo] SAAIOSWSS");

		close(_fd);
	}

#if 0	/* todo: fix. probably outside of video.cpp. */
	static int last_videoformat = AVS_FNCOUT_EXT43;
	if (format != last_videoformat) {
		switch (format) {
		case AVS_FNCOUT_EXT169:
		{
			execute_start_file(FORMAT_16_9_FILE);
			CIRSend irs("16:9");
			irs.Send();
			last_videoformat = format;
			break;
		}
		case AVS_FNCOUT_EXT43:
		{
			execute_start_file(FORMAT_4_3_FILE);
			CIRSend irs("4:3");
			irs.Send();
			last_videoformat = format;
			break;
		}
		default:
			break;
		}
	}
#endif
#endif
}

int CVideo::setSource(video_stream_source_t source)
{
	return fop(ioctl, VIDEO_SELECT_SOURCE, source);
}

video_stream_source_t CVideo::getSource(void)
{
	struct video_status status;
	fop(ioctl, VIDEO_GET_STATUS, &status);
#if HAVE_DVB_API_VERSION < 3
	return status.streamSource;
#else
	return status.stream_source;
#endif
}

int CVideo::start(void)
{
	return fop(ioctl, VIDEO_PLAY);
}

int CVideo::stop(void)
{
	return fop(ioctl, VIDEO_STOP);
}

int CVideo::setBlank(int enable)
{
	return fop(ioctl, VIDEO_SET_BLANK, enable);
}

int CVideo::getBlank(void)
{
	struct video_status status;
	fop(ioctl, VIDEO_GET_STATUS, &status);
#if HAVE_DVB_API_VERSION < 3
	return status.videoBlank;
#else
	return status.video_blank;
#endif
}

video_play_state_t CVideo::getPlayState(void)
{
	struct video_status status;
	fop(ioctl, VIDEO_GET_STATUS, &status);
#if HAVE_DVB_API_VERSION < 3
	return status.playState;
#else
	return status.play_state;
#endif
}

int CVideo::setVideoSystem(int video_system)
{
        return fop(ioctl, VIDEO_SET_SYSTEM, video_system);
}

