/*
 * $Id: service_list_descriptor.cpp,v 1.5 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/service_list_descriptor.h>

ServiceListItem::ServiceListItem(const uint8_t * const buffer)
{
	serviceId = UINT16(&buffer[0]);
	serviceType = buffer[2];
}

uint16_t ServiceListItem::getServiceId(void) const
{
	return serviceId;
}

uint8_t ServiceListItem::getServiceType(void) const
{
	return serviceType;
}

ServiceListDescriptor::ServiceListDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 3) {
		ASSERT_MIN_DLEN(i + 3);
		serviceList.push_back(new ServiceListItem(&buffer[i + 2]));
	}
}

ServiceListDescriptor::~ServiceListDescriptor(void)
{
	for (ServiceListItemIterator i = serviceList.begin(); i != serviceList.end(); ++i)
		delete *i;
}

const ServiceListItemList *ServiceListDescriptor::getServiceList(void) const
{
	return &serviceList;
}

