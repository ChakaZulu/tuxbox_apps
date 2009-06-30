/*
 * $Id: xait_location_descriptor.h,v 1.1 2009/06/30 07:28:29 mws Exp $
 *
 * Copyright (C) 2009 mws@twisted-brains.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __xait_location_descriptor_h__
#define __xait_location_descriptor_h__

#include "descriptor.h"

class XaitLocationDescriptor : public Descriptor
{
	protected:
		unsigned originalNetworkId			:16;
		unsigned serviceId				:16;
		unsigned versionNumber				: 5;
		unsigned updatePolicy				: 3;

	public:
		XaitLocationDescriptor(const uint8_t * const buffer);
		virtual ~XaitLocationDescriptor() {};

		uint16_t getOriginalNetworkId(void) const;
		uint16_t getServiceId(void) const;
		uint8_t getVersionNumber(void) const;
		uint8_t getUpdatePolicy(void) const;
};

#endif /* __xait_location_descriptor_h__ */
