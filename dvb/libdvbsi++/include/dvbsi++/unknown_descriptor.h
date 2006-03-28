/*
 * $Id: unknown_descriptor.h,v 1.3 2006/03/28 17:21:59 ghostrider Exp $
 *
 * Copyright (C) 2005 Andreas Monzner <andreas.monzner@multimedia-labs.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __unknown_descriptor_h__
#define __unknown_descriptor_h__

#include "descriptor.h"

class UnknownDescriptor : public Descriptor
{
	public:
		UnknownDescriptor(const uint8_t * const buffer) __attribute__ ((deprecated));
} __attribute__ ((deprecated));

#endif /* __unknown_descriptor_h__ */
