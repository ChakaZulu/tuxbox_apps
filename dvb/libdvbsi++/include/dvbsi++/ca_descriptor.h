/*
 * $Id: ca_descriptor.h,v 1.5 2006/03/28 17:21:59 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __ca_descriptor_h__
#define __ca_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> CaDataByteVector;
typedef CaDataByteVector::iterator CaDataByteIterator;
typedef CaDataByteVector::const_iterator CaDataByteConstIterator;

class CaDescriptor : public Descriptor
{
	protected:
		unsigned caSystemId				: 16;
		unsigned caPid					: 13;
		CaDataByteVector caDataBytes;

	public:
		CaDescriptor(const uint8_t * const buffer);

		uint16_t getCaSystemId(void) const;
		uint16_t getCaPid(void) const;
		const CaDataByteVector *getCaDataBytes(void) const;
};

typedef std::list<CaDescriptor *> CaDescriptorList;
typedef CaDescriptorList::iterator CaDescriptorIterator;
typedef CaDescriptorList::const_iterator CaDescriptorConstIterator;

#endif /* __ca_descriptor_h__ */
