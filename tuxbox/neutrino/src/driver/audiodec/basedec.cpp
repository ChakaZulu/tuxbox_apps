/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2004 Zwen
   base decoder class
	Homepage: http://www.cyberphoria.org/

	Kommentar:

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

#define DBOX

#include <basedec.h>
#include <cdrdec.h>
#include <mp3dec.h>
#include <oggdec.h>
#include <wavdec.h>
#include <linux/soundcard.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <dbox/avs_core.h>
#include <driver/netfile.h>
#define AVS_DEVICE "/dev/dbox/avs0"

unsigned int CBaseDec::mSamplerate=0;

CBaseDec::RetCode CBaseDec::DecoderBase(FILE *InputFp,int OutputFd, State* state, CAudioMetaData* md, time_t* t)
{
	RetCode Status;
	if(ftype(InputFp, "ogg"))
	{
		printf("(ogg)\n");
		Status = COggDec::getInstance()->Decoder(InputFp, OutputFd, state, md, t);
	}
	else if(ftype(InputFp, "cdr"))
	{
		printf("(cdr)\n");
		Status = CCdrDec::getInstance()->Decoder(InputFp, OutputFd, state, md, t);
	}
	else if(ftype(InputFp, "wav"))
	{
		printf("(wav)\n");
		Status = CWavDec::getInstance()->Decoder(InputFp, OutputFd, state, md, t);
	}
	else
	{
		printf("(mp3)\n");
		Status = CMP3Dec::getInstance()->Decoder(InputFp, OutputFd, state, md, t);
	}
	return Status;
}

bool CBaseDec::GetMetaDataBase(FILE *in, bool nice, CAudioMetaData* m)
{
	bool Status;
	if(ftype(in, "ogg"))
	{
		Status = COggDec::getInstance()->GetMetaData(in, nice, m);
	}
	else if(ftype(in, "cdr"))
	{
		Status = CCdrDec::getInstance()->GetMetaData(in, nice, m);
	}
	else if(ftype(in, "wav"))
	{
		Status = CWavDec::getInstance()->GetMetaData(in, nice, m);
	}
	else
	{
		Status = CMP3Dec::getInstance()->GetMetaData(in, nice, m);
	}
	return Status;
}

bool CBaseDec::SetDSP(int soundfd, int fmt, unsigned int dsp_speed, unsigned int channels)
{
	bool crit_error=false;
	 
	if (::ioctl(soundfd, SNDCTL_DSP_RESET))
		printf("reset failed\n");
	if(::ioctl(soundfd, SNDCTL_DSP_SETFMT, &fmt))
		printf("setfmt failed\n");
	if(::ioctl(soundfd, SNDCTL_DSP_CHANNELS, &channels))
		printf("channel set failed\n");
#ifdef	 DBOX
	if (dsp_speed != mSamplerate)
#endif	
   {
		// mute audio to reduce pops when changing samplerate (avia_reset)
		bool was_muted = avs_mute(true);
		if (::ioctl(soundfd, SNDCTL_DSP_SPEED, &dsp_speed))
		{
			printf("speed set failed\n");
			crit_error=true;
	 	}
	 	else
	 		mSamplerate = dsp_speed;
		usleep(400000);
		if (!was_muted)
			avs_mute(false);
	}
//		  printf("Debug: SNDCTL_DSP_RESET %d / SNDCTL_DSP_SPEED %d / SNDCTL_DSP_CHANNELS %d / SNDCTL_DSP_SETFMT %d\n",
//					SNDCTL_DSP_RESET, SNDCTL_DSP_SPEED, SNDCTL_DSP_CHANNELS, SNDCTL_DSP_SETFMT);
	return crit_error;
}

bool CBaseDec::avs_mute(bool mute)
{
	int fd, a, b=AVS_UNMUTE;
	a = mute ? AVS_MUTE : AVS_UNMUTE;
	if ((fd = open(AVS_DEVICE, O_RDWR)) < 0)
		 perror("[CBaseDec::avs_mute] " AVS_DEVICE);
	 else 
	 {
		 if (ioctl(fd, AVSIOGMUTE, &b) < 0)
			 perror("[CBaseDec::avs_mute] AVSIOSMUTE");
		 if(a!=b)
		 {
			 if (ioctl(fd, AVSIOSMUTE, &a) < 0)
				 perror("[CBaseDec::avs_mute] AVSIOSMUTE");
		 }
		 close(fd);
	 }
	 return (b==AVS_MUTE);
}

void CBaseDec::Init()
{
	mSamplerate=0;
}

