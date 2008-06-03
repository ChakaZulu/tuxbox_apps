/*
 * $Id: registration_descriptor.cpp,v 1.1 2008/06/03 15:02:24 obi Exp $
 *
 * Copyright (C) 2008 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/registration_descriptor.h>

RegistrationDescriptor::RegistrationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(4);

	formatIdentifier = r32(&buffer[2]);

	additionalIdentificationInfo.resize(descriptorLength - 4);
	memcpy(&additionalIdentificationInfo[0], &buffer[6], descriptorLength - 4);
}

uint32_t RegistrationDescriptor::getFormatIdentifier(void) const
{
	return formatIdentifier;
}

const AdditionalIdentificationInfoVector *RegistrationDescriptor::getAdditionalIdentificationInfo(void) const
{
	return &additionalIdentificationInfo;
}

