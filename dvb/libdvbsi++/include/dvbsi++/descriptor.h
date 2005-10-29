/*
 * $Id: descriptor.h,v 1.6 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __descriptor_h__
#define __descriptor_h__

#include "compat.h"

class Descriptor
{
	protected:
		unsigned descriptorTag				: 8;
		unsigned descriptorLength			: 8;

	public:
		Descriptor(const uint8_t * const buffer);
		virtual ~Descriptor() { };

		uint8_t getTag(void) const;
		uint8_t getLength(void) const;

		virtual size_t writeToBuffer(uint8_t * const buffer) const;
};

typedef std::list<Descriptor *> DescriptorList;
typedef DescriptorList::iterator DescriptorIterator;
typedef DescriptorList::const_iterator DescriptorConstIterator;

#endif /* __descriptor_h__ */
