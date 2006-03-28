/*
 * $Id: event_information_section.cpp,v 1.6 2006/03/28 17:22:00 ghostrider Exp $
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
#include <dvbsi++/event_information_section.h>

Event::Event(const uint8_t * const buffer)
{
	eventId = UINT16(&buffer[0]);
	startTimeMjd = UINT16(&buffer[2]);
	startTimeBcd = (buffer[4] << 16) | UINT16(&buffer[5]);
	duration = (buffer[7] << 16) | UINT16(&buffer[8]);
	runningStatus = (buffer[10] >> 5) & 0x07;
	freeCaMode = (buffer[10] >> 4) & 0x01;
	descriptorsLoopLength = DVB_LENGTH(&buffer[10]);

	for (size_t i = 12; i < descriptorsLoopLength + 12; i += buffer[i + 1] + 2)
		descriptor(&buffer[i], SCOPE_SI);
}

uint16_t Event::getEventId(void) const
{
	return eventId;
}

uint16_t Event::getStartTimeMjd(void) const
{
	return startTimeMjd;
}

uint32_t Event::getStartTimeBcd(void) const
{
	return startTimeBcd;
}

uint32_t Event::getDuration(void) const
{
	return duration;
}

uint8_t Event::getRunningStatus(void) const
{
	return runningStatus;
}

uint8_t Event::getFreeCaMode(void) const
{
	return freeCaMode;
}

EventInformationSection::EventInformationSection(const uint8_t * const buffer) : LongCrcSection(buffer)
{
	transportStreamId = sectionLength > 10 ? UINT16(&buffer[8]) : 0;
	originalNetworkId = sectionLength > 12 ? UINT16(&buffer[10]) : 0;
	segmentLastSectionNumber = sectionLength > 13 ? buffer[12] : sectionNumber;
	lastTableId = sectionLength > 14 ? buffer[13] : buffer[0];

	uint16_t pos = 14;
	uint16_t bytesLeft = sectionLength > 15 ? sectionLength-15 : 0;
	uint16_t loopLength=0;

	while (bytesLeft > 11 && bytesLeft >= (loopLength = 12 + DVB_LENGTH(&buffer[pos+10]))) {
		events.push_back(new Event(&buffer[pos]));	
		bytesLeft -= loopLength;
		pos += loopLength;
	}
}

EventInformationSection::~EventInformationSection(void)
{
	for (EventIterator i = events.begin(); i != events.end(); ++i)
		delete *i;
}

uint16_t EventInformationSection::getTransportStreamId(void) const
{
	return transportStreamId;
}

uint16_t EventInformationSection::getOriginalNetworkId(void) const
{
	return originalNetworkId;
}

uint8_t EventInformationSection::getSegmentLastSectionNumber(void) const
{
	return segmentLastSectionNumber;
}

uint8_t EventInformationSection::getLastTableId(void) const
{
	return lastTableId;
}

const EventList *EventInformationSection::getEvents(void) const
{
	return &events;
}

