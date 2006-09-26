/*
 * $Id: adaptation_field_data_descriptor.cpp,v 1.2 2006/09/26 07:29:23 mws Exp $
 *
 * Copyright (C) 2006 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include <dvbsi++/adaptation_field_data_descriptor.h>

AdaptationFieldDataDescriptor::AdaptationFieldDataDescriptor(const uint8_t* const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(1);

	adaptationFieldDataIdentifier = buffer[2];
}

AdaptationFieldDataDescriptor::~AdaptationFieldDataDescriptor()
{
}

uint8_t AdaptationFieldDataDescriptor::getAdaptationFieldDataIdentifier(void) const
{
	return adaptationFieldDataIdentifier;
}
