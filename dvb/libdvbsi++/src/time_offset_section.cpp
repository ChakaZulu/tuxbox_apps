/*
 * $Id: time_offset_section.cpp,v 1.5 2006/03/28 17:22:00 ghostrider Exp $
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
#include <dvbsi++/time_offset_section.h>

TimeOffsetSection::TimeOffsetSection(const uint8_t * const buffer) : ShortCrcSection(buffer)
{
	utcTimeMjd = sectionLength > 8 ? UINT16(&buffer[3]) : 0;
	utcTimeBcd = sectionLength > 8 ? (buffer[5] << 16) | UINT16(&buffer[6]) : 0;
	descriptorsLoopLength = sectionLength > 10 ? DVB_LENGTH(&buffer[8]) : 0;

	uint16_t pos = 10;
	uint16_t bytesLeft = sectionLength > 11 ? sectionLength - 11 : 0;
	uint16_t loopLength = 0;
	uint16_t bytesLeft2 = descriptorsLoopLength;

	while (bytesLeft >= bytesLeft2 && bytesLeft2 > 1 && bytesLeft2 >= (loopLength = 2 + buffer[pos+1])) {
		descriptor(&buffer[pos], SCOPE_SI);
		pos += loopLength;
		bytesLeft -= loopLength;
		bytesLeft2 -= loopLength;
	}
}

uint16_t TimeOffsetSection::getUtcTimeMjd(void) const
{
	return utcTimeMjd;
}

uint32_t TimeOffsetSection::getUtcTimeBcd(void) const
{
	return utcTimeBcd;
}

