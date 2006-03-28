/*
 * $Id: application_signalling_descriptor.cpp,v 1.5 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/application_signalling_descriptor.h>
#include <dvbsi++/byte_stream.h>

ApplicationSignalling::ApplicationSignalling(const uint8_t * const buffer)
{
	applicationType = UINT16(&buffer[0]);
	aitVersionNumber = buffer[2] & 0x1f;
}

uint16_t ApplicationSignalling::getApplicationType(void) const
{
	return applicationType;
}

uint8_t ApplicationSignalling::getAitVersionNumber(void) const
{
	return aitVersionNumber;
}

ApplicationSignallingDescriptor::ApplicationSignallingDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 3) {
		ASSERT_MIN_DLEN(i + 3);
		applicationSignallings.push_back(new ApplicationSignalling(&buffer[i + 2]));
	}
}

ApplicationSignallingDescriptor::~ApplicationSignallingDescriptor(void)
{
	for (ApplicationSignallingIterator i = applicationSignallings.begin(); i != applicationSignallings.end(); ++i)
		delete *i;
}

const ApplicationSignallingList *ApplicationSignallingDescriptor::getApplicationSignallings(void) const
{
	return &applicationSignallings;
}

