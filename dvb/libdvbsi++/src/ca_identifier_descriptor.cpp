/*
 * $Id: ca_identifier_descriptor.cpp,v 1.4 2005/10/29 00:10:16 obi Exp $
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
#include <dvbsi++/ca_identifier_descriptor.h>

CaIdentifierDescriptor::CaIdentifierDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 2)
		caSystemIds.push_back(UINT16(&buffer[i + 2]));
}

const CaSystemIdList *CaIdentifierDescriptor::getCaSystemIds(void) const
{
	return &caSystemIds;
}

