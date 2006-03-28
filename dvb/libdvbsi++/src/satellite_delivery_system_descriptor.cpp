/*
 * $Id: satellite_delivery_system_descriptor.cpp,v 1.6 2006/03/28 17:22:00 ghostrider Exp $
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
#include <dvbsi++/satellite_delivery_system_descriptor.h>

SatelliteDeliverySystemDescriptor::SatelliteDeliverySystemDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(11);

	frequency =
	(
		((buffer[2] >> 4)	* 10000000) +
		((buffer[2] & 0x0F)	* 1000000) +
		((buffer[3] >> 4)	* 100000) +
		((buffer[3] & 0x0F)	* 10000) +
		((buffer[4] >> 4)	* 1000) +
		((buffer[4] & 0x0F)	* 100) +
		((buffer[5] >> 4)	* 10) +
		((buffer[5] & 0x0F)	* 1)
	);

	orbitalPosition = UINT16(&buffer[6]);
	westEastFlag = (buffer[8] >> 7) & 0x01;
	polarization = (buffer[8] >> 5) & 0x03;
	rollOff = (buffer[8] >> 4) & 0x03;
	modulationSystem = (buffer[8] >> 2) & 0x01;
	modulationType = (buffer[8]) & 0x03;

	symbolRate =
	(
		((buffer[9] >> 4)	* 1000000) +
		((buffer[9] & 0x0F)	* 100000) +
		((buffer[10] >> 4)	* 10000) +
		((buffer[10] & 0x0F)	* 1000) +
		((buffer[11] >> 4)	* 100) +
		((buffer[11] & 0x0F)	* 10) +
		((buffer[12] >> 4)	* 1)
	);

	fecInner = buffer[12] & 0x0F;
}

uint32_t SatelliteDeliverySystemDescriptor::getFrequency(void) const
{
	return frequency;
}

uint16_t SatelliteDeliverySystemDescriptor::getOrbitalPosition(void) const
{
	return orbitalPosition;
}

uint8_t SatelliteDeliverySystemDescriptor::getWestEastFlag(void) const
{
	return westEastFlag;
}

uint8_t SatelliteDeliverySystemDescriptor::getPolarization(void) const
{
	return polarization;
}

uint8_t SatelliteDeliverySystemDescriptor::getModulation(void) const
{
	return modulationType;
}

uint8_t SatelliteDeliverySystemDescriptor::getRollOff(void) const
{
	return rollOff;
}

uint8_t SatelliteDeliverySystemDescriptor::getModulationSystem(void) const
{
	return modulationSystem;
}

uint32_t SatelliteDeliverySystemDescriptor::getSymbolRate(void) const
{
	return symbolRate;
}

uint8_t SatelliteDeliverySystemDescriptor::getFecInner(void) const
{
	return fecInner;
}

