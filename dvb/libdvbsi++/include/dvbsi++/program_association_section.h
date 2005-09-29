/*
 * $Id: program_association_section.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __program_association_section_h__
#define __program_association_section_h__

#include "long_crc_section.h"

class NetworkAssociation
{
	protected:
		unsigned programNumber				: 16;
		unsigned networkPid				: 13;

	public:
		NetworkAssociation(const uint8_t * buffer);

		uint16_t getNetworkPid(void) const;
};

typedef std::list<NetworkAssociation *> NetworkAssociationList;
typedef NetworkAssociationList::iterator NetworkAssociationIterator;
typedef NetworkAssociationList::const_iterator NetworkAssociationConstIterator;

class ProgramAssociation
{
	protected:
		unsigned programNumber				: 16;
		unsigned programMapPid				: 13;

	public:
		ProgramAssociation(const uint8_t * buffer);

		uint16_t getProgramNumber(void) const;
		uint16_t getProgramMapPid(void) const;
};

typedef std::list<ProgramAssociation *> ProgramAssociationList;
typedef ProgramAssociationList::iterator ProgramAssociationIterator;
typedef ProgramAssociationList::const_iterator ProgramAssociationConstIterator;

class ProgramAssociationSection : public LongCrcSection
{
	protected:
		NetworkAssociationList networks;
		ProgramAssociationList programs;

	public:
		ProgramAssociationSection(const uint8_t * const buffer);
		~ProgramAssociationSection(void);

		static const enum PacketId PID = PID_PAT;
		static const enum TableId TID = TID_PAT;
		static const uint32_t TIMEOUT = 1200;

		const NetworkAssociationList *getNetworks(void) const;
		const ProgramAssociationList *getPrograms(void) const;
};

typedef std::list<ProgramAssociationSection *> ProgramAssociationSectionList;
typedef ProgramAssociationSectionList::iterator ProgramAssociationSectionIterator;
typedef ProgramAssociationSectionList::const_iterator ProgramAssociationSectionConstIterator;

#endif /* __program_association_section_h__ */
