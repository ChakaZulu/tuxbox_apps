/*
 * $Id: network_information_section.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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
