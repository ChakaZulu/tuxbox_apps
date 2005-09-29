/*
 * $Id: multilingual_service_name_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __multilingual_service_name_descriptor_h__
#define __multilingual_service_name_descriptor_h__

#include "descriptor.h"

class MultilingualServiceName
{
	protected:
		std::string iso639LanguageCode;
		unsigned serviceProviderNameLength		: 8;
		std::string serviceProviderName;
		unsigned serviceNameLength			: 8;
		std::string serviceName;

	public:
		MultilingualServiceName(const uint8_t * const buffer);

		const std::string &getIso639LanguageCode(void) const;
		const std::string &getServiceProviderName(void) const;
		const std::string &getServiceName(void) const;

	friend class MultilingualServiceNameDescriptor;
};

typedef std::list<MultilingualServiceName *> MultilingualServiceNameList;
typedef MultilingualServiceNameList::iterator MultilingualServiceNameIterator;
typedef MultilingualServiceNameList::const_iterator MultilingualServiceNameConstIterator;

class MultilingualServiceNameDescriptor : public Descriptor
{
	protected:
		MultilingualServiceNameList multilingualServiceNames;

	public:
		MultilingualServiceNameDescriptor(const uint8_t * const buffer);
		~MultilingualServiceNameDescriptor(void);

		const MultilingualServiceNameList *getMultilingualServiceNames(void) const;
};

#endif /* __multilingual_service_name_descriptor_h__ */
