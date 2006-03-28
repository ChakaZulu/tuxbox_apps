/*
 * $Id: ca_descriptor.cpp,v 1.6 2006/03/28 17:22:00 ghostrider Exp $
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

CaDescriptor::CaDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(4);

	caSystemId = UINT16(&buffer[2]);
	caPid = DVB_PID(&buffer[4]);

	caDataBytes.resize(descriptorLength - 4);
	memcpy(&caDataBytes[0], &buffer[6], descriptorLength - 4);
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

