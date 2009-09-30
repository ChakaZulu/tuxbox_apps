/*
 * $Id: video.cpp,v 1.17 2009/09/30 17:47:10 seife Exp $
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

#ifdef HAVE_TRIPLEDRAGON
#include <avs/avs_inf.h>
#endif

extern struct Ssettings settings;

CVideo::CVideo(void)
{
	if ((fd = open(VIDEO_DEVICE, O_RDWR)) < 0)
		ERROR(VIDEO_DEVICE);
#ifdef HAVE_TRIPLEDRAGON
	playstate = VIDEO_STOPPED;
	croppingMode = VID_DISPMODE_NORM;
	z[0] = 100;
	z[1] = 100;
	zoomvalue = &z[0];
	const char *blanknames[2] = { "/share/tuxbox/blank_576.mpg", "/share/tuxbox/blank_480.mpg" };
	int blankfd;
	struct stat st;

	for (int i = 0; i < 2; i++)
	{
		blank_data[i] = NULL; /* initialize */
		blank_size[i] = 0;
		blankfd = open(blanknames[i], O_RDONLY);
		if (blankfd < 0)
		{
			WARN("cannot open %s: %m", blanknames[i]);
			continue;
		}
		if (fstat(blankfd, &st) != -1 && st.st_size > 0)
		{
			blank_size[i] = st.st_size;
			blank_data[i] = malloc(blank_size[i]);
			if (! blank_data[i])
				ERROR("cannot malloc memory");
			else if (read(blankfd, blank_data[i], blank_size[i]) != blank_size[i])
			{
				ERROR("short read");
				free(blank_data[i]); /* don't leak... */
				blank_data[i] = NULL;
			}
		}
		close(blankfd);
	}
#endif

#ifdef HAVE_DBOX_HARDWARE
	/* setBlank is not _needed_ on the Dreambox. I don't know about the
	   dbox, so i ifdef'd it out. Not blanking the fb leaves the bootlogo
	   on screen until the video starts. --seife */
	setBlank(true);
#endif
}

CVideo::~CVideo(void)
{
#ifdef HAVE_TRIPLEDRAGON
	playstate = VIDEO_STOPPED;
	for (int i = 0; i < 2; i++)
	{
		if (blank_data[i])
			free(blank_data[i]);
		blank_data[i] = NULL;
	}
#endif
	if (fd >= 0)
		close(fd);
}

int CVideo::setAspectRatio(video_format_t format)
{
	return fop(ioctl, VIDEO_SET_FORMAT, format);
}

#ifndef HAVE_TRIPLEDRAGON
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
#else
video_format_t CVideo::getAspectRatio(void)
{
	VIDEOINFO v;
	/* this memset silences *TONS* of valgrind warnings */
	memset(&v, 0, sizeof(v));
	quiet_fop(ioctl, MPEG_VID_GET_V_INFO, &v);
	if (v.pel_aspect_ratio < VID_DISPSIZE_4x3 || v.pel_aspect_ratio > VID_DISPSIZE_UNKNOWN)
	{
		WARN("invalid value %d, returning VID_DISPSIZE_UNKNOWN fd: %d", v.pel_aspect_ratio, fd);
		return VID_DISPSIZE_UNKNOWN;
	}
	return v.pel_aspect_ratio;
}
#endif

int CVideo::setCroppingMode(video_displayformat_t format)
{
#ifdef HAVE_TRIPLEDRAGON
	croppingMode = format;
#endif
	const char *format_string[] = { "panscan", "letterbox", "center_cutout", "unknown" };
	DBG("setting cropping mode to %s", format_string[format]);
	return fop(ioctl, VIDEO_SET_DISPLAY_FORMAT, format);
}

#ifndef HAVE_TRIPLEDRAGON
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
#else
video_displayformat_t CVideo::getCroppingMode(void)
{
	return croppingMode;
}
#endif

/* set the format:
   0 auto
   1 16:9
   2 4:3(LB)
   3 4:3(PS)
   format == -1 disables 12V on SCART pin 8 */
