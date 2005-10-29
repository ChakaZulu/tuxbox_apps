/*
 * $Id: ca_section.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __ca_section_h__
#define __ca_section_h__

#include "descriptor_container.h"
#include "long_crc_section.h"

class ConditionalAccessSection : public LongCrcSection, public DescriptorContainer
{
	public:
		ConditionalAccessSection(const uint8_t * const buffer);

		static const enum PacketId PID = PID_CAT;
		static const enum TableId TID = TID_CAT;
		static const uint32_t TIMEOUT = 200;
};

typedef std::list<ConditionalAccessSection *> ConditionalAccessSectionList;
typedef ConditionalAccessSectionList::iterator ConditionalAccessSectionIterator;
typedef ConditionalAccessSectionList::const_iterator ConditionalAccessSectionConstIterator;

#endif /* __ca_section_h__ */
