/*
 * $Id: long_section.cpp,v 1.2 2004/02/13 17:51:08 obi Exp $
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

