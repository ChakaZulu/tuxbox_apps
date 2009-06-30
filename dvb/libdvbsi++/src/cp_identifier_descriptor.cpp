/*
 * $Id: cp_identifier_descriptor.cpp,v 1.1 2009/06/30 12:03:03 mws Exp $
 *
 * Copyright (C) 2009 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/cp_identifier_descriptor.h>

CpIdentifierDescriptor::CpIdentifierDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength -1; i += 2)
		cpSystemIds.push_back(UINT16(&buffer[i + 3]));
}

const CpSystemIdList *CpIdentifierDescriptor::getCpSystemIds(void) const
{
	return &cpSystemIds;
}

