/*
 * $Id: service_descriptor.cpp,v 1.2 2005/10/29 00:10:17 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/service_descriptor.h>

ServiceDescriptor::ServiceDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	serviceType = buffer[2];
	serviceProviderNameLength = buffer[3];
	serviceProviderName.assign((char *)&buffer[4], serviceProviderNameLength);
	serviceNameLength = buffer[serviceProviderNameLength + 4];
	serviceName.assign((char *)&buffer[serviceProviderNameLength + 5], serviceNameLength);
}

uint8_t ServiceDescriptor::getServiceType(void) const
{
	return serviceType;
}

const std::string &ServiceDescriptor::getServiceProviderName(void) const
{
	return serviceProviderName;
}

const std::string &ServiceDescriptor::getServiceName(void) const
{
	return serviceName;
}

