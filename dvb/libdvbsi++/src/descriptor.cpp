/*
 * $Id: descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/descriptor.h>

Descriptor::Descriptor(const uint8_t * const buffer)
{
	descriptorTag = buffer[0];
	descriptorLength = buffer[1];

	dataBytes.resize(descriptorLength);
	memcpy(&dataBytes[0], &buffer[2], descriptorLength);

	valid = true;
}

uint8_t Descriptor::getTag(void) const
{
	return descriptorTag;
}

uint8_t Descriptor::getLength(void) const
{
	return descriptorLength;
}

size_t Descriptor::writeToBuffer(uint8_t * const buffer) const
{
	buffer[0] = descriptorTag;
	buffer[1] = descriptorLength;
	memcpy(&buffer[2], &dataBytes[0], descriptorLength);

	return descriptorLength + 2;
}

