/*
 * $Id: audio_stream_descriptor.h,v 1.1 2004/02/13 15:27:37 obi Exp $
 *
 * Copyright (C) 2002-2004 Andreas Oberritter <obi@saftware.de>
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

#ifndef __audio_stream_descriptor_h__
#define __audio_stream_descriptor_h__

#include "descriptor.h"

class AudioStreamDescriptor : public Descriptor
{
	protected:
		unsigned freeFormatFlag				: 1;
		unsigned id					: 1;
		unsigned layer					: 2;
		unsigned variableRateAudioIndicator		: 1;

	public:
		AudioStreamDescriptor(const uint8_t * const buffer);

		uint8_t getFreeFormatFlag(void) const;
		uint8_t getId(void) const;
		uint8_t getLayer(void) const;
		uint8_t getVariableRateAudioIndicator(void) const;
};

#endif /* __audio_stream_descriptor_h__ */
