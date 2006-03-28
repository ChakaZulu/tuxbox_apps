/*
 * $Id: pdc_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
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
#include <dvbsi++/pdc_descriptor.h>

PdcDescriptor::PdcDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(3);

	programmeIdentificationLabel = ((buffer[2] & 0x0f) << 16) | UINT16(&buffer[3]);
}

uint32_t PdcDescriptor::getProgrammeIdentificationLabel(void) const
{
	return programmeIdentificationLabel;
}