#ifdef HAVE_GENERIC_HARDWARE
/* not implemented :-( */
void CVideo::setVideoFormat(int)
{
}
#else
void CVideo::setVideoFormat(int format)
{
#if defined HAVE_DREAMBOX_HARDWARE || defined HAVE_DBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
	video_displayformat_t videoDisplayFormat;
	int avsiosfncFormat;
	int wss;
#endif
#ifdef HAVE_TRIPLEDRAGON
	vidDispSize_t dispsize;
	vidDispMode_t videoDisplayFormat;
#endif

	int _fd;
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

#ifndef HAVE_TRIPLEDRAGON
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
#else
	unsigned int volt = 12;
	switch(format)
	{
	case 1:	/* 16:9 */
		videoDisplayFormat = VID_DISPMODE_NORM;
		dispsize = VID_DISPSIZE_16x9;
		volt = 6;
		break;
	case 3:	/* 4:3 PS */
		videoDisplayFormat = VID_DISPMODE_NORM;
		dispsize = VID_DISPSIZE_4x3;
		break;
	default:
		volt = 0;
		// fallthrough
	case 2: /* 4:3 LB */
		videoDisplayFormat = VID_DISPMODE_LETTERBOX;
		dispsize = VID_DISPSIZE_4x3;
		break;
	}
	if (fop(ioctl, MPEG_VID_SET_DISPSIZE, dispsize) < 0)
		perror("[CVideo] MPEG_VID_SET_DISPSIZE");

	if ((_fd = open(AVS_DEVICE, O_RDWR)) < 0)
		perror("[CVideo] " AVS_DEVICE);
	else
	{
		DBG("setting PIN 8 to %dV", volt);
		if (ioctl(_fd, IOC_AVS_SCART_PIN8_SET, volt) < 0)
			WARN("IOC_AVS_SCART_PIN8_SET: %m");
		close(_fd);
	}
	setCroppingMode(videoDisplayFormat);
	if (*zoomvalue != 100)
		setZoom(*zoomvalue);
#endif

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
}
#endif

int CVideo::setSource(video_stream_source_t source)
{
	return quiet_fop(ioctl, VIDEO_SELECT_SOURCE, source);
}

#ifndef HAVE_TRIPLEDRAGON
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
#endif

int CVideo::start(void)
{
#ifdef HAVE_TRIPLEDRAGON
	if (playstate == VIDEO_PLAYING)
		return 0;
	playstate = VIDEO_PLAYING;
	fop(ioctl, MPEG_VID_PLAY);
	return fop(ioctl, MPEG_VID_SYNC_ON, VID_SYNC_AUD);
#else
	return fop(ioctl, VIDEO_PLAY);
#endif
}

int CVideo::stop(void)
{
#ifdef HAVE_TRIPLEDRAGON
	playstate = VIDEO_STOPPED;
#endif
	return fop(ioctl, VIDEO_STOP);
}

#ifndef HAVE_TRIPLEDRAGON
int CVideo::setBlank(int enable)
{
	return fop(ioctl, VIDEO_SET_BLANK, enable);
}
#else
int CVideo::setBlank(int)
{
	/* The TripleDragon has no VIDEO_SET_BLANK ioctl.
	   instead, you write a black still-MPEG Iframe into the decoder.
	   The original software uses different files for 4:3 and 16:9 and
	   for PAL and NTSC. I optimized that a little bit
	 */
	int index = 0; /* default PAL */
	VIDEOINFO v;
	BUFINFO buf;
	memset(&v, 0, sizeof(v));
	quiet_fop(ioctl, MPEG_VID_GET_V_INFO, &v);

	if ((v.v_size % 240) == 0) /* NTSC */
	{
		INFO("NTSC format detected");
		index = 1;
	}

	if (blank_data[index] == NULL) /* no MPEG found */
		return -1;

	/* hack: this might work only on those two still-MPEG files!
	   I diff'ed the 4:3 and the 16:9 still mpeg from the original
	   soft and spotted the single bit difference, so there is no
	   need to keep two different MPEGs in memory
	   If we would read them from disk all the time it would be
	   slower and it might wake up the drive occasionally */
	if (v.pel_aspect_ratio == VID_DISPSIZE_4x3)
		((char *)blank_data[index])[7] &= ~0x10; // clear the bit
	else
		((char *)blank_data[index])[7] |=  0x10; // set the bit

	//WARN("blank[7] == 0x%02x", ((char *)blank_data[index])[7]);

	buf.ulLen = blank_size[index];
	buf.ulStartAdrOff = (int)blank_data[index];
	return fop(ioctl, MPEG_VID_STILLP_WRITE, &buf);
}
#endif

int CVideo::setVideoSystem(int video_system)
{
        return fop(ioctl, VIDEO_SET_SYSTEM, video_system);
}

#ifndef HAVE_TRIPLEDRAGON
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
#endif

#ifdef HAVE_TRIPLEDRAGON
video_play_state_t CVideo::getPlayState(void)
{
	return playstate;
}

