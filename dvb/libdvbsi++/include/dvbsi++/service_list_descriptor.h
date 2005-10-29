/*
 * $Id: service_list_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __service_list_descriptor_h__
#define __service_list_descriptor_h__

#include "descriptor.h"

class ServiceListItem
{
	protected:
		unsigned serviceId				: 16;
		unsigned serviceType				: 8;

	public:
		ServiceListItem(const uint8_t * const buffer);

		uint16_t getServiceId(void) const;
		uint8_t getServiceType(void) const;
};

typedef std::list<ServiceListItem *> ServiceListItemList;
typedef ServiceListItemList::iterator ServiceListItemIterator;
typedef ServiceListItemList::const_iterator ServiceListItemConstIterator;

class ServiceListDescriptor : public Descriptor
{
	protected:
		ServiceListItemList serviceList;

	public:
		ServiceListDescriptor(const uint8_t * const buffer);
		~ServiceListDescriptor(void);

		const ServiceListItemList *getServiceList(void) const;
};

#endif /* __service_list_descriptor_h__ */
