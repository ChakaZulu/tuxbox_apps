/*
 * $Id: multilingual_service_name_descriptor.h,v 1.1 2004/02/13 15:27:38 obi Exp $
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

typedef std::vector<MultilingualServiceName *> MultilingualServiceNameVector;
typedef MultilingualServiceNameVector::iterator MultilingualServiceNameIterator;
typedef MultilingualServiceNameVector::const_iterator MultilingualServiceNameConstIterator;

class MultilingualServiceNameDescriptor : public Descriptor
{
	protected:
		MultilingualServiceNameVector multilingualServiceNames;

	public:
		MultilingualServiceNameDescriptor(const uint8_t * const buffer);
		~MultilingualServiceNameDescriptor(void);

		const MultilingualServiceNameVector *getMultilingualServiceNames(void) const;
};

#endif /* __multilingual_service_name_descriptor_h__ */
