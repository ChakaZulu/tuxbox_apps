/*
 *  $Id: transport_stream_description_section.h,v 1.1 2006/02/23 19:12:39 mws Exp $
 *
 *  Copyright (C) 2006 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __transport_stream_description_section_h__
#define __transport_stream_description_section_h__

#include "descriptor_container.h"
#include "long_crc_section.h"

class TransportStreamDescriptionSection : public LongCrcSection, public DescriptorContainer
{
	public:
		TransportStreamDescriptionSection(const uint8_t* const buffer);
		virtual ~TransportStreamDescriptionSection(){}

		static const enum PacketId PID = PID_TSDT;
		static const enum TableId TID = TID_TSDT;
		static const uint32_t TIMEOUT = 200;
};

typedef std::list<TransportStreamDescriptionSection*> TransportStreamDescriptionSectionList;
typedef TransportStreamDescriptionSectionList::iterator TransportStreamDescriptionSectionIterator;
typedef TransportStreamDescriptionSectionList::const_iterator TransportStreamDescriptionSectionConstIterator;

#endif /* __transport_stream_description_section_h__ */
