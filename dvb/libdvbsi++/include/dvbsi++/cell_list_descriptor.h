/*
 * $Id: cell_list_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __cell_list_descriptor_h__
#define __cell_list_descriptor_h__

#include "descriptor.h"

class Subcell
{
	protected:
		unsigned cellIdExtension			: 8;
		unsigned subcellLatitude			: 16;
		unsigned subcellLongitude			: 16;
		unsigned subcellExtendOfLatitude		: 12;
		unsigned subcellExtendOfLongitude		: 12;

	public:
		Subcell(const uint8_t * const buffer);

		uint8_t getCellIdExtension(void) const;
		uint16_t getSubcellLatitude(void) const;
		uint16_t getSubcellLongtitude(void) const;
		uint16_t getSubcellExtendOfLatitude(void) const;
		uint16_t getSubcellExtendOfLongtitude(void) const;
};

typedef std::list<Subcell *> SubcellList;
typedef SubcellList::iterator SubcellIterator;
typedef SubcellList::const_iterator SubcellConstIterator;

class Cell
{
	protected:
		unsigned cellId					: 16;
		unsigned cellLatitude				: 16;
		unsigned cellLongtitude				: 16;
		unsigned cellExtendOfLatitude			: 12;
		unsigned cellExtendOfLongtitude			: 12;
		unsigned subcellInfoLoopLength			: 8;
		SubcellList subcells;

	public:
		Cell(const uint8_t * const buffer);
		~Cell(void);

		uint16_t getCellId(void) const;
		uint16_t getCellLatitude(void) const;
		uint16_t getCellLongtitude(void) const;
		uint16_t getCellExtendOfLatitude(void) const;
		uint16_t getCellExtendOfLongtitude(void) const;
		const SubcellList *getSubcells(void) const;
};

typedef std::list<Cell *> CellList;
typedef CellList::iterator CellIterator;
typedef CellList::const_iterator CellConstIterator;

class CellListDescriptor : public Descriptor
{
	protected:
		CellList cells;

	public:
		CellListDescriptor(const uint8_t * const buffer);
		~CellListDescriptor(void);

		const CellList *getCells(void) const;
};

#endif /* __cell_list_descriptor_h__ */
