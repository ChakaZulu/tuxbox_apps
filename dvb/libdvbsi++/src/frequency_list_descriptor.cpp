/*
 * $Id: frequency_list_descriptor.cpp,v 1.4 2005/10/29 00:10:16 obi Exp $
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
#include <dvbsi++/frequency_list_descriptor.h>

FrequencyListDescriptor::FrequencyListDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	codingType = buffer[2] & 0x03;

	for (size_t i = 0; i < descriptorLength - 1; i += 4)
		centreFrequencies.push_back(UINT32(&buffer[i + 3]));
}

uint8_t FrequencyListDescriptor::getCodingType(void) const
{
	return codingType;
}

const CentreFrequencyList *FrequencyListDescriptor::getCentreFrequencies(void) const
{
	return &centreFrequencies;
}

