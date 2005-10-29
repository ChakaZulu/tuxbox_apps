/*
 * $Id: network_information_section.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __network_information_section_h__
#define __network_information_section_h__

#include "descriptor_container.h"
#include "long_crc_section.h"

class TransportStreamInfo : public DescriptorContainer
{
	protected:
		unsigned transportStreamId			: 16;
		unsigned originalNetworkId			: 16;
		unsigned transportDescriptorsLength		: 12;

	public:
		TransportStreamInfo(const uint8_t * const buffer);

		uint16_t getTransportStreamId(void) const;
		uint16_t getOriginalNetworkId(void) const;
};

typedef std::list<TransportStreamInfo *> TransportStreamInfoList;
typedef TransportStreamInfoList::iterator TransportStreamInfoIterator;
typedef TransportStreamInfoList::const_iterator TransportStreamInfoConstIterator;

class NetworkInformationSection : public LongCrcSection, public DescriptorContainer
{
	protected:
		unsigned networkDescriptorsLength		: 12;
		unsigned transportStreamLoopLength		: 12;
		TransportStreamInfoList tsInfo;

	public:
		NetworkInformationSection(const uint8_t * const buffer);
		~NetworkInformationSection(void);

		static const enum PacketId PID = PID_NIT;
		static const enum TableId TID = TID_NIT_ACTUAL;
		static const uint32_t TIMEOUT = 12000;

		const TransportStreamInfoList *getTsInfo(void) const;
};

typedef std::list<NetworkInformationSection *> NetworkInformationSectionList;
typedef NetworkInformationSectionList::iterator NetworkInformationSectionIterator;
typedef NetworkInformationSectionList::const_iterator NetworkInformationSectionConstIterator;

#endif /* __network_information_section_h__ */
