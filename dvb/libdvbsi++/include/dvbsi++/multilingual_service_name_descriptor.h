/*
 * $Id: multilingual_service_name_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
