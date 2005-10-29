/*
 * $Id: ca_section.cpp,v 1.3 2005/10/29 00:10:16 obi Exp $
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
	for (size_t i = 8; i < sectionLength - 1; i += buffer[i + 1] + 2)
		descriptor(&buffer[i], SCOPE_SI);
}

