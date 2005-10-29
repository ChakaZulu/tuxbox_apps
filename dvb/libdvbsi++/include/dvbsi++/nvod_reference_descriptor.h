/*
 * $Id: nvod_reference_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __nvod_reference_descriptor_h__
#define __nvod_reference_descriptor_h__

#include "descriptor.h"

class NvodReference
{
	protected:
		unsigned transportStreamId			: 16;
		unsigned originalNetworkId			: 16;
		unsigned serviceId				: 16;

	public:
		NvodReference(const uint8_t * const buffer);

		uint16_t getTransportStreamId(void) const;
		uint16_t getOriginalNetworkId(void) const;
		uint16_t getServiceId(void) const;
};

typedef std::list<NvodReference *> NvodReferenceList;
typedef NvodReferenceList::iterator NvodReferenceIterator;
typedef NvodReferenceList::const_iterator NvodReferenceConstIterator;

class NvodReferenceDescriptor : public Descriptor
{
	protected:
		NvodReferenceList nvodReferences;

	public:
		NvodReferenceDescriptor(const uint8_t * const buffer);
		~NvodReferenceDescriptor(void);

		const NvodReferenceList* getNvodReferences(void) const;
};

#endif /* __nvod_reference_descriptor_h__ */
