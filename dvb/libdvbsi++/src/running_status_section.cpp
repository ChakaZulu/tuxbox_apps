/*
 * $Id: running_status_section.cpp,v 1.2 2006/09/26 20:54:04 mws Exp $
 *
 * Copyright (C) 2006 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include <dvbsi++/byte_stream.h>
#include <dvbsi++/running_status_section.h>

RunningStatus::RunningStatus(const uint8_t* const buffer)
{
	transportStreamId = UINT16(&buffer[0]);
	originalNetworkId = UINT16(&buffer[2]);
	serviceId = UINT16(&buffer[4]);
	eventId = UINT16(&buffer[6]);
	runningStatus = UINT16(&buffer[8]) & 0x03;
}

uint16_t RunningStatus::getTransportStreamId(void) const
{
	return transportStreamId;
}

uint16_t RunningStatus::getOriginalNetworkId(void) const
{
	return originalNetworkId;
}

uint16_t RunningStatus::getServiceId(void) const
{
	return serviceId;
}

uint16_t RunningStatus::getEventId(void) const
{
	return eventId;
}

uint8_t RunningStatus::getRunningStatus(void) const
{
	return runningStatus;
}

RunningStatusSection::RunningStatusSection(const uint8_t* const buffer) : ShortSection(buffer)
{
	for (size_t i = 0; i < sectionLength; i += 9)
	{
		runningStatus.push_back(new RunningStatus(&buffer[3 + i]));
	}
}

RunningStatusSection::~RunningStatusSection(void)
{
	for( RunningStatusList::iterator it = runningStatus.begin(); it != runningStatus.end(); ++it)
	{
		delete *it;
	}
}

const RunningStatusList* RunningStatusSection::getRunningStatus() const
{
	return &runningStatus;
}
