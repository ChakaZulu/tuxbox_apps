/*
 * $Id: cable_delivery_system_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/cable_delivery_system_descriptor.h>

CableDeliverySystemDescriptor::CableDeliverySystemDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
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

	fecOuter = buffer[7] & 0x0F;
	modulation = buffer[8];

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

uint32_t CableDeliverySystemDescriptor::getFrequency(void) const
{
	return frequency;
}

uint8_t CableDeliverySystemDescriptor::getFecOuter(void) const
{
	return fecOuter;
}

uint8_t CableDeliverySystemDescriptor::getModulation(void) const
{
	return modulation;
}

uint32_t CableDeliverySystemDescriptor::getSymbolRate(void) const
{
	return symbolRate;
}

uint8_t CableDeliverySystemDescriptor::getFecInner(void) const
{
	return fecInner;
}

