/*
 * $Id: country_availability_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __country_availability_descriptor_h__
#define __country_availability_descriptor_h__

#include "descriptor.h"

typedef std::list<std::string> CountryCodeList;
typedef CountryCodeList::iterator CountryCodeIterator;
typedef CountryCodeList::const_iterator CountryCodeConstIterator;

class CountryAvailabilityDescriptor : public Descriptor
{
	protected:
		unsigned countryAvailabilityFlag		: 1;
		CountryCodeList countryCodes;

	public:
		CountryAvailabilityDescriptor(const uint8_t * const buffer);

		uint8_t getCountryAvailabilityFlag(void) const;
		const CountryCodeList *getCountryCodes(void) const;
};

#endif /* __country_availability_descriptor_h__ */
