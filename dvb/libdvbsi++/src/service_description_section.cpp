/*
 * $Id: service_description_section.cpp,v 1.7 2006/09/26 20:13:58 mws Exp $
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
	originalNetworkId = sectionLength > 9 ? UINT16(&buffer[8]) : 0;

	uint16_t pos = 11;
	uint16_t bytesLeft = sectionLength > 12 ? sectionLength - 12 : 0;
	uint16_t loopLength = 0;

	while (bytesLeft > 4 && bytesLeft >= (loopLength = 5 + DVB_LENGTH(&buffer[pos+3]))) {
		description.push_back(new ServiceDescription(&buffer[pos]));
		bytesLeft -= loopLength;
		pos += loopLength;
	}
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

