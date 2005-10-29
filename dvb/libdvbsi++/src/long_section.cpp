/*
 * $Id: long_section.cpp,v 1.3 2005/10/29 00:10:17 obi Exp $
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
#include <dvbsi++/long_section.h>

LongSection::LongSection(const uint8_t * const buffer) : ShortSection(buffer)
{
	tableIdExtension = UINT16(&buffer[3]);
	versionNumber = (buffer[5] >> 1) & 0x1f;
	currentNextIndicator = buffer[5] & 0x01;
	sectionNumber = buffer[6];
	lastSectionNumber = buffer[7];
}

uint16_t LongSection::getTableIdExtension(void) const
{
	return tableIdExtension;
}

uint8_t LongSection::getVersionNumber(void) const
{
	return versionNumber;
}

uint8_t LongSection::getCurrentNextIndicator(void) const
{
	return currentNextIndicator;
}

uint8_t LongSection::getSectionNumber(void) const
{
	return sectionNumber;
}

uint8_t LongSection::getLastSectionNumber(void) const
{
	return lastSectionNumber;
}

bool LongSection::operator< (const LongSection &t) const
{
	return (sectionNumber < t.sectionNumber);
}

bool LongSection::operator> (const LongSection &t) const
{
	return (sectionNumber > t.sectionNumber);
}

bool LongSection::operator<= (const LongSection &t) const
{
	return (sectionNumber <= t.sectionNumber);
}

bool LongSection::operator>= (const LongSection &t) const
{
	return (sectionNumber >= t.sectionNumber);
}

bool LongSection::operator== (const LongSection &t) const
{
	return (sectionNumber == t.sectionNumber);
}

bool LongSection::operator!= (const LongSection &t) const
{
	return (sectionNumber != t.sectionNumber);
}

