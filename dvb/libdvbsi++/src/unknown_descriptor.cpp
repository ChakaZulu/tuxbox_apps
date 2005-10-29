/*
 * $Id: unknown_descriptor.cpp,v 1.3 2005/10/29 00:10:17 obi Exp $
 *
 * Copyright (C) 2005 Andreas Monzner <andreas.monzner@multimedia-labs.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/unknown_descriptor.h>

UnknownDescriptor::UnknownDescriptor(const uint8_t * const buffer) : Descriptor(buffer), dataBytes(descriptorLength)
{
	memcpy(&dataBytes[0], buffer+2, descriptorLength);
}

size_t UnknownDescriptor::writeToBuffer(uint8_t * const buffer) const
{
	Descriptor::writeToBuffer(buffer);
	memcpy(buffer+2, &dataBytes[0], descriptorLength);
	return 2 + descriptorLength;
}

