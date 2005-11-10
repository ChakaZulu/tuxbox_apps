/*
 *  $Id: s2_satellite_delivery_system_descriptor.h,v 1.1 2005/11/10 23:55:32 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __s2_satellite_delivery_system_descriptor_h__
#define __s2_satellite_delivery_system_descriptor_h__

#include "descriptor.h"

class S2SatelliteDeliverySystemDescriptor : public Descriptor
{
	protected:
		unsigned scramblingSequenceSelector	 : 1;
		unsigned multipleInputStreamFlag	 : 1;
		unsigned backwardsCompatibilityIndicator : 1;
		unsigned scramblingSequenceIndex	 :18;
		unsigned inputStreamIdentifier	 	 : 8;

	public:
		S2SatelliteDeliverySystemDescriptor(const uint8_t* const buffer);
		virtual ~S2SatelliteDeliverySystemDescriptor();

		uint8_t getScramblingSequenceSelector() const;
		uint8_t getMultipleInputStreamFlag() const;
		uint8_t getBackwardsCompatibilityIndicator() const;
		uint32_t getScramblingSequenceIndex() const;
		uint8_t getInputStreamIdentifier() const;
};

#endif /* __s2_satellite_delivery_system_descriptor_h__*/
