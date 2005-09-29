/*
 * $Id: multilingual_network_name_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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
