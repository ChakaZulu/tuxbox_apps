/*
 * $Id: short_section.cpp,v 1.3 2005/10/29 00:10:17 obi Exp $
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
#include <dvbsi++/short_section.h>

ShortSection::ShortSection(const uint8_t * const buffer)
{
	tableId = buffer[0];
	sectionSyntaxIndicator = (buffer[1] >> 7) & 0x01;
	sectionLength = DVB_LENGTH(&buffer[1]);
}

uint8_t ShortSection::getTableId(void) const
{
	return tableId;
}

uint8_t ShortSection::getSectionSyntaxIndicator(void) const
{
	return sectionSyntaxIndicator;
}

uint16_t ShortSection::getSectionLength(void) const
{
	return sectionLength;
}

