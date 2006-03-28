/*
 * $Id: telephone_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/telephone_descriptor.h>

TelephoneDescriptor::TelephoneDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	size_t headerLength = 3;
	ASSERT_MIN_DLEN(headerLength);

	foreignAvailability = (buffer[2] >> 5) & 0x01;
	connectionType = buffer[2] & 0x1f;
	countryPrefixLength = (buffer[3] >> 5) & 0x03;
	internationalAreaCodeLength = (buffer[3] >> 2) & 0x07;
	operatorCodeLength = buffer[3] & 0x03;
	nationalAreaCodeLength = (buffer[4] >> 4) & 0x07;
	coreNumberLength = buffer[4] & 0x0f;

	headerLength += countryPrefixLength;
	headerLength += internationalAreaCodeLength;
	headerLength += operatorCodeLength;
	headerLength += nationalAreaCodeLength;
	headerLength += coreNumberLength;
	ASSERT_MIN_DLEN(headerLength);

	uint16_t offset = 5;
	countryPrefix.assign((char *)&buffer[offset], countryPrefixLength);
	offset += countryPrefixLength;
	internationalAreaCode.assign((char *)&buffer[offset], internationalAreaCodeLength);
	offset += internationalAreaCodeLength;
	operatorCode.assign((char *)&buffer[offset], operatorCodeLength);
	offset += operatorCodeLength;
	nationalAreaCode.assign((char *)&buffer[offset], nationalAreaCodeLength);
	offset += nationalAreaCodeLength;
	coreNumber.assign((char *)&buffer[offset], coreNumberLength);
}

uint8_t TelephoneDescriptor::getForeignAvailability(void) const
{
	return foreignAvailability;
}

uint8_t TelephoneDescriptor::getConnectionType(void) const
{
	return connectionType;
}

const std::string &TelephoneDescriptor::getCountryPrefix(void) const
{
	return countryPrefix;
}

const std::string &TelephoneDescriptor::getInternationalAreaCode(void) const
{
	return internationalAreaCode;
}

const std::string &TelephoneDescriptor::getOperatorCode(void) const
{
	return operatorCode;
}

const std::string &TelephoneDescriptor::getNationalAreaCode(void) const
{
	return nationalAreaCode;
}

const std::string &TelephoneDescriptor::getCoreNumber(void) const
{
	return coreNumber;
}

