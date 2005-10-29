/*
 * $Id: service_description_section.cpp,v 1.5 2005/10/29 00:10:17 obi Exp $
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
#include <dvbsi++/service_description_section.h>

ServiceDescription::ServiceDescription(const uint8_t * const buffer)
{
	serviceId = UINT16(&buffer[0]);
	eitScheduleFlag = (buffer[2] >> 1) & 0x01;
	eitPresentFollowingFlag = buffer[2] & 0x01;
	runningStatus = (buffer[3] >> 5) & 0x07;
	freeCaMode = (buffer[3] >> 4) & 0x01;
	descriptorsLoopLength = DVB_LENGTH(&buffer[3]);

	for (size_t i = 5; i < descriptorsLoopLength + 5; i += buffer[i + 1] + 2)
		descriptor(&buffer[i], SCOPE_SI);
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

uint16_t ServiceDescriptionSection::getTransportStreamId(void) const
{
	return getTableIdExtension();
}

const ServiceDescriptionList *ServiceDescriptionSection::getDescriptions(void) const
{
	return &description;
}

