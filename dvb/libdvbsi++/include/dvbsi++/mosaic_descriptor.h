/*
 * $Id: mosaic_descriptor.h,v 1.1 2004/02/13 15:27:38 obi Exp $
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

#ifndef __mosaic_descriptor_h__
#define __mosaic_descriptor_h__

#include "descriptor.h"

class ElementaryCellField
{
	protected:
		unsigned elementaryCellId			: 6;

	public:
		ElementaryCellField(const uint8_t * const buffer);

		uint8_t getElementaryCellId(void) const;
};

typedef std::vector<ElementaryCellField *> ElementaryCellFieldVector;
typedef ElementaryCellFieldVector::iterator ElementaryCellFieldIterator;
typedef ElementaryCellFieldVector::const_iterator ElementaryCellFieldConstIterator;

class MosaicCell
{
	protected:
		unsigned logicalCellId				: 6;
		unsigned logicalCellPresentationInfo		: 3;
		unsigned elementaryCellFieldLength		: 8;
		ElementaryCellFieldVector elementaryCellFields;
		unsigned cellLinkageInfo			: 8;
		unsigned bouquetId				: 16;
		unsigned originalNetworkId			: 16;
		unsigned transportStreamId			: 16;
		unsigned serviceId				: 16;
		unsigned eventId				: 16;

	public:
		MosaicCell(const uint8_t * const buffer);
		~MosaicCell(void);

		uint8_t getLogicalCellId(void) const;
		uint8_t getLogicalCellPresentationInfo(void) const;
		const ElementaryCellFieldVector *getElementaryCellFields(void) const;
		uint8_t getCellLinkageInfo(void) const;
		uint16_t getBouquetId(void) const;
		uint16_t getOriginalNetworkId(void) const;
		uint16_t getTransportStreamId(void) const;
		uint16_t getServiceId(void) const;
		uint16_t getEventId(void) const;
};

typedef std::vector<MosaicCell *> MosaicCellVector;
typedef MosaicCellVector::iterator MosaicCellIterator;
typedef MosaicCellVector::const_iterator MosaicCellConstIterator;

class MosaicDescriptor : public Descriptor
{
	protected:
		unsigned mosaicEntryPoint			: 1;
		unsigned numberOfHorizontalElementaryCells	: 3;
		unsigned numberOfVerticalElementaryCells	: 3;
		MosaicCellVector mosaicCells;

	public:
		MosaicDescriptor(const uint8_t * const buffer);
		~MosaicDescriptor(void);

		uint8_t getMosaicEntryPoint(void) const;
		uint8_t getNumberOfHorizontalElementaryCells(void) const;
		uint8_t getNumberOfVerticalElementaryCells(void) const;
		const MosaicCellVector *getMosaicCells(void) const;
};

#endif /* __mosaic_descriptor_h__ */
