/*
 * $Id: pes_filter.cpp,v 1.1 2003/07/17 01:07:54 obi Exp $
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

#include <linux/dvb/dmx.h>
#include <sys/ioctl.h>

#include <dvb/debug/debug.h>
#include <dvb/hardware/pes_filter.h>

const char * const PesFilter::filename = "/dev/dvb/adapter0/demux0";

PesFilter::PesFilter(void)
{
	mpegDecoder = MpegDecoder::getInstance();
}

PesFilter::~PesFilter(void)
{
	stop();
}

void PesFilter::setFilter(const dmx_output_t output, const dmx_pes_type_t pes_type, const uint16_t pid)
{
	FILE *dmx = fopen(filename, "r+");

	if (!dmx) {
		DVB_ERROR(filename);
		return;
	}

	struct dmx_pes_filter_params dpfp;
	dpfp.pid = pid;
	dpfp.input = DMX_IN_FRONTEND;
	dpfp.output = output;
	dpfp.pes_type = pes_type;
	dpfp.flags = DMX_IMMEDIATE_START;

	int fd = fileno(dmx);

	if (DVB_FOP(ioctl, DMX_SET_PES_FILTER, &dpfp) < 0)
		return;

	if ((output == DMX_OUT_DECODER) && (mpegDecoder)) {
		switch (dpfp.pes_type) {
		case DMX_PES_AUDIO:
			mpegDecoder->playAudio();
			break;
		case DMX_PES_VIDEO:
			mpegDecoder->playVideo();
			break;
		default:
			break;
		}
	}

	dmxList.push_back(dmx);
}

void PesFilter::toDecoder(const dmx_pes_type_t pes_type, const uint16_t pid)
{
	setFilter(DMX_OUT_DECODER, pes_type, pid);
}

void PesFilter::toRecorder(const dmx_output_t output, const uint16_t pid)
{
	setFilter(output, DMX_PES_OTHER, pid);
}

void PesFilter::audioToDecoder(const uint16_t pid)
{
	toDecoder(DMX_PES_AUDIO, pid);
}

void PesFilter::pcrToDecoder(const uint16_t pid)
{
	toDecoder(DMX_PES_PCR, pid);
}

void PesFilter::teletextToDecoder(const uint16_t pid)
{
	toDecoder(DMX_PES_TELETEXT, pid);
}

void PesFilter::videoToDecoder(const uint16_t pid)
{
	toDecoder(DMX_PES_VIDEO, pid);
}

void PesFilter::toPesRecorder(const uint16_t pid)
{
	toRecorder(DMX_OUT_TAP, pid);
}

void PesFilter::toTsRecorder(const uint16_t pid)
{
	toRecorder(DMX_OUT_TS_TAP, pid);
}

void PesFilter::stop(void)
{
	for (std::vector<FILE *>::iterator i = dmxList.begin(); i != dmxList.end(); ++i)
		if (*i) {
			int fd = fileno(*i);
			DVB_FOP(ioctl, DMX_STOP);
			fclose(*i);
		}

	dmxList.clear();

	if (mpegDecoder)
		mpegDecoder->stop();
}

