/*
 * $Id: local_time_offset_descriptor.cpp,v 1.2 2004/02/13 17:51:08 obi Exp $
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
#include <dvbsi++/local_time_offset_descriptor.h>

LocalTimeOffset::LocalTimeOffset(const uint8_t * const buffer)
{
	countryCode.assign((char *)&buffer[0], 3);
	countryRegionId = (buffer[3] >> 2) & 0x3f;
	localTimeOffsetPolarity = buffer[3] & 0x01;
	localTimeOffset = UINT16(&buffer[4]);
	timeOfChangeMjd = UINT16(&buffer[6]);
	timeOfChangeBcd = (buffer[8] << 16) | UINT16(&buffer[9]);
	nextTimeOffset = UINT16(&buffer[11]);
}

const std::string &LocalTimeOffset::getCountryCode(void) const
{
	return countryCode;
}

uint8_t LocalTimeOffset::getCountryRegionId(void) const
{
	return countryRegionId;
}

uint8_t LocalTimeOffset::getLocalTimeOffsetPolarity(void) const
{
	return localTimeOffsetPolarity;
}

uint16_t LocalTimeOffset::getLocalTimeOffset(void) const
{
	return localTimeOffset;
}

uint16_t LocalTimeOffset::getTimeOfChangeMjd(void) const
{
	return timeOfChangeMjd;
}

uint32_t LocalTimeOffset::getTimeOfChangeBcd(void) const
{
	return timeOfChangeBcd;
}

uint16_t LocalTimeOffset::getNextTimeOffset(void) const
{
	return nextTimeOffset;
}

LocalTimeOffsetDescriptor::LocalTimeOffsetDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 13)
		localTimeOffsets.push_back(new LocalTimeOffset(&buffer[i + 2]));
}


LocalTimeOffsetDescriptor::~LocalTimeOffsetDescriptor(void)
{
	for (LocalTimeOffsetIterator i = localTimeOffsets.begin(); i != localTimeOffsets.end(); ++i)
		delete *i;
}

const LocalTimeOffsetVector *LocalTimeOffsetDescriptor::getLocalTimeOffsets(void) const
{
	return &localTimeOffsets;
}

