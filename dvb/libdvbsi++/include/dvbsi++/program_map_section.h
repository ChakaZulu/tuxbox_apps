/*
 * $Id: program_map_section.h,v 1.4 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __program_map_section_h__
#define __program_map_section_h__

#include "descriptor_container.h"
#include "long_crc_section.h"

class ElementaryStreamInfo : public DescriptorContainer
{
	protected:
		unsigned streamType				: 8;
		unsigned elementaryPid				: 13;
		unsigned esInfoLength				: 12;

	public:
		ElementaryStreamInfo(const uint8_t * const buffer);

		uint8_t getType(void) const;
		uint16_t getPid(void) const;

	friend class CaElementaryStreamInfo;

};

typedef std::list<ElementaryStreamInfo *> ElementaryStreamInfoList;
typedef ElementaryStreamInfoList::iterator ElementaryStreamInfoIterator;
typedef ElementaryStreamInfoList::const_iterator ElementaryStreamInfoConstIterator;

class ProgramMapSection : public LongCrcSection, public DescriptorContainer
{
	protected:
		unsigned pcrPid					: 13;
		unsigned programInfoLength			: 12;
		ElementaryStreamInfoList esInfo;

	public:
		ProgramMapSection(const uint8_t * const buffer);
		~ProgramMapSection(void);

		static const enum TableId TID = TID_PMT;
		static const uint32_t TIMEOUT = 600;

		uint16_t getPcrPid(void) const;
		uint16_t getProgramNumber(void) const;
		const ElementaryStreamInfoList *getEsInfo(void) const;

	friend class CaProgramMapSection;
};

typedef std::list<ProgramMapSection *> ProgramMapSectionList;
typedef ProgramMapSectionList::iterator ProgramMapSectionIterator;
typedef ProgramMapSectionList::const_iterator ProgramMapSectionConstIterator;

#endif /* __program_map_section_h__ */
