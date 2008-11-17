/*
 *  $Id: service_availability_descriptor.h,v 1.2 2008/11/17 17:01:30 ghostrider Exp $
 *
 *  Copyright (C) 2008 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __service_availability_descriptor_h__
#define __service_availability_descriptor_h__

#include "descriptor.h"

typedef std::list<uint16_t> CellIdList;
typedef CellIdList::iterator CellIdIterator;
typedef CellIdList::const_iterator CellIdConstIterator;

class ServiceAvailabilityDescriptor : public Descriptor
{
	protected:
		unsigned availabilityFlag		: 1;
		CellIdList cellIds;

	public:
		ServiceAvailabilityDescriptor(const uint8_t* const buffer);

		uint8_t getAvailabilityFlag() const;
		const CellIdList *getCellIds(void) const;
};

#endif /* __service_availability_descriptor_h__*/
