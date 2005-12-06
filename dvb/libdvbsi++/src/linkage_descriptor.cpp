/*
 * $Id: linkage_descriptor.cpp,v 1.7 2005/12/06 23:51:25 ghostrider Exp $
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
	transportStreamId = UINT16(&buffer[2]);
	originalNetworkId = UINT16(&buffer[4]);
	serviceId = UINT16(&buffer[6]);
	linkageType = buffer[8];

	if (linkageType != 0x08)
	{
		privateDataBytes.reserve(descriptorLength - 7);
		privateDataBytes.insert(privateDataBytes.begin(), buffer+9, buffer+9+descriptorLength-7);
	}
	else {
		uint8_t offset = 0;
		int bytes = 0;
		handOverType = (buffer[9] >> 4) & 0x0f;
		originType = buffer[9] & 0x01;

		if ((handOverType >= 0x01) && (handOverType <= 0x03)) {
			networkId = UINT16(&buffer[10]);
			offset += 2;
		}

		if (originType == 0x00) {
			initialServiceId = UINT16(&buffer[offset + 10]);
			offset += 2;
		}
		bytes = descriptorLength-offset-8;
		privateDataBytes.reserve(bytes);
		privateDataBytes.insert(privateDataBytes.begin(), buffer+offset+10, buffer+offset+10+bytes);
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

