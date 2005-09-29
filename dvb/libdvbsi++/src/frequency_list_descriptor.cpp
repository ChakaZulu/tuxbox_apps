/*
 * $Id: frequency_list_descriptor.cpp,v 1.3 2005/09/29 23:49:44 ghostrider Exp $
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

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/frequency_list_descriptor.h>

FrequencyListDescriptor::FrequencyListDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	codingType = buffer[2] & 0x03;

	for (size_t i = 0; i < descriptorLength - 1; i += 4)
		centreFrequencies.push_back(UINT32(&buffer[i + 3]));
}

uint8_t FrequencyListDescriptor::getCodingType(void) const
{
	return codingType;
}

const CentreFrequencyList *FrequencyListDescriptor::getCentreFrequencies(void) const
{
	return &centreFrequencies;
}

