/*
 * $Id: short_section.h,v 1.2 2004/02/13 17:51:07 obi Exp $
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

#ifndef __short_section_h__
#define __short_section_h__

#include "compat.h"
#include "packet_id.h"
#include "table_id.h"

class ShortSection
{
	protected:
		unsigned tableId				: 8;
		unsigned sectionSyntaxIndicator			: 1;
		unsigned sectionLength				: 12;

	public:
		ShortSection(const uint8_t * const buffer);

		static const uint8_t CRC32 = 0;
		static const uint16_t LENGTH = 1024;
		static const enum PacketId PID = PID_RESERVED;
		static const uint8_t SYNTAX = 0;
		static const enum TableId TID = TID_RESERVED;
		static const uint32_t TIMEOUT = 0;

		uint8_t getTableId(void) const;
		uint8_t getSectionSyntaxIndicator(void) const;
		uint16_t getSectionLength(void) const;
};

typedef std::vector<ShortSection *> ShortSectionVector;
typedef ShortSectionVector::iterator ShortSectionIterator;
typedef ShortSectionVector::const_iterator ShortSectionConstIterator;

#endif /* __short_section_h__ */
