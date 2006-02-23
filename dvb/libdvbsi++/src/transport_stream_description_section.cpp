/*
 * $Id: transport_stream_description_section.cpp,v 1.1 2006/02/23 19:12:41 mws Exp $
 *
 * Copyright (C) 2006 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/transport_stream_description_section.h"

TransportStreamDescriptionSection::TransportStreamDescriptionSection(const uint8_t* const buffer) : LongCrcSection(buffer)
{
	for (size_t i = 8; i < sectionLength - 1; i += buffer[i + 1] + 2)
	{
		descriptor(&buffer[i], SCOPE_SI);
	}
}
