/*
 * $Id: service_description_section.cpp,v 1.1 2004/02/13 15:27:47 obi Exp $
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

#include <dvbsi++/service_description_section.h>
#include "byte_stream.h"

ServiceDescription::ServiceDescription(const uint8_t * const buffer)
{
	serviceId = UINT16(&buffer[0]);
	eitScheduleFlag = (buffer[2] >> 1) & 0x01;
	eitPresentFollowingFlag = buffer[2] & 0x01;
	runningStatus = (buffer[3] >> 5) & 0x07;
	freeCaMode = (buffer[3] >> 4) & 0x01;
	descriptorsLoopLength = DVB_LENGTH(&buffer[3]);

	for (size_t i = 5; i < descriptorsLoopLength + 5; i += buffer[i + 1] + 2)
		descriptor(&buffer[i]);
}

uint16_t ServiceDescription::getServiceId(void) const
{
	return serviceId;
}

uint8_t ServiceDescription::getEitScheduleFlag(void) const
{
	return eitScheduleFlag;
}

uint8_t ServiceDescription::getEitPresentFollowingFlag(void) const
{
	return eitPresentFollowingFlag;
}

uint8_t ServiceDescription::getRunningStatus(void) const
{
	return runningStatus;
}

uint8_t ServiceDescription::getFreeCaMode(void) const
{
	return freeCaMode;
}

ServiceDescriptionSection::ServiceDescriptionSection(const uint8_t * const buffer) : LongCrcSection (buffer)
{
	originalNetworkId = UINT16(&buffer[8]);

	for (size_t i = 11; i < sectionLength - 1; i += DVB_LENGTH(&buffer[i + 3]) + 5)
		description.push_back(new ServiceDescription(&buffer[i]));
}

ServiceDescriptionSection::~ServiceDescriptionSection(void)
{
	for (ServiceDescriptionIterator i = description.begin(); i != description.end(); ++i)
		delete *i;
}

uint16_t ServiceDescriptionSection::getOriginalNetworkId(void) const
{
	return originalNetworkId;
}

const ServiceDescriptionVector *ServiceDescriptionSection::getDescriptions(void) const
{
	return &description;
}

