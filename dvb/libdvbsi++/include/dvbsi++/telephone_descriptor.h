/*
 * $Id: telephone_descriptor.h,v 1.1 2004/02/13 15:27:38 obi Exp $
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

#ifndef __telephone_descriptor_h__
#define __telephone_descriptor_h__

#include "descriptor.h"

class TelephoneDescriptor : public Descriptor
{
	protected:
		unsigned foreignAvailability			: 1;
		unsigned connectionType				: 5;
		unsigned countryPrefixLength			: 2;
		unsigned internationalAreaCodeLength		: 3;
		unsigned operatorCodeLength			: 2;
		unsigned nationalAreaCodeLength			: 3;
		unsigned coreNumberLength			: 4;
		std::string countryPrefix;
		std::string internationalAreaCode;
		std::string operatorCode;
		std::string nationalAreaCode;
		std::string coreNumber;

	public:
		TelephoneDescriptor(const uint8_t * const buffer);

		uint8_t getForeignAvailability(void) const;
		uint8_t getConnectionType(void) const;
		const std::string &getCountryPrefix(void) const;
		const std::string &getInternationalAreaCode(void) const;
		const std::string &getOperatorCode(void) const;
		const std::string &getNationalAreaCode(void) const;
		const std::string &getCoreNumber(void) const;
};

#endif /* __telephone_descriptor_h__ */
