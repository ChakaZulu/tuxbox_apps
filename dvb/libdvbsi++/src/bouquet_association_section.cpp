/*
 * $Id: bouquet_association_section.cpp,v 1.4 2005/10/29 00:10:16 obi Exp $
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

BouquetAssociationSection::BouquetAssociationSection(const uint8_t * const buffer) : LongCrcSection(buffer)
{
	bouquetDescriptorsLength = DVB_LENGTH(&buffer[8]);

	for (size_t i = 10; i < bouquetDescriptorsLength + 10; i += buffer[i + 1] + 2)
		descriptor(&buffer[i], SCOPE_SI);

	transportStreamLoopLength = DVB_LENGTH(&buffer[bouquetDescriptorsLength + 10]);

	for (size_t i = bouquetDescriptorsLength + 12; i < sectionLength - 1; i += DVB_LENGTH(&buffer[i + 4]) + 6)
		bouquet.push_back(new BouquetAssociation(&buffer[i]));
}

BouquetAssociationSection::~BouquetAssociationSection(void)
{
	for (BouquetAssociationIterator i = bouquet.begin(); i != bouquet.end(); ++i)
		delete *i;
}

