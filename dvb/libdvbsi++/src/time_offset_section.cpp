/*
 * $Id: time_offset_section.cpp,v 1.4 2005/10/29 00:10:17 obi Exp $
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
	utcTimeMjd = UINT16(&buffer[3]);
	utcTimeBcd = (buffer[5] << 16) | UINT16(&buffer[6]);
	descriptorsLoopLength = DVB_LENGTH(&buffer[8]);

	for (size_t i = 0; i < descriptorsLoopLength; i += buffer[i + 11] + 2)
		descriptor(&buffer[i + 10], SCOPE_SI);
}

uint16_t TimeOffsetSection::getUtcTimeMjd(void) const
{
	return utcTimeMjd;
}

uint32_t TimeOffsetSection::getUtcTimeBcd(void) const
{
	return utcTimeBcd;
}

