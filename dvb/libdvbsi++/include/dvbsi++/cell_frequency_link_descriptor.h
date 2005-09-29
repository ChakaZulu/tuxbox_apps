/*
 * $Id: cell_frequency_link_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __cell_frequency_link_descriptor_h__
#define __cell_frequency_link_descriptor_h__

#include "descriptor.h"

class SubcellInfo
{
	protected:
		unsigned cellIdExtenstion			: 8;
		unsigned transposerFrequency			: 32;

	public:
		SubcellInfo(const uint8_t * const buffer);

		uint8_t getCellIdExtension(void) const;
		uint32_t getTransposerFrequency(void) const;
};

typedef std::list<SubcellInfo *> SubcellInfoList;
typedef SubcellInfoList::iterator SubcellInfoIterator;
typedef SubcellInfoList::const_iterator SubcellInfoConstIterator;

class CellFrequencyLink
{
	protected:
		unsigned cellId					: 16;
		unsigned frequency				: 32;
		unsigned subcellInfoLoopLength			: 8;
		SubcellInfoList subcells;

	public:
		CellFrequencyLink(const uint8_t * const buffer);
		~CellFrequencyLink(void);

		uint16_t getCellId(void) const;
		uint32_t getFrequency(void) const;
		const SubcellInfoList *getSubcells(void) const;

};

typedef std::list<CellFrequencyLink *> CellFrequencyLinkList;
typedef CellFrequencyLinkList::iterator CellFrequencyLinkIterator;
typedef CellFrequencyLinkList::const_iterator CellFrequencyLinkConstIterator;

class CellFrequencyLinkDescriptor : public Descriptor
{
	protected:
		CellFrequencyLinkList cellFrequencyLinks;

	public:
		CellFrequencyLinkDescriptor(const uint8_t * const buffer);
		~CellFrequencyLinkDescriptor(void);

		const CellFrequencyLinkList *getCellFrequencyLinks(void) const;
};

#endif /* __cell_frequency_link_descriptor_h__ */
