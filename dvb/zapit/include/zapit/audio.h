/*
 * $Id: audio.h,v 1.17 2009/09/30 17:12:39 seife Exp $
 *
 * (C) 2002-2003 by Steffen Hehn 'McClean' &
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

#ifndef __zapit_audio_h__
#define __zapit_audio_h__

#ifdef HAVE_TRIPLEDRAGON
#include <zapit/td-audio-compat.h>
#include <avs/avs_inf.h>
#include <tddevices.h>
#define AVS_DEVICE "/dev/" DEVICE_NAME_AVS
#else /* dbox and dreambox */
#if defined HAVE_DBOX_HARDWARE || defined HAVE_DREAMBOX_HARDWARE || defined HAVE_IPBOX_HARDWARE
#include <dbox/avs_core.h>
#define AVS_DEVICE "/dev/dbox/avs0"
#endif
#if HAVE_DVB_API_VERSION < 3
#include <ost/audio.h>
#define audio_stream_source_t	audioStreamSource_t
#define audio_play_state_t	audioPlayState_t
#define audio_channel_select_t	audioChannelSelect_t
#define audio_mixer		audioMixer
#define	audio_status		audioStatus
#define stream_source		streamSource
#define channel_select		channelSelect
#else
#include <linux/dvb/audio.h>
#endif
#endif /* HAVE_TRIPLEDRAGON */

class CAudio
{
	private:
		/* dvb audio device */
		int fd;

		/* internal methods */
		int setMute(int enable);
		int setBypassMode(int disable);

	public:
		/* construct & destruct */
		CAudio(void);
		~CAudio(void);

		/* shut up */
		int mute(void);
		int unmute(void);

		/* bypass audio to external decoder */
		int enableBypass(void);
		int disableBypass(void);

		/* volume, min = 0, max = 100, forcetype should be CControld::volume_type */
		int setVolume(unsigned char vol, int forcetype = 3);

		/* claim and release the device */
		int openDevice(void);
		void closeDevice(void);

		/* start and stop audio */
		int start(void);
		int stop(void);

		/* stream source */
		audio_stream_source_t getSource(void);
		int setSource(audio_stream_source_t source);

		/* select channels */
		int setChannel(audio_channel_select_t channel);
		audio_channel_select_t getChannel(void);
};

#endif /* __zapit_audio_h__ */
