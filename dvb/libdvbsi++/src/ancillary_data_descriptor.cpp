/*
 * $Id: ancillary_data_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/ancillary_data_descriptor.h>

AncillaryDataDescriptor::AncillaryDataDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(1);

	ancillaryDataIdentifier = buffer[2];
}

uint8_t AncillaryDataDescriptor::getAncillaryDataIdentifier(void) const
{
	return ancillaryDataIdentifier;
}

