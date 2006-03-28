/*
 * $Id: cell_frequency_link_descriptor.cpp,v 1.5 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/cell_frequency_link_descriptor.h>

SubcellInfo::SubcellInfo(const uint8_t * const buffer)
{
	cellIdExtenstion = buffer[0];
	transposerFrequency = UINT32(&buffer[1]);
}

uint8_t SubcellInfo::getCellIdExtension(void) const
{
	return cellIdExtenstion;
}

uint32_t SubcellInfo::getTransposerFrequency(void) const
{
	return transposerFrequency;
}

CellFrequencyLink::CellFrequencyLink(const uint8_t * const buffer)
{
	cellId = UINT16(&buffer[0]);
	frequency = UINT32(&buffer[2]);
	subcellInfoLoopLength = buffer[6];

	for (size_t i = 0; i < subcellInfoLoopLength; i += 5)
		subcells.push_back(new SubcellInfo(&buffer[i + 7]));
}

CellFrequencyLink::~CellFrequencyLink(void)
{
	for (SubcellInfoIterator i = subcells.begin(); i != subcells.end(); ++i)
		delete *i;
}

uint16_t CellFrequencyLink::getCellId(void) const
{
	return cellId;
}

uint32_t CellFrequencyLink::getFrequency(void) const
{
	return frequency;
}

const SubcellInfoList *CellFrequencyLink::getSubcells(void) const
{
	return &subcells;
}

CellFrequencyLinkDescriptor::CellFrequencyLinkDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += buffer[i + 10] + 6) {
		ASSERT_MIN_DLEN(i + buffer[i + 10] + 6);
		cellFrequencyLinks.push_back(new CellFrequencyLink(&buffer[i + 2]));
	}
}

CellFrequencyLinkDescriptor::~CellFrequencyLinkDescriptor(void)
{
	for (CellFrequencyLinkIterator i = cellFrequencyLinks.begin(); i != cellFrequencyLinks.end(); ++i)
		delete *i;
}

const CellFrequencyLinkList *CellFrequencyLinkDescriptor::getCellFrequencyLinks(void) const
{
	return &cellFrequencyLinks;
}

