/*
 * $Id: mpeg_decoder.cpp,v 1.1 2003/07/17 01:07:54 obi Exp $
 *
 * Copyright (C) 2002, 2003 Andreas Oberritter <obi@saftware.de>
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

#include <linux/dvb/audio.h>
#include <linux/dvb/video.h>
#include <sys/ioctl.h>

#include <dvb/debug/debug.h>
#include <dvb/hardware/mpeg_decoder.h>

MpegDecoder *MpegDecoder::mpegDecoder = NULL;

const char * const MpegDecoder::audioFilename = "/dev/dvb/adapter0/audio0";
const char * const MpegDecoder::videoFilename = "/dev/dvb/adapter0/video0";

MpegDecoder::MpegDecoder(void)
{
	audio = 0;
	video = 0;
}

MpegDecoder::~MpegDecoder(void)
{
	stop();
	if (mpegDecoder)
		delete mpegDecoder;
}

MpegDecoder *MpegDecoder::getInstance(void)
{
	if (!mpegDecoder)
		mpegDecoder = new MpegDecoder();
	return mpegDecoder;
}

void MpegDecoder::playAudio(void)
{
	if (audio)
		stop();

	audio = fopen(audioFilename, "r+");

	if (!audio) {
		DVB_ERROR(audioFilename);
		return;
	}

	int fd = fileno(audio);
	
	if (DVB_FOP(ioctl, AUDIO_SET_BYPASS_MODE, 1) < 0)
		return;

	if (DVB_FOP(ioctl, AUDIO_SELECT_SOURCE, AUDIO_SOURCE_DEMUX) < 0)
		return;

	if (DVB_FOP(ioctl, AUDIO_PLAY) < 0)
		return;
}

void MpegDecoder::playVideo(void)
{
	if (video)
		stop();

	video = fopen(videoFilename, "r+");

	if (!video) {
		DVB_ERROR(videoFilename);
		return;
	}

	int fd = fileno(video);

	if (DVB_FOP(ioctl, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_DEMUX) < 0)
		return;

	if (DVB_FOP(ioctl, VIDEO_PLAY) < 0)
		return;
}

void MpegDecoder::stop(void)
{
	int fd;

	if (audio) {
		fd = fileno(audio);
		DVB_FOP(ioctl, AUDIO_STOP);
		fclose(audio);
		audio = 0;
	}

	if (video) {
		fd = fileno(video);
		DVB_FOP(ioctl, VIDEO_STOP);
		fclose(video);
		video = 0;
	}
}

