/*
 * $Id:
 *
 * (C) 2002 by Steffen Hehn 'McClean'
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


#define AUDIO_DEV "/dev/ost/audio0"


class CAudio
{
	
	private:
		int						audio_fd;		//dvb audio device
		//audioStatus_t	status;	//audio status
		//struct audioMixer_t		mixerStatus;	//audio mixersettings

		int setMute (bool mute);
		int setBypassMode (bool bypass);

	public:
		CAudio();
		~CAudio();

		bool mute();
		bool unMute();

		bool enableBypass();
		bool disableBypass();

		int setVolume (unsigned int left, unsigned int right);

};


#endif
