/*
 * $Id: ca_message_section.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/ca_message_section.h>

ConditionalAccessMessageSection::ConditionalAccessMessageSection(const uint8_t * const buffer) : ShortSection(buffer)
{
	if ( sectionLength > 1 )
		for (size_t i = 8; i < sectionLength - 1; ++i)
			caDataByte.push_back(buffer[i]);
}
