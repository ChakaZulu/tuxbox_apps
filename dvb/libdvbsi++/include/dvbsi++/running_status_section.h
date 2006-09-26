/*
 *  $Id: running_status_section.h,v 1.2 2006/09/26 20:54:04 mws Exp $
 *
 *  Copyright (C) 2006 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __running_status_section_h__
#define __running_status_section_h__

#include "short_section.h"

class RunningStatus
{
	protected:
		unsigned transportStreamId		: 16;
		unsigned originalNetworkId		: 16;
		unsigned serviceId			: 16;
		unsigned eventId			: 16;
		unsigned runningStatus			:  3;
	public:
		RunningStatus(const uint8_t* const buffer);
		virtual ~RunningStatus(){}

		uint16_t getTransportStreamId(void) const;
		uint16_t getOriginalNetworkId(void) const;
		uint16_t getServiceId(void) const;
		uint16_t getEventId(void) const;
		uint8_t getRunningStatus(void) const;
};

typedef std::list<RunningStatus*> RunningStatusList;
typedef RunningStatusList::iterator RunningStatusIterator;
typedef RunningStatusList::const_iterator RunningStatusConstIterator;

class RunningStatusSection : public ShortSection
{
	protected:
		RunningStatusList runningStatus;

	public:
		RunningStatusSection(const uint8_t* const buffer);
		virtual ~RunningStatusSection();

		static const enum PacketId PID = PID_RST;
		static const enum TableId TID = TID_RST;
		static const uint32_t TIMEOUT = 36000;

		const RunningStatusList* getRunningStatus(void) const;
};

typedef std::list<RunningStatusSection*> RunningStatusSectionList;
typedef RunningStatusSectionList::iterator RunningStatusSectionIterator;
typedef RunningStatusSectionList::const_iterator RunningStatusSectionConstIterator;

#endif /* __running_status_section_h__ */
