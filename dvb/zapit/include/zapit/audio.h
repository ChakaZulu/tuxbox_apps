/*
 * $Id: audio.h,v 1.10 2002/11/02 17:21:15 obi Exp $
 *
 * (C) 2002 by Steffen Hehn 'McClean' &
 *	Andreas Oberritter <obi@tuxbox.org>
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

#ifndef __audio_h__
#define __audio_h__

/* nokia api */
#include <linux/dvb/audio.h>

class CAudio
{
	private:
		/* dvb audio device */
		int fd;

		/* current audio settings */
		struct audio_status status;
		struct audio_mixer mixer;

		/* internal methods */
		int setMute (bool mute);
		int setBypassMode (bool bypass);

		/* true if constructor had success */
		bool initialized;

	public:
		/* construct & destruct */
		CAudio();
		~CAudio();

		/* check if initialitation failed before rocking */
		bool isInitialized () { return initialized; }

		/* shut up */
		int mute ();
		int unmute ();

		/* bypass audio to external decoder */
		int enableBypass ();
		int disableBypass ();

		/* volume, min = 0, max = 255 */
		int setVolume (unsigned char left, unsigned char right);

		/* start and stop audio */
		int start ();
		int stop ();

		/* stream source */
		audio_stream_source_t getSource () { return status.stream_source; }
		int setSource (audio_stream_source_t source);

		/* select channels */
		int selectChannel (audio_channel_select_t sel);
		audio_channel_select_t getSelectedChannel ();
};

#endif /* __audio_h__ */
