/*
 * $Id: pes_filter.h,v 1.1 2003/07/17 01:07:32 obi Exp $
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

#ifndef __dvb_hardware_pes_filter_h__
#define __dvb_hardware_pes_filter_h__

#include <cstdio>
#include <vector>
#include <inttypes.h>
#include <linux/dvb/dmx.h>
#include "mpeg_decoder.h"

class PesFilter
{
	protected:
		std::vector<FILE *> dmxList;
		static const char * const filename;
		MpegDecoder *mpegDecoder;

		void setFilter(const dmx_output_t output, const dmx_pes_type_t pes_type, const uint16_t pid);
		void toDecoder(const dmx_pes_type_t pes_type, const uint16_t pid);
		void toRecorder(const dmx_output_t output, const uint16_t pid);

	public:
		PesFilter(void);
		~PesFilter(void);

		void audioToDecoder(const uint16_t pid);
		void pcrToDecoder(const uint16_t pid);
		void teletextToDecoder(const uint16_t pid);
		void videoToDecoder(const uint16_t pid);

		void toPesRecorder(const uint16_t pid);
		void toTsRecorder(const uint16_t pid);

		void stop(void);
};

#endif /* __dvb_hardware_pes_filter_h__ */
