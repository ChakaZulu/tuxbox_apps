/*
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

#include <dvbsi++/dii_location_descriptor.h>
#include <dvbsi++/byte_stream.h>

DiiLocation::DiiLocation(const uint8_t * const buffer)
{
	diiIdentification = r16(&buffer[0]) & 0x7FFF;
	associationTag = r16(&buffer[2]);
}

uint16_t DiiLocation::getDiiIdentification(void) const
{
	return diiIdentification;
}

uint16_t DiiLocation::getAssociationTag(void) const
{
	return associationTag;
}


DiiLocationDescriptor::DiiLocationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	transportProtocolLabel = buffer[2];
	for (size_t i = 0; i < descriptorLength - 1; i += 4)
		diiLocations.push_back(new DiiLocation(&buffer[i + 3]));
}

DiiLocationDescriptor::~DiiLocationDescriptor(void)
{
	for (DiiLocationIterator i = diiLocations.begin(); i != diiLocations.end(); ++i)
		delete *i;
}

uint8_t DiiLocationDescriptor::getTransportProtocolLabel(void) const
{
	return transportProtocolLabel;
}

const DiiLocationList *DiiLocationDescriptor::getDiiLocations(void) const
{
	return &diiLocations;
}
