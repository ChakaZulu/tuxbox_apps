/*
 * $Id: time_shifted_service_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
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
#include <dvbsi++/time_shifted_service_descriptor.h>

TimeShiftedServiceDescriptor::TimeShiftedServiceDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(2);

	referenceServiceId = UINT16(&buffer[2]);
}

uint16_t TimeShiftedServiceDescriptor::getReferenceServiceId(void) const
{
	return referenceServiceId;
}

