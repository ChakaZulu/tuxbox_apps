/*
 * $Id: short_crc_section.cpp,v 1.3 2005/10/29 00:10:17 obi Exp $
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
#include <dvbsi++/short_crc_section.h>

ShortCrcSection::ShortCrcSection(const uint8_t * const buffer) : ShortSection(buffer)
{
	crc32 = UINT32(&buffer[sectionLength - 1]);
}

uint32_t ShortCrcSection::getCrc32(void) const
{
	return crc32;
}

