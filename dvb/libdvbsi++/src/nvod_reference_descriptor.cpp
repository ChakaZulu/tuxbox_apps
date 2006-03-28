/*
 * $Id: nvod_reference_descriptor.cpp,v 1.5 2006/03/28 17:22:00 ghostrider Exp $
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
#include <dvbsi++/nvod_reference_descriptor.h>

NvodReference::NvodReference(const uint8_t * const buffer)
{
	transportStreamId = UINT16(&buffer[0]);
	originalNetworkId = UINT16(&buffer[2]);
	serviceId = UINT16(&buffer[4]);
}

uint16_t NvodReference::getTransportStreamId(void) const
{
	return transportStreamId;
}

uint16_t NvodReference::getOriginalNetworkId(void) const
{
	return originalNetworkId;
}

uint16_t NvodReference::getServiceId(void) const
{
	return serviceId;
}

NvodReferenceDescriptor::NvodReferenceDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 6) {
		ASSERT_MIN_DLEN(i + 6);
		nvodReferences.push_back(new NvodReference(&buffer[i + 2]));
	}
}

NvodReferenceDescriptor::~NvodReferenceDescriptor(void)
{
	for (NvodReferenceIterator i = nvodReferences.begin(); i != nvodReferences.end(); ++i)
		delete *i;
}

const NvodReferenceList *NvodReferenceDescriptor::getNvodReferences(void) const
{
	return &nvodReferences;
}

