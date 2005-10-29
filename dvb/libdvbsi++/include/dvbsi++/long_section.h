/*
 * $Id: long_section.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
