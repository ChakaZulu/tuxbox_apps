/*
 * $Id: service_availability_descriptor.cpp,v 1.1 2008/03/31 07:52:52 mws Exp $
 *
 * Copyright (C) 2008 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/service_availability_descriptor.h>

ServiceAvailabilityDescriptor::ServiceAvailabilityDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(1);

	availabilityFlag = (buffer[2] >> 7) & 0x01;

	for (size_t i = 0; i < descriptorLength - 1; i += 2) {
		ASSERT_MIN_DLEN(i + 3);
		cellIds.push_back(UINT16(&buffer[i + 3]));
	}
}

uint8_t ServiceAvailabilityDescriptor::getAvailabilityFlag(void) const
{
	return availabilityFlag;
}

const CellIdList *ServiceAvailabilityDescriptor::getCellIds(void) const
{
	return &cellIds;
}
