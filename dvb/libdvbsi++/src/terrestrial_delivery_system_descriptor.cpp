/*
 * $Id: terrestrial_delivery_system_descriptor.cpp,v 1.1 2004/02/13 15:27:47 obi Exp $
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

#include "byte_stream.h"
#include <dvbsi++/terrestrial_delivery_system_descriptor.h>

TerrestrialDeliverySystemDescriptor::TerrestrialDeliverySystemDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	centreFrequency = UINT32(&buffer[2]);
	bandwidth = (buffer[6] >> 5) & 0x07;
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

