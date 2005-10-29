/*
 * $Id: multilingual_service_name_descriptor.cpp,v 1.3 2005/10/29 00:10:17 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/multilingual_service_name_descriptor.h>

MultilingualServiceName::MultilingualServiceName(const uint8_t * const buffer)
{
	iso639LanguageCode.assign((char *)&buffer[0], 3);
	serviceProviderNameLength = buffer[3];
	serviceProviderName.assign((char *)&buffer[4], serviceProviderNameLength);
	serviceNameLength = buffer[serviceProviderNameLength + 4];
	serviceName.assign((char *)&buffer[serviceProviderNameLength + 5], serviceNameLength);
}

const std::string &MultilingualServiceName::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

const std::string &MultilingualServiceName::getServiceProviderName(void) const
{
	return serviceProviderName;
}

const std::string &MultilingualServiceName::getServiceName(void) const
{
	return serviceName;
}

MultilingualServiceNameDescriptor::MultilingualServiceNameDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	MultilingualServiceName *name;

	for (size_t i = 0; i < descriptorLength; i += name->serviceProviderNameLength + name->serviceNameLength + 5) {
		name = new MultilingualServiceName(&buffer[i + 2]);
		multilingualServiceNames.push_back(name);
	}
}

MultilingualServiceNameDescriptor::~MultilingualServiceNameDescriptor(void)
{
	for (MultilingualServiceNameIterator i = multilingualServiceNames.begin(); i != multilingualServiceNames.end(); ++i)
		delete *i;
}

const MultilingualServiceNameList *MultilingualServiceNameDescriptor::getMultilingualServiceNames(void) const
{
	return &multilingualServiceNames;
}

