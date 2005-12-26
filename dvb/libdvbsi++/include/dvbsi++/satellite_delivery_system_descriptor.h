/*
 * $Id: satellite_delivery_system_descriptor.h,v 1.4 2005/12/26 20:48:57 mws Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __satellite_delivery_system_descriptor_h__
#define __satellite_delivery_system_descriptor_h__

#include "descriptor.h"

class SatelliteDeliverySystemDescriptor : public Descriptor
{
	protected:
		unsigned frequency				: 32;
		unsigned orbitalPosition			: 16;
		unsigned westEastFlag				: 1;
		unsigned polarization				: 2;
		unsigned rollOff				: 2;
		unsigned modulationSystem			: 1;
		unsigned modulationType				: 2;
		unsigned symbolRate				: 28;
		unsigned fecInner				: 4;

	public:
		SatelliteDeliverySystemDescriptor(const uint8_t * const buffer);

		uint32_t getFrequency(void) const;
		uint16_t getOrbitalPosition(void) const;
		uint8_t getWestEastFlag(void) const;
		uint8_t getPolarization(void) const;
		uint8_t getRollOff(void) const;
		uint8_t getModulationSystem(void) const;
		uint8_t getModulation(void) const;
		uint32_t getSymbolRate(void) const;
		uint8_t getFecInner(void) const;
};

#endif /* __satellite_delivery_system_descriptor_h__ */
