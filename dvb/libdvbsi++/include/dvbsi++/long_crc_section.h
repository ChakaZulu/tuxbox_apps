/*
 * $Id: long_crc_section.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __long_crc_section_h__
#define __long_crc_section_h__

#include "long_section.h"

class LongCrcSection : public LongSection
{
	protected:
		unsigned crc32					: 32;

	public:
		LongCrcSection(const uint8_t * const buffer);

		static const uint8_t CRC32 = 1;

		uint32_t getCrc32(void) const;
};

typedef std::list<LongCrcSection *> LongCrcSectionList;
typedef LongCrcSectionList::iterator LongCrcSectionIterator;
typedef LongCrcSectionList::const_iterator LongCrcSectionConstIterator;

#endif /* __long_crc_section_h__ */
