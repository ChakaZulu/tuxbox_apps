/*
 * $Id: mpeg_decoder.h,v 1.1 2003/07/17 01:07:32 obi Exp $
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

#ifndef __dvb_hardware_mpeg_decoder_h__
#define __dvb_hardware_mpeg_decoder_h__

#include <cstdio>

class MpegDecoder
{
	protected:
		MpegDecoder(void);
		~MpegDecoder(void);
		static MpegDecoder *mpegDecoder;
		static const char * const audioFilename;
		static const char * const videoFilename;
		FILE *audio;
		FILE *video;

	public:
		static MpegDecoder *getInstance(void);
		void playAudio(void);
		void playVideo(void);
		void stop(void);
};

#endif /* __dvb_hardware_mpeg_decoder_h__ */
