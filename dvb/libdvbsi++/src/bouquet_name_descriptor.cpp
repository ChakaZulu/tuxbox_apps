/*
 * $Id: bouquet_name_descriptor.cpp,v 1.3 2005/10/29 00:10:16 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/bouquet_name_descriptor.h>

BouquetNameDescriptor::BouquetNameDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	bouquetName.assign((char *)&buffer[2], descriptorLength);
}

BouquetNameDescriptor::~BouquetNameDescriptor(void)
{
}

const std::string &BouquetNameDescriptor::getBouquetName(void) const
{
	return bouquetName;
}

