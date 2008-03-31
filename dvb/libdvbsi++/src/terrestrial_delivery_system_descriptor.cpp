/*
 * $Id: terrestrial_delivery_system_descriptor.cpp,v 1.5 2008/03/31 07:49:43 mws Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/terrestrial_delivery_system_descriptor.h>

TerrestrialDeliverySystemDescriptor::TerrestrialDeliverySystemDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(7);

	centreFrequency = UINT32(&buffer[2]);
	bandwidth = (buffer[6] >> 5) & 0x07;
	priority = (buffer[6] >> 4) & 0x01;
	timeSlicingIndicator = (buffer[6] >> 3) & 0x01;
	mpeFecIndicator = (buffer[6] >> 2) & 0x01;
	constellation = (buffer[7] >> 6) & 0x03;
	hierarchyInformation = (buffer[7] >> 3) & 0x07;
	codeRateHpStream = buffer[7] & 0x07;
	codeRateLpStream = (buffer[8] >> 5) & 0x07;
	guardInterval = (buffer[8] >> 3) & 0x03;
	transmissionMode = (buffer[8] >> 1) & 0x03;
	otherFrequencyFlag = buffer[8] & 0x01;
}

uint32_t TerrestrialDeliverySystemDescriptor::getCentreFrequency(void) const
{
	return centreFrequency;
}

uint8_t TerrestrialDeliverySystemDescriptor::getBandwidth(void) const
{
	return bandwidth;
}

uint8_t TerrestrialDeliverySystemDescriptor::getPriority(void) const
{
	return priority;
}

uint8_t TerrestrialDeliverySystemDescriptor::getTimeSlicingIndicator(void) const
{
	return timeSlicingIndicator;
}

uint8_t TerrestrialDeliverySystemDescriptor::getMpeFecIndicator(void) const
{
	return mpeFecIndicator;
}

uint8_t TerrestrialDeliverySystemDescriptor::getConstellation(void) const
{
	return constellation;
}

uint8_t TerrestrialDeliverySystemDescriptor::getHierarchyInformation(void) const
{
	return hierarchyInformation;
}

uint8_t TerrestrialDeliverySystemDescriptor::getCodeRateHpStream(void) const
{
	return codeRateHpStream;
}

uint8_t TerrestrialDeliverySystemDescriptor::getCodeRateLpStream(void) const
{
	return codeRateLpStream;
}

uint8_t TerrestrialDeliverySystemDescriptor::getGuardInterval(void) const
{
	return guardInterval;
}

uint8_t TerrestrialDeliverySystemDescriptor::getTransmissionMode(void) const
{
	return transmissionMode;
}

uint8_t TerrestrialDeliverySystemDescriptor::getOtherFrequencyFlag(void) const
{
	return otherFrequencyFlag;
}
