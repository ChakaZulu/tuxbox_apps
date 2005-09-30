/*
 * $Id: ca_descriptor.cpp,v 1.4 2005/09/30 16:13:49 ghostrider Exp $
 *
 * Copyright (C) 2002-2004 Andreas Oberritter <obi@saftware.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
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

