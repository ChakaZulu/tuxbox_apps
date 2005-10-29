/*
 * $Id: unknown_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
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
	protected:
		std::vector<uint8_t> dataBytes;

	public:
		UnknownDescriptor(const uint8_t * const buffer);

		virtual size_t writeToBuffer(uint8_t * const buffer) const;
};

#endif /* __unknown_descriptor_h__ */
