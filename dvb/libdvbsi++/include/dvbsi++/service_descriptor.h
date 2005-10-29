/*
 * $Id: service_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __service_descriptor_h__
#define __service_descriptor_h__

#include "descriptor.h"

class ServiceDescriptor : public Descriptor
{
	protected:
		unsigned serviceType				: 8;
		unsigned serviceProviderNameLength		: 8;
		std::string serviceProviderName;
		unsigned serviceNameLength			: 8;
		std::string serviceName;

	public:
		ServiceDescriptor(const uint8_t * const buffer);

		uint8_t getServiceType(void) const;
		const std::string &getServiceProviderName(void) const;
		const std::string &getServiceName(void) const;
};

#endif /* __service_descriptor_h__ */
