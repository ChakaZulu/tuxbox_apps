/*
 * $Id: video.h,v 1.7 2007/06/17 18:23:57 dbluelle Exp $
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

#if HAVE_DVB_API_VERSION < 3
#include <ost/video.h>
#define video_format_t videoFormat_t
#define video_displayformat_t	videoDisplayFormat_t
#define video_stream_source_t	videoStreamSource_t
#define video_play_state_t	videoPlayState_t
#define video_status		videoStatus
#else
#include <linux/dvb/video.h>
#endif

class CVideo
{
	private:
		/* video device */
		int fd;

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
};

#endif /* __zapit_video_h__ */
