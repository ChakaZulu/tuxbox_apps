/*
 * $Id: ca_descriptor.cpp,v 1.5 2005/10/29 00:10:16 obi Exp $
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
#include <dvbsi++/ca_descriptor.h>

CaDescriptor::CaDescriptor(const uint8_t * const buffer) : Descriptor(buffer), caDataBytes(descriptorLength-4)
{
	caSystemId = UINT16(&buffer[2]);
	caPid = DVB_PID(&buffer[4]);

	memcpy(&caDataBytes[0], buffer+6, descriptorLength-4);
}

uint16_t CaDescriptor::getCaSystemId(void) const
{
	return caSystemId;
}

uint16_t CaDescriptor::getCaPid(void) const
{
	return caPid;
}

const CaDataByteVector *CaDescriptor::getCaDataBytes(void) const
{
	return &caDataBytes;
}

size_t CaDescriptor::writeToBuffer(uint8_t * const buffer) const
{
	size_t total = 0;

	total += Descriptor::writeToBuffer(buffer);

	buffer[total++] = (caSystemId >> 8) & 0xff;
	buffer[total++] = (caSystemId >> 0) & 0xff;
	buffer[total++] = (caPid >> 8) & 0xff;
	buffer[total++] = (caPid >> 0) & 0xff;

	memcpy(buffer+total, &caDataBytes[0], caDataBytes.size());

	return total+caDataBytes.size();
}

