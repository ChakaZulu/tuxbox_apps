/*
 * $Id: service_list_descriptor.cpp,v 1.2 2004/02/13 17:51:08 obi Exp $
 *
 * Copyright (C) 2002-2004 Andreas Oberritter <obi@saftware.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
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
	for (size_t i = 0; i < descriptorLength; i += 3)
		serviceList.push_back(new ServiceListItem(&buffer[i + 2]));
}

ServiceListDescriptor::~ServiceListDescriptor(void)
{
	for (ServiceListItemIterator i = serviceList.begin(); i != serviceList.end(); ++i)
		delete *i;
}

const ServiceListItemVector *ServiceListDescriptor::getServiceList(void) const
{
	return &serviceList;
}

