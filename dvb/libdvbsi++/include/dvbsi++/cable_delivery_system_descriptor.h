/*
 * $Id: cable_delivery_system_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __cable_delivery_system_descriptor_h__
#define __cable_delivery_system_descriptor_h__

#include "descriptor.h"

class CableDeliverySystemDescriptor : public Descriptor
{
	protected:
		unsigned frequency				: 32;
		unsigned fecOuter				: 4;
		unsigned modulation				: 8;
		unsigned symbolRate				: 28;
		unsigned fecInner				: 4;

	public:
		CableDeliverySystemDescriptor(const uint8_t * const buffer);

		uint32_t getFrequency(void) const;
		uint8_t getFecOuter(void) const;
		uint8_t getModulation(void) const;
		uint32_t getSymbolRate(void) const;
		uint8_t getFecInner(void) const;
};

#endif /* __cable_delivery_system_descriptor_h__ */
