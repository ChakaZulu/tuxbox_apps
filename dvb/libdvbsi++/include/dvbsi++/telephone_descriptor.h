/*
 * $Id: telephone_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
