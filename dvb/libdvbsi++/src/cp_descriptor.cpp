/*
 * $Id: cp_descriptor.cpp,v 1.1 2009/06/30 12:03:03 mws Exp $
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
#include <dvbsi++/cp_descriptor.h>

CpDescriptor::CpDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(5);

	cpSystemId = UINT16(&buffer[3]);
	cpPid = DVB_PID(&buffer[5]);

	cpDataBytes.resize(descriptorLength - 5);
	memcpy(&cpDataBytes[0], &buffer[7], descriptorLength - 5);
}

uint16_t CpDescriptor::getCpSystemId(void) const
{
	return cpSystemId;
}

uint16_t CpDescriptor::getCpPid(void) const
{
	return cpPid;
}

const CpDataByteVector *CpDescriptor::getCpDataBytes(void) const
{
	return &cpDataBytes;
}

