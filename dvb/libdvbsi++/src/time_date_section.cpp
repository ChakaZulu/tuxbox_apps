/*
 * $Id: time_date_section.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
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
#include <dvbsi++/time_date_section.h>

TimeAndDateSection::TimeAndDateSection(const uint8_t * const buffer) : ShortSection(buffer)
{
	if (sectionLength > 8) {
		utcTimeMjd = UINT16(&buffer[3]);
		utcTimeBcd = (buffer[5] << 16) | UINT16(&buffer[6]);
	}
	else {
		utcTimeMjd=0;
		utcTimeBcd=0;
	}
}

uint16_t TimeAndDateSection::getUtcTimeMjd(void) const
{
	return utcTimeMjd;
}

uint32_t TimeAndDateSection::getUtcTimeBcd(void) const
{
	return utcTimeBcd;
}

