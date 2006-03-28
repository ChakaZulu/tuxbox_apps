/*
 * $Id: transport_stream_descriptor.cpp,v 1.2 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2006 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/byte_stream.h"
#include "dvbsi++/transport_stream_descriptor.h"

TransportStreamDescriptor::TransportStreamDescriptor(const uint8_t * const buffer) : Descriptor(buffer), privateDataBytes(descriptorLength)
{
	memcpy(&privateDataBytes[0], &buffer[2], descriptorLength);
}

TransportStreamDescriptor::~TransportStreamDescriptor()
{
}

const PrivateDataByteVector *TransportStreamDescriptor::getPrivateDataBytes(void) const
{
	return &privateDataBytes;
}
