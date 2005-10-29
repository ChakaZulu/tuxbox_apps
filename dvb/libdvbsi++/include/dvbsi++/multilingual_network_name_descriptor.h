/*
 * $Id: multilingual_network_name_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __multilingual_network_name_descriptor_h__
#define __multilingual_network_name_descriptor_h__

#include "descriptor.h"

class MultilingualNetworkName
{
	protected:
		std::string iso639LanguageCode;
		unsigned networkNameLength			: 8;
		std::string networkName;

	public:
		MultilingualNetworkName(const uint8_t * const buffer);

		const std::string &getIso639LanguageCode(void) const;
		const std::string &getNetworkName(void) const;
};

typedef std::list<MultilingualNetworkName *> MultilingualNetworkNameList;
typedef MultilingualNetworkNameList::iterator MultilingualNetworkNameIterator;
typedef MultilingualNetworkNameList::const_iterator MultilingualNetworkNameConstIterator;

class MultilingualNetworkNameDescriptor : public Descriptor
{
	protected:
		MultilingualNetworkNameList multilingualNetworkNames;

	public:
		MultilingualNetworkNameDescriptor(const uint8_t * const buffer);
		~MultilingualNetworkNameDescriptor(void);

		const MultilingualNetworkNameList *getMultilingualNetworkNames(void) const;
};

#endif /* __multilingual_network_name_descriptor_h__ */
