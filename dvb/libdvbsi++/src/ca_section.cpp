/*
 * $Id: ca_section.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/ca_section.h>

ConditionalAccessSection::ConditionalAccessSection(const uint8_t * const buffer) : LongCrcSection(buffer)
{
	uint16_t pos = 8;
	uint16_t bytesLeft = sectionLength > 9 ? sectionLength - 9 : 0;
	uint16_t loopLength = 0;

	while (bytesLeft > 1 && bytesLeft >= (loopLength = 2 + buffer[pos+1])) {
		descriptor(&buffer[pos], SCOPE_SI);
		pos += loopLength;
		bytesLeft -= loopLength;
	}
}

