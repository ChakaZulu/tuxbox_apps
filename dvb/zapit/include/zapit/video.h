/*
 * $Id: video.h,v 1.4 2002/11/02 17:21:15 obi Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#ifndef __video_h__
#define __video_h__

#include <time.h>

#include <linux/dvb/video.h>

class CVideo
{
	private:
		/* video device */
		int fd;

		/* video status */
		struct video_status status;

		/* true when construction was complete */
		bool initialized;

	public:
		/* constructor & destructor */
		CVideo ();
		~CVideo ();

		bool isInitialized () { return initialized; }

		/* aspect ratio */
		video_format_t getAspectRatio () { return status.video_format; }
		int setAspectRatio (video_format_t format);

		/* cropping mode */
		video_displayformat_t getCroppingMode () { return status.display_format; }
		int setCroppingMode (video_displayformat_t format);

		/* stream source */
		video_stream_source_t getSource () { return status.stream_source; }
		int setSource (video_stream_source_t source);

		/* blank on freeze */
		bool getBlank () { return status.video_blank; }
		int setBlank (bool blank);

		/* get play state */
		bool isPlaying () { return (status.play_state == VIDEO_PLAYING); }

		/* change video play state */
		int start ();
		int stop ();
};

#endif /* __video_h__ */
