/*
 * $Id: long_section.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __long_section_h__
#define __long_section_h__

#include "short_section.h"

class LongSection : public ShortSection
{
	protected:
		unsigned tableIdExtension			: 16;
		unsigned versionNumber				: 5;
		unsigned currentNextIndicator			: 1;
		unsigned sectionNumber				: 8;
		unsigned lastSectionNumber			: 8;

	public:
		LongSection(const uint8_t * const buffer);

		static const uint8_t SYNTAX = 1;

		uint16_t getTableIdExtension(void) const;
		uint8_t getVersionNumber(void) const;
		uint8_t getCurrentNextIndicator(void) const;
		uint8_t getSectionNumber(void) const;
		uint8_t getLastSectionNumber(void) const;

		bool operator< (const LongSection &t) const;
		bool operator> (const LongSection &t) const;
		bool operator<= (const LongSection &t) const;
		bool operator>= (const LongSection &t) const;
		bool operator== (const LongSection &t) const;
		bool operator!= (const LongSection &t) const;
};

typedef std::list<LongSection *> LongSectionList;
typedef LongSectionList::iterator LongSectionIterator;
typedef LongSectionList::const_iterator LongSectionConstIterator;

#endif /* __long_section_h__ */
