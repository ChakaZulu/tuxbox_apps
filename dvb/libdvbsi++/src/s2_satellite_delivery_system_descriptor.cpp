/*
 * $Id: s2_satellite_delivery_system_descriptor.cpp,v 1.1 2005/11/10 23:55:33 mws Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/s2_satellite_delivery_system_descriptor.h"

#include "dvbsi++/byte_stream.h"

S2SatelliteDeliverySystemDescriptor::S2SatelliteDeliverySystemDescriptor(const uint8_t* const buffer) : Descriptor(buffer)
{
	scramblingSequenceSelector = (buffer[2] >> 7) & 0x01;
	multipleInputStreamFlag = (buffer[2] >> 6) & 0x01;
	backwardsCompatibilityIndicator = (buffer[2] >> 5) & 0x01;
	size_t i = 3;
	if (scramblingSequenceSelector == 1)
	{
		scramblingSequenceIndex = (buffer[i++] & 0x3) << 16;
		scramblingSequenceIndex |= buffer[i++] << 8;
		scramblingSequenceIndex |= buffer[i++];
	}
	else
	{
		scramblingSequenceIndex = 0;
	}

	if ( multipleInputStreamFlag == 1 )
	{
		inputStreamIdentifier = buffer[i];
	}
	else
	{
		inputStreamIdentifier = 0;
	}
}

S2SatelliteDeliverySystemDescriptor::~S2SatelliteDeliverySystemDescriptor()
{
}

uint8_t S2SatelliteDeliverySystemDescriptor::getScramblingSequenceSelector() const
{
	return scramblingSequenceSelector;
}

uint8_t S2SatelliteDeliverySystemDescriptor::getMultipleInputStreamFlag() const
{
	return multipleInputStreamFlag;
}

uint8_t S2SatelliteDeliverySystemDescriptor::getBackwardsCompatibilityIndicator() const
{
	return backwardsCompatibilityIndicator;
}

uint32_t S2SatelliteDeliverySystemDescriptor::getScramblingSequenceIndex() const
{
	return scramblingSequenceIndex;
}

uint8_t S2SatelliteDeliverySystemDescriptor::getInputStreamIdentifier() const
{
	return inputStreamIdentifier;
}
