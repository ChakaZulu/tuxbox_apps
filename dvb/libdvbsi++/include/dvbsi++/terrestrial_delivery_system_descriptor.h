/*
 * $Id: terrestrial_delivery_system_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __terrestrial_delivery_system_descriptor_h__
#define __terrestrial_delivery_system_descriptor_h__

#include "descriptor.h"

class TerrestrialDeliverySystemDescriptor : public Descriptor
{
	protected:
		unsigned centreFrequency			: 32;
		unsigned bandwidth				: 3;
		unsigned constellation				: 2;
		unsigned hierarchyInformation			: 3;
		unsigned codeRateHpStream			: 3;
		unsigned codeRateLpStream			: 3;
		unsigned guardInterval				: 2;
		unsigned transmissionMode			: 2;
		unsigned otherFrequencyFlag			: 1;

	public:
		TerrestrialDeliverySystemDescriptor(const uint8_t * const buffer);

		uint32_t getCentreFrequency(void) const;
		uint8_t getBandwidth(void) const;
		uint8_t getConstellation(void) const;
		uint8_t getHierarchyInformation(void) const;
		uint8_t getCodeRateHpStream(void) const;
		uint8_t getCodeRateLpStream(void) const;
		uint8_t getGuardInterval(void) const;
		uint8_t getTransmissionMode(void) const;
		uint8_t getOtherFrequencyFlag(void) const;
};

#endif /* __terrestrial_delivery_system_descriptor_h__ */
