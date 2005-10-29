/*
 * $Id: multilingual_network_name_descriptor.cpp,v 1.3 2005/10/29 00:10:17 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/multilingual_network_name_descriptor.h>

MultilingualNetworkName::MultilingualNetworkName(const uint8_t * const buffer)
{
	iso639LanguageCode.assign((char *)&buffer[0], 3);
	networkNameLength = buffer[3];
	networkName.assign((char *)&buffer[4], networkNameLength);
}

const std::string &MultilingualNetworkName::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

const std::string &MultilingualNetworkName::getNetworkName(void) const
{
	return networkName;
}

MultilingualNetworkNameDescriptor::MultilingualNetworkNameDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += buffer[i + 5] + 4)
		multilingualNetworkNames.push_back(new MultilingualNetworkName(&buffer[i + 2]));
}

MultilingualNetworkNameDescriptor::~MultilingualNetworkNameDescriptor(void)
{
	for (MultilingualNetworkNameIterator i = multilingualNetworkNames.begin(); i != multilingualNetworkNames.end(); ++i)
		delete *i;
}

const MultilingualNetworkNameList *MultilingualNetworkNameDescriptor::getMultilingualNetworkNames(void) const
{
	return &multilingualNetworkNames;
}

