/*
 * $Id: ca_section.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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
