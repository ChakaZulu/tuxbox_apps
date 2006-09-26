/*
 * $Id: bouquet_association_section.cpp,v 1.7 2006/09/26 20:34:33 mws Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/bouquet_association_section.h>
#include <dvbsi++/byte_stream.h>

BouquetAssociation::BouquetAssociation(const uint8_t * const buffer)
{
	transportStreamId = UINT16(&buffer[0]);
	originalNetworkId = UINT16(&buffer[2]);
	transportStreamLoopLength = DVB_LENGTH(&buffer[4]);

	for (size_t i = 6; i < transportStreamLoopLength + 6; i += buffer[i + 1] + 2)
		descriptor(&buffer[i], SCOPE_SI);
}

uint16_t BouquetAssociation::getTransportStreamId(void) const
{
	return transportStreamId;
}

uint16_t BouquetAssociation::getOriginalNetworkId(void) const
{
	return originalNetworkId;
}

BouquetAssociationSection::BouquetAssociationSection(const uint8_t * const buffer) : LongCrcSection(buffer)
{
	bouquetDescriptorsLength = sectionLength > 9 ? DVB_LENGTH(&buffer[8]) : 0;

	uint16_t pos = 10;
	uint16_t bytesLeft = sectionLength > 11 ? sectionLength - 11 : 0;
	uint16_t loopLength = 0;
	uint16_t bytesLeft2 = bouquetDescriptorsLength;

	while (bytesLeft >= bytesLeft2 && bytesLeft2 > 1 && bytesLeft2 >= (loopLength = 2 + buffer[pos+1])) {
		descriptor(&buffer[pos], SCOPE_SI);
		pos += loopLength;
		bytesLeft -= loopLength;
		bytesLeft2 -= loopLength;
	}

	if (!bytesLeft2 && bytesLeft > 1) {
		bytesLeft2 = transportStreamLoopLength = DVB_LENGTH(&buffer[pos]);
		bytesLeft -= 2;
		pos += 2;
		while (bytesLeft >= bytesLeft2 && bytesLeft2 > 4 && bytesLeft2 >= (loopLength = 6 + DVB_LENGTH(&buffer[pos+4]))) {
			bouquet.push_back(new BouquetAssociation(&buffer[pos]));
			bytesLeft -= loopLength;
			bytesLeft2 -= loopLength;
			pos += loopLength;
		}
	}
}

BouquetAssociationSection::~BouquetAssociationSection(void)
{
	for (BouquetAssociationIterator i = bouquet.begin(); i != bouquet.end(); ++i)
		delete *i;
}

const BouquetAssociationList* BouquetAssociationSection::getBouquets(void) const
{
	return &bouquet;
}
