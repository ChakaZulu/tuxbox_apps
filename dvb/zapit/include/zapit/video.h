/*
 * $Id: video.h,v 1.11 2009/09/30 17:54:43 seife Exp $
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

#ifndef __zapit_video_h__
#define __zapit_video_h__

#ifdef HAVE_TRIPLEDRAGON
#include <zapit/td-video-compat.h>
#include <clip/clipinfo.h>
#else
#if HAVE_DVB_API_VERSION < 3
#include <ost/video.h>
#define video_format_t videoFormat_t
#define video_displayformat_t	videoDisplayFormat_t
#define video_stream_source_t	videoStreamSource_t
#define video_play_state_t	videoPlayState_t
#define video_status		videoStatus
#else
#include <linux/dvb/video.h>
#endif /* HAVE_DVB_API_VERSION */
#endif /* !HAVE_TRIPLEDRAGON */

#if defined HAVE_DBOX_HARDWARE || defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
#include <dbox/avs_core.h>
#include <dbox/saa7126_core.h>
#endif

class CVideo
{
	private:
		/* video device */
		int fd;
#ifdef HAVE_TRIPLEDRAGON
		/* apparently we cannot query the driver's state
		   => remember it */
		video_play_state_t playstate;
		video_displayformat_t croppingMode;
		int z[2]; /* zoomvalue for 4:3 (0) and 16:9 (1) in percent */
		int *zoomvalue;
		void *blank_data[2]; /* we store two blank MPEGs (PAL/NTSC) in there */
		int blank_size[2];
#endif

	public:
		/* constructor & destructor */
		CVideo(void);
		~CVideo(void);

		/* aspect ratio */
		video_format_t getAspectRatio(void);
		int setAspectRatio(video_format_t format);

		/* cropping mode */
		video_displayformat_t getCroppingMode(void);
		int setCroppingMode(video_displayformat_t format);
		/* video format (auto/16:9/4:3LB/4:3PS) */
		void setVideoFormat(int format);

		/* stream source */
		video_stream_source_t getSource(void);
		int setSource(video_stream_source_t source);

		/* blank on freeze */
		int getBlank(void);
		int setBlank(int enable);

		/* get play state */
		video_play_state_t getPlayState(void);

		/* change video play state */
		int start(void);
		int stop(void);

		/* set video_system */
		int setVideoSystem(int video_system);
#ifdef HAVE_TRIPLEDRAGON
		/* that's S-Video, RGB, CVBS, ... */
		int setVideoOutput(vidOutFmt_t arg);
		int setZoom(int zoom);
		int getZoom(void);
		void setZoomAspect(int index);
		void setPig(int x, int y, int w, int h, bool aspect);
		int VdecIoctl(int request, int arg);
#endif
};

#endif /* __zapit_video_h__ */
