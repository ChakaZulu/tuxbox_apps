/*
 * $Id: ca_descriptor.cpp,v 1.3 2005/09/29 23:49:44 ghostrider Exp $
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

CaDescriptor::CaDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	caSystemId = UINT16(&buffer[2]);
	caPid = DVB_PID(&buffer[4]);

	for (size_t i = 0; i < descriptorLength - 4; ++i)
		privateDataBytes.push_back(buffer[i + 6]);
}

uint16_t CaDescriptor::getCaSystemId(void) const
{
	return caSystemId;
}

uint16_t CaDescriptor::getCaPid(void) const
{
	return caPid;
}

const PrivateDataByteList *CaDescriptor::getPrivateDataBytes(void) const
{
	return &privateDataBytes;
}

size_t CaDescriptor::writeToBuffer(uint8_t * const buffer) const
{
	size_t total = 0;

	total += Descriptor::writeToBuffer(buffer);

	buffer[total++] = (caSystemId >> 8) & 0xff;
	buffer[total++] = (caSystemId >> 0) & 0xff;
	buffer[total++] = (caPid >> 8) & 0xff;
	buffer[total++] = (caPid >> 0) & 0xff;

	for (PrivateDataByteConstIterator i = privateDataBytes.begin(); i != privateDataBytes.end(); ++i)
		buffer[total++] = *i;

	return total;
}

