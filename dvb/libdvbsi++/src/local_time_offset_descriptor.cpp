/*
 * $Id: local_time_offset_descriptor.cpp,v 1.5 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
	for (size_t i = 0; i < descriptorLength; i += 13) {
		ASSERT_MIN_DLEN(i + 13);
		localTimeOffsets.push_back(new LocalTimeOffset(&buffer[i + 2]));
	}
}


LocalTimeOffsetDescriptor::~LocalTimeOffsetDescriptor(void)
{
	for (LocalTimeOffsetIterator i = localTimeOffsets.begin(); i != localTimeOffsets.end(); ++i)
		delete *i;
}

const LocalTimeOffsetList *LocalTimeOffsetDescriptor::getLocalTimeOffsets(void) const
{
	return &localTimeOffsets;
}

