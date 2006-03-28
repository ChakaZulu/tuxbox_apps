/*
 * $Id: dii_location_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
	ASSERT_MIN_DLEN(1);

	transportProtocolLabel = buffer[2];
	for (size_t i = 0; i < descriptorLength - 1; i += 4) {
		ASSERT_MIN_DLEN(i + 5);
		diiLocations.push_back(new DiiLocation(&buffer[i + 3]));
	}
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
