/*
 * $Id: multilingual_bouquet_name_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __multilingual_bouquet_name_descriptor_h__
#define __multilingual_bouquet_name_descriptor_h__

#include "descriptor.h"

class MultilingualBouquetName
{
	protected:
		std::string iso639LanguageCode;
		unsigned bouquetNameLength			: 8;
		std::string bouquetName;

	public:
		MultilingualBouquetName(const uint8_t * const buffer);

		const std::string &getIso639LanguageCode(void) const;
		const std::string &getBouquetName(void) const;
};

typedef std::list<MultilingualBouquetName *> MultilingualBouquetNameList;
typedef MultilingualBouquetNameList::iterator MultilingualBouquetNameIterator;
typedef MultilingualBouquetNameList::const_iterator MultilingualBouquetNameConstIterator;

class MultilingualBouquetNameDescriptor : public Descriptor
{
	protected:
		MultilingualBouquetNameList multilingualBouquetNames;

	public:
		MultilingualBouquetNameDescriptor(const uint8_t * const buffer);
		~MultilingualBouquetNameDescriptor(void);

		const MultilingualBouquetNameList *getMultilingualBouquetNames(void) const;
};

#endif /* __multilingual_bouquet_name_descriptor_h__ */
