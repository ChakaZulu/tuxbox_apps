/*
 * $Id: program_map_section.h,v 1.5 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
