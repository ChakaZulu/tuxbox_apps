/*
 * $Id: dii_location_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
 
#ifndef __dii_location_descriptor_h__
#define __dii_location_descriptor_h__

#include "descriptor.h"

class DiiLocation
{
	protected:
		unsigned diiIdentification			: 15;
		unsigned associationTag				: 16;

	public:
		DiiLocation(const uint8_t * const buffer);

		uint16_t getDiiIdentification(void) const;
		uint16_t getAssociationTag(void) const;
};

typedef std::list<DiiLocation *> DiiLocationList;
typedef DiiLocationList::iterator DiiLocationIterator;
typedef DiiLocationList::const_iterator DiiLocationConstIterator;

class DiiLocationDescriptor : public Descriptor
{
	protected:
		unsigned transportProtocolLabel			: 8;
		DiiLocationList diiLocations;

	public:
		DiiLocationDescriptor(const uint8_t * const buffer);
		~DiiLocationDescriptor(void);
		
		uint8_t getTransportProtocolLabel(void) const;
		const DiiLocationList *getDiiLocations(void) const;
};

#endif /* __dii_location_descriptor_h__ */
