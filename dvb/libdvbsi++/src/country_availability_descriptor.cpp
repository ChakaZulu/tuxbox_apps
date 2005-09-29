/*
 * $Id: country_availability_descriptor.cpp,v 1.2 2005/09/29 23:49:44 ghostrider Exp $
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

#include <dvbsi++/country_availability_descriptor.h>

CountryAvailabilityDescriptor::CountryAvailabilityDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	std::string countryCode;
	countryAvailabilityFlag = (buffer[2] >> 7) & 0x01;

	for (size_t i = 0; i < descriptorLength - 1; i += 3) {
		countryCode.assign((char *)&buffer[i + 3], 3);
		countryCodes.push_back(countryCode);
	}
}

uint8_t CountryAvailabilityDescriptor::getCountryAvailabilityFlag(void) const
{
	return countryAvailabilityFlag;
}

const CountryCodeList *CountryAvailabilityDescriptor::getCountryCodes(void) const
{
	return &countryCodes;
}

