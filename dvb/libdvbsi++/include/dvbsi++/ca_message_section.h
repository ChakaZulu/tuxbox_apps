/*
 * $Id: ca_message_section.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __ca_message_section_h__
#define __ca_message_section_h__

#include "short_section.h"

class ConditionalAccessMessageSection : public ShortSection
{
	protected:
		std::list<uint8_t> caDataByte;

	public:
		ConditionalAccessMessageSection(const uint8_t * const buffer);

		static const uint16_t LENGTH = 256;
		static const enum TableId TID = TID_CAMT_ECM_0;
};

typedef std::list<ConditionalAccessMessageSection *> ConditionalAccessMessageSectionList;
typedef ConditionalAccessMessageSectionList::iterator ConditionalAccessMessageSectionIterator;
typedef ConditionalAccessMessageSectionList::const_iterator ConditionalAccessMessageSectionConstIterator;

#endif /* __ca_message_section_h__ */