int CVideo::setVideoOutput(vidOutFmt_t arg)
{
	return fop(ioctl, MPEG_VID_SET_OUTFMT, arg);
}

/* set zoom in percent (100% == 1:1) */
int CVideo::setZoom(int zoom)
{
	if (zoom == -1) // "auto" reset
		zoom = *zoomvalue;

	if (zoom > 150 || zoom < 100)
		return -1;

	*zoomvalue = zoom;

	if (zoom == 100)
	{
		setCroppingMode(getCroppingMode());
		return fop(ioctl, MPEG_VID_SCALE_OFF);
	}

	/* the SCALEINFO describes the source and destination of the scaled
	   video. "src" is the part of the source picture that gets scaled,
	   "dst" is the area on the screen where this part is displayed
	   Messing around with MPEG_VID_SET_SCALE_POS disables the automatic
	   letterboxing, which, as I guess, is only a special case of
	   MPEG_VID_SET_SCALE_POS. Therefor we need to care for letterboxing
	   etc here, which is probably not yet totally correct */
	SCALEINFO s;
	memset(&s, 0, sizeof(s));
	if (zoom > 100)
	{
		vidDispSize_t x = getAspectRatio();
		if (x == VID_DISPSIZE_4x3 && croppingMode == VID_DISPMODE_NORM)
		{
			s.src.hori_size = 720;
			s.des.hori_size = 720 * 3/4 * zoom / 100;
		}
		else
		{
			s.src.hori_size = 2 * 720 - 720 * zoom / 100;
			s.des.hori_size = 720;
		}
		s.src.vert_size = 2 * 576 - 576 * zoom / 100;
		s.des.hori_off = (720 - s.des.hori_size) / 2;
		s.des.vert_size = 576;
	}
/* not working correctly (wrong formula) and does not make sense IMHO
	else
	{
		s.src.hori_size = 720;
		s.src.vert_size = 576;
		s.des.hori_size = 720 * zoom / 100;
		s.des.vert_size = 576 * zoom / 100;
		s.des.hori_off = (720 - s.des.hori_size) / 2;
		s.des.vert_off = (576 - s.des.vert_size) / 2;
	}
 */
	DBG("setZoom: %d%% src: %d:%d:%d:%d dst: %d:%d:%d:%d", zoom,
		s.src.hori_off,s.src.vert_off,s.src.hori_size,s.src.vert_size,
		s.des.hori_off,s.des.vert_off,s.des.hori_size,s.des.vert_size);
	fop(ioctl, VIDEO_SET_DISPLAY_FORMAT, VID_DISPMODE_SCALE);
	fop(ioctl, MPEG_VID_SCALE_ON);
	return fop(ioctl, MPEG_VID_SET_SCALE_POS, &s);
}

int CVideo::getZoom(void)
{
	return *zoomvalue;
}

void CVideo::setZoomAspect(int index)
{
	if (index < 0 || index > 1)
		WARN("index out of range");
	else
		zoomvalue = &z[index];
}

void CVideo::setPig(int x, int y, int w, int h, bool /*aspect*/)
{
	/* the aspect parameter is intended for maintaining
	   correct aspect of the PIG... => not yet implemented */
	/* all values zero -> reset ("hide_pig()") */
	if (x + y + w + h == 0)
	{
		setZoom(-1);
		return;
	}
	SCALEINFO s;
	memset(&s, 0, sizeof(s));
	s.src.hori_size = 720;
	s.src.vert_size = 576;
	s.des.hori_off = x;
	s.des.vert_off = y;
	s.des.hori_size = w;
	s.des.vert_size = h;
	DBG("setPig src: %d:%d:%d:%d dst: %d:%d:%d:%d",
		s.src.hori_off,s.src.vert_off,s.src.hori_size,s.src.vert_size,
		s.des.hori_off,s.des.vert_off,s.des.hori_size,s.des.vert_size);
	fop(ioctl, VIDEO_SET_DISPLAY_FORMAT, VID_DISPMODE_SCALE);
	fop(ioctl, MPEG_VID_SCALE_ON);
	fop(ioctl, MPEG_VID_SET_SCALE_POS, &s);
}

int CVideo::VdecIoctl(int request, int arg)
{
	int ret = 0;
	if (fd < 0)
		return -EBADFD;
	DBG("fd: %d req: 0x%08x arg: %d", fd, request, arg);
	ret = ioctl(fd, request, arg);
	if (ret < 0)
		return -errno;
	return ret;
}
#endif
