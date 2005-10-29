/*
 * $Id: cell_frequency_link_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
