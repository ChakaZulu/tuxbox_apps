/*
 * $Id: xait_location_descriptor.cpp,v 1.1 2009/06/30 07:28:30 mws Exp $
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
#include "dvbsi++/xait_location_descriptor.h"

XaitLocationDescriptor::XaitLocationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	size_t dataLength = 5;
	ASSERT_MIN_DLEN(dataLength);

	originalNetworkId = r16(&buffer[2]);
	serviceId = r16(&buffer[4]);
	versionNumber = (buffer[6] >> 3) & 0x1f;
	updatePolicy = buffer[6] & 0x03;
}

uint16_t XaitLocationDescriptor::getOriginalNetworkId() const
{
	return originalNetworkId;
}

uint16_t XaitLocationDescriptor::getServiceId() const
{
	return serviceId;
}

uint8_t XaitLocationDescriptor::getVersionNumber() const
{
	return versionNumber;
}

uint8_t XaitLocationDescriptor::getUpdatePolicy() const
{
	return updatePolicy;
}
