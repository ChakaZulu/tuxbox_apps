/*
 * $Id: location_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __location_descriptor_h__
#define __location_descriptor_h__

#include "descriptor.h"

class LocationDescriptor : public Descriptor
{
	protected:
		unsigned locationTag				: 8;

	public:
		LocationDescriptor(const uint8_t * const buffer);

		uint8_t getLocationTag(void) const;
};

#endif /* __location_descriptor_h__ */
