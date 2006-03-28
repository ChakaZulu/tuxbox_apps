/*
 * $Id: descriptor.h,v 1.7 2006/03/28 17:21:59 ghostrider Exp $
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

/* To be used only(!) in constructors of descendants of
 * class Descriptor. Might be enhanced to throw an
 * exception in the future.
 */
#define ASSERT_MIN_DLEN(length)			\
do {						\
	if (descriptorLength < (length)) {	\
		valid = false;			\
		return;				\
	}					\
} while (0)

class Descriptor
{
	protected:
		unsigned descriptorTag				: 8;
		unsigned descriptorLength			: 8;
		std::vector<uint8_t> dataBytes;
		bool valid;

	public:
		Descriptor(const uint8_t * const buffer);
		virtual ~Descriptor() { };

		uint8_t getTag(void) const;
		uint8_t getLength(void) const;
		bool isValid(void) const { return valid; }

		size_t writeToBuffer(uint8_t * const buffer) const;
};

typedef std::list<Descriptor *> DescriptorList;
typedef DescriptorList::iterator DescriptorIterator;
typedef DescriptorList::const_iterator DescriptorConstIterator;

#endif /* __descriptor_h__ */
