/*
 * $Id: audio.cpp,v 1.16 2009/09/30 17:50:36 seife Exp $
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <zapit/audio.h>
#include <zapit/debug.h>
#include <zapit/settings.h>
#include <zapit/zapit.h>

extern struct Ssettings settings;
unsigned char map_volume(const unsigned char volume, const bool to_AVS);

CAudio::CAudio(void)
{
	fd = -1;
	openDevice();
}

CAudio::~CAudio(void)
{
	closeDevice();
}

int CAudio::openDevice(void)
{
	if (fd < 0)
		if ((fd = open(AUDIO_DEVICE, O_RDWR)) < 0)
			ERROR(AUDIO_DEVICE);
	return fd;
}

void CAudio::closeDevice(void)
{
	if (fd >= 0)
		close(fd);
	fd = -1;
}

#ifndef HAVE_TRIPLEDRAGON
int CAudio::setBypassMode(int disable)
{
	return quiet_fop(ioctl, AUDIO_SET_BYPASS_MODE, disable);
}
#else
int CAudio::setBypassMode(int disable)
{
	/* disable = 1 actually means: audio is MPEG, disable = 0 is audio is AC3 */
	if (disable)
		return quiet_fop(ioctl, MPEG_AUD_SET_MODE, AUD_MODE_MPEG);

	/* dvb2001 does always set AUD_MODE_DTS before setting AUD_MODE_AC3,
	   this might be some workaround, so we do the same... */
	quiet_fop(ioctl, MPEG_AUD_SET_MODE, AUD_MODE_DTS);
	return quiet_fop(ioctl, MPEG_AUD_SET_MODE, AUD_MODE_AC3);

	/* all those ioctl aways return "invalid argument", but they seem to
	   work nevertheless, that's why I use quiet_fop here */
}
#endif

#ifdef HAVE_DBOX_HARDWARE
int CAudio::setMute(int enable)
{
	if (settings.volume_type == CControld::TYPE_OST)
		return fop(ioctl, AUDIO_SET_MUTE, enable);
	int fd, a;
	a = enable ? AVS_MUTE : AVS_UNMUTE;
	if ((fd = open(AVS_DEVICE, O_RDWR)) < 0)
		perror("[controld] " AVS_DEVICE);
	else {
		if (ioctl(fd, AVSIOSMUTE, &a) < 0)
			perror("[controld] AVSIOSMUTE");
		close(fd);
		return 0;
	}
	return -1;
}
#else
/* we can not mute AVS, so mute mpeg */
int CAudio::setMute(int enable)
{
	return fop(ioctl, AUDIO_SET_MUTE, enable);
}
#endif

int CAudio::enableBypass(void)
{
	return setBypassMode(0);
}

int CAudio::disableBypass(void)
{
	return setBypassMode(1);
}

int CAudio::mute(void)
{
	return setMute(1);
}

int CAudio::unmute(void)
{
	return setMute(0);
}

#ifndef HAVE_TRIPLEDRAGON
int CAudio::setVolume(unsigned char volume, int forcetype)
{
	if (settings.volume_type == CControld::TYPE_OST || forcetype == (int)CControld::TYPE_OST)
	{
		unsigned int v = map_volume(volume, false);
		struct audio_mixer mixer;
		mixer.volume_left = v;
		mixer.volume_right = v;
		return fop(ioctl, AUDIO_SET_MIXER, &mixer);
	}
#ifdef HAVE_DBOX_HARDWARE
	else if (settings.volume_type == CControld::TYPE_AVS || forcetype == (int)CControld::TYPE_AVS)
	{
		int fd;
		int i = map_volume(volume, true);
		if ((fd = open(AVS_DEVICE, O_RDWR)) < 0)
			perror("[controld] " AVS_DEVICE);
		else {
			if (ioctl(fd, AVSIOSVOL, &i) < 0)
				perror("[controld] AVSIOSVOL");
			close(fd);
			return 0;
		}
	}
#else
	printf("CAudio::setVolume: volume_type != TYPE_OST not supported on dreambox!\n");
#endif
	return -1;
}
#else
int CAudio::setVolume(const unsigned char volume, int forcetype)
{
	int avsfd;
	int v = (int)map_volume(volume, false);
	if (settings.volume_type == CControld::TYPE_OST || forcetype == (int)CControld::TYPE_OST)
	{
		AUDVOL vol;
		vol.frontleft  = v;
		vol.frontright = v;
		vol.rearleft   = v;
		vol.rearright  = v;
		vol.center     = v;
		vol.lfe        = v;
		return fop(ioctl, MPEG_AUD_SET_VOL, &vol);
	}
	else if (settings.volume_type == CControld::TYPE_AVS || forcetype == (int)CControld::TYPE_AVS)
	{
		if ((avsfd = open(AVS_DEVICE, O_RDWR)) < 0)
			perror("[controld] " AVS_DEVICE);
		else {
			if (ioctl(avsfd, IOC_AVS_SET_VOLUME, v))
				perror("[controld] IOC_AVS_SET_VOLUME");
			close(avsfd);
			return 0;
		}
	}
	fprintf(stderr, "CAudio::setVolume: invalid settings.volume_type = %d\n", settings.volume_type);
	return -1;
}
#endif

int CAudio::setSource(audio_stream_source_t source)
{
	return quiet_fop(ioctl, AUDIO_SELECT_SOURCE, source);
}

#ifndef HAVE_TRIPLEDRAGON
audio_stream_source_t CAudio::getSource(void)
{
	struct audio_status status;
	fop(ioctl, AUDIO_GET_STATUS, &status);
	return status.stream_source;
}
#endif

int CAudio::start(void)
{
	return quiet_fop(ioctl, AUDIO_PLAY);
}

int CAudio::stop(void)
{
	return fop(ioctl, AUDIO_STOP);
}

int CAudio::setChannel(audio_channel_select_t channel)
{
	return fop(ioctl, AUDIO_CHANNEL_SELECT, channel);
}

#ifndef HAVE_TRIPLEDRAGON
audio_channel_select_t CAudio::getChannel(void)
{
	struct audio_status status;
	fop(ioctl, AUDIO_GET_STATUS, &status);
	return status.channel_select;
}
#endif

// input:   0 (min volume) <=     volume           <= 100 (max volume)
// output: 63 (min volume) >= map_volume(., true)  >=   0 (max volume)
// output:  0 (min volume) <= map_volume(., false) <= 255 (max volume)
#ifdef HAVE_DBOX_HARDWARE
unsigned char map_volume(const unsigned char volume, const bool to_AVS)
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
			return settings.scale_logarithmic ? invlog63[volume] : 63 - ((((unsigned int)volume) * 63) / 100);
	}
	else
	{
		if (volume>100) 
			return log255[0];
		else
			return (volume ? (settings.scale_logarithmic ? log255[volume] : ((((unsigned int)volume) * 255) / 100)) : 0);
	}
}
#else
unsigned char map_volume(const unsigned char volume, const bool /*to_AVS*/)
{
	unsigned char vol = volume;
	if (vol > 100)
		vol = 100;

//	vol = (invlog63[volume] + 1) / 2;
	vol = 31 - vol * 31 / 100;
	return vol;
}
#endif
