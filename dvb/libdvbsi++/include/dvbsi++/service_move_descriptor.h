/*
 * $Id: service_move_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __service_move_descriptor_h__
#define __service_move_descriptor_h__

#include "descriptor.h"

class ServiceMoveDescriptor : public Descriptor
{
	protected:
		unsigned newOriginalNetworkId			: 16;
		unsigned newTransportStreamId			: 16;
		unsigned newServiceId				: 16;

	public:
		ServiceMoveDescriptor(const uint8_t * const buffer);

		uint16_t getNewOriginalNetworkId(void) const;
		uint16_t getNewTransportStreamId(void) const;
		uint16_t getNewServiceId(void) const;
};

#endif /* __service_move_descriptor_h__ */
