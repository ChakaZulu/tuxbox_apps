/*
 * $Id: service_description_section.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __service_description_section_h__
#define __service_description_section_h__

#include "descriptor_container.h"
#include "long_crc_section.h"

class ServiceDescription : public DescriptorContainer
{
	protected:
		unsigned serviceId				: 16;
		unsigned eitScheduleFlag			: 1;
		unsigned eitPresentFollowingFlag		: 1;
		unsigned runningStatus				: 3;
		unsigned freeCaMode				: 1;
		unsigned descriptorsLoopLength			: 12;

	public:
		ServiceDescription(const uint8_t * const buffer);

		uint16_t getServiceId(void) const;
		uint8_t getEitScheduleFlag(void) const;
		uint8_t getEitPresentFollowingFlag(void) const;
		uint8_t getRunningStatus(void) const;
		uint8_t getFreeCaMode(void) const;
};

typedef std::list<ServiceDescription *> ServiceDescriptionList;
typedef ServiceDescriptionList::iterator ServiceDescriptionIterator;
typedef ServiceDescriptionList::const_iterator ServiceDescriptionConstIterator;

class ServiceDescriptionSection : public LongCrcSection
{
	protected:
		unsigned originalNetworkId			: 16;
		ServiceDescriptionList description;

	public:
		ServiceDescriptionSection(const uint8_t * const buffer);
		~ServiceDescriptionSection(void);

		static const enum PacketId PID = PID_SDT;
		static const enum TableId TID = TID_SDT_ACTUAL;
		static const uint32_t TIMEOUT = 3000;

		uint16_t getOriginalNetworkId(void) const;
		uint16_t getTransportStreamId(void) const;
		const ServiceDescriptionList *getDescriptions(void) const;
};

typedef std::list<ServiceDescriptionSection *> ServiceDescriptionSectionList;
typedef ServiceDescriptionSectionList::iterator ServiceDescriptionSectionIterator;
typedef ServiceDescriptionSectionList::const_iterator ServiceDescriptionSectionConstIterator;

#endif /* __service_description_section_h__ */
