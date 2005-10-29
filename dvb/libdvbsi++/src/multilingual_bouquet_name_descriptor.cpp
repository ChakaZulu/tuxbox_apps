/*
 * $Id: multilingual_bouquet_name_descriptor.cpp,v 1.4 2005/10/29 00:10:17 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/multilingual_bouquet_name_descriptor.h>

MultilingualBouquetName::MultilingualBouquetName(const uint8_t * const buffer)
{
	iso639LanguageCode.assign((char *)&buffer[0], 3);
	bouquetNameLength = buffer[3];
	bouquetName.assign((char *)&buffer[4], bouquetNameLength);
}

const std::string &MultilingualBouquetName::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

const std::string &MultilingualBouquetName::getBouquetName(void) const
{
	return bouquetName;
}

MultilingualBouquetNameDescriptor::MultilingualBouquetNameDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += buffer[i + 5] + 4)
		multilingualBouquetNames.push_back(new MultilingualBouquetName(&buffer[i + 2]));
}

MultilingualBouquetNameDescriptor::~MultilingualBouquetNameDescriptor(void)
{
	for (MultilingualBouquetNameIterator i = multilingualBouquetNames.begin(); i != multilingualBouquetNames.end(); ++i)
		delete *i;
}

const MultilingualBouquetNameList *MultilingualBouquetNameDescriptor::getMultilingualBouquetNames(void) const
{
	return &multilingualBouquetNames;
}

