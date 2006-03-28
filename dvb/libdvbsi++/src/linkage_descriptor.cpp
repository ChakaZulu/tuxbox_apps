/*
 * $Id: linkage_descriptor.cpp,v 1.9 2006/03/28 17:22:00 ghostrider Exp $
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
#include <dvbsi++/linkage_descriptor.h>

LinkageDescriptor::LinkageDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	size_t headerLength = 7;
	ASSERT_MIN_DLEN(headerLength);

	transportStreamId = UINT16(&buffer[2]);
	originalNetworkId = UINT16(&buffer[4]);
	serviceId = UINT16(&buffer[6]);
	linkageType = buffer[8];

	if (linkageType != 0x08) {
		privateDataBytes.resize(descriptorLength - headerLength);
		memcpy(&privateDataBytes[0], &buffer[9], descriptorLength - headerLength);
	} else {
		uint8_t offset = 0;
		int bytes = 0;

		headerLength++;
		ASSERT_MIN_DLEN(headerLength);

		handOverType = (buffer[9] >> 4) & 0x0f;
		originType = buffer[9] & 0x01;

		if ((handOverType >= 0x01) && (handOverType <= 0x03)) {
			headerLength += 2;
			ASSERT_MIN_DLEN(headerLength);

			networkId = UINT16(&buffer[10]);
			offset += 2;
		}

		if (originType == 0x00) {
			headerLength += 2;
			ASSERT_MIN_DLEN(headerLength);

			initialServiceId = UINT16(&buffer[offset + 10]);
			offset += 2;
		}

		privateDataBytes.resize(descriptorLength - headerLength);
		memcpy(&privateDataBytes[0], &buffer[offset + 10], descriptorLength - headerLength);
	}
}

uint16_t LinkageDescriptor::getTransportStreamId(void) const
{
	return transportStreamId;
}

uint16_t LinkageDescriptor::getOriginalNetworkId(void) const
{
	return originalNetworkId;
}

uint16_t LinkageDescriptor::getServiceId(void) const
{
	return serviceId;
}

uint8_t LinkageDescriptor::getLinkageType(void) const
{
	return linkageType;
}

const PrivateDataByteVector *LinkageDescriptor::getPrivateDataBytes(void) const
{
	return &privateDataBytes;
}

uint8_t LinkageDescriptor::getHandOverType(void) const
{
	return handOverType;
}

uint8_t LinkageDescriptor::getOriginType(void) const
{
	return originType;
}

uint16_t LinkageDescriptor::getNetworkId(void) const
{
	return networkId;
}

uint16_t LinkageDescriptor::getInitialServiceId(void) const
{
	return initialServiceId;
}

