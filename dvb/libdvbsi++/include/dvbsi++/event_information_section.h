/*
 * $Id: event_information_section.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __event_information_section_h__
#define __event_information_section_h__

#include "descriptor_container.h"
#include "long_crc_section.h"

class Event : public DescriptorContainer
{
	protected:
		unsigned eventId				: 16;
		unsigned startTimeMjd				: 16;
		unsigned startTimeBcd				: 24;
		unsigned duration				: 24;
		unsigned runningStatus				: 3;
		unsigned freeCaMode				: 1;
		unsigned descriptorsLoopLength			: 12;

	public:
		Event(const uint8_t * const buffer);

		uint16_t getEventId(void) const;
		uint16_t getStartTimeMjd(void) const;
		uint32_t getStartTimeBcd(void) const;
		uint32_t getDuration(void) const;
		uint8_t getRunningStatus(void) const;
		uint8_t getFreeCaMode(void) const;
};

typedef std::list<Event *> EventList;
typedef EventList::iterator EventIterator;
typedef EventList::const_iterator EventConstIterator;

class EventInformationSection : public LongCrcSection
{
	protected:
		unsigned transportStreamId			: 16;
		unsigned originalNetworkId			: 16;
		unsigned segmentLastSectionNumber		: 8;
		unsigned lastTableId				: 8;
		EventList events;

	public:
		EventInformationSection(const uint8_t * const buffer);
		~EventInformationSection(void);

		static const uint16_t LENGTH = 4096;
		static const enum PacketId PID = PID_EIT;
		static const enum TableId TID = TID_EIT_ACTUAL;
		static const uint32_t TIMEOUT = 3000;

		uint16_t getTransportStreamId(void) const;
		uint16_t getOriginalNetworkId(void) const;
		uint8_t getSegmentLastSectionNumber(void) const;
		uint8_t getLastTableId(void) const;
		const EventList *getEvents(void) const;
};

typedef std::list<EventInformationSection *> EventInformationSectionList;
typedef EventInformationSectionList::iterator EventInformationSectionIterator;
typedef EventInformationSectionList::const_iterator EventInformationSectionConstIterator;

#endif /* __event_information_section_h__ */
