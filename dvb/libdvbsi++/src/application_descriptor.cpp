/*
 * $Id: application_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/application_descriptor.h>
#include <dvbsi++/byte_stream.h>

ApplicationDescriptor::ApplicationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	size_t headerLength = 3;
	ASSERT_MIN_DLEN(headerLength);

	applicationProfilesLength = buffer[2];

	headerLength += applicationProfilesLength;
	ASSERT_MIN_DLEN(headerLength);

	for (size_t i = 0; i < applicationProfilesLength; i += 5)
		applicationProfiles.push_back(new ApplicationProfile(&buffer[i + 3]));

	serviceBoundFlag = (buffer[applicationProfilesLength + 3] >> 7) & 0x01;
	visibility = (buffer[applicationProfilesLength + 3] >> 5) & 0x02;
	applicationPriority = buffer[applicationProfilesLength + 4];

	for (size_t i = 0; i < descriptorLength - applicationProfilesLength - 3; i += 1)
		transportProtocolLabels.push_back(buffer[i + applicationProfilesLength + 5]);
}

ApplicationDescriptor::~ApplicationDescriptor(void)
{
	for (ApplicationProfileIterator i = applicationProfiles.begin(); i != applicationProfiles.end(); ++i)
		delete *i;
}

const ApplicationProfileList *ApplicationDescriptor::getApplicationProfiles(void) const
{
	return &applicationProfiles;
}

uint8_t ApplicationDescriptor::getServiceBoundFlag(void) const
{
	return serviceBoundFlag;
}

uint8_t ApplicationDescriptor::getVisibility(void) const
{
	return visibility;
}

uint8_t ApplicationDescriptor::getApplicationPriority(void) const
{
	return applicationPriority;
}

const TransportProtocolLabelList *ApplicationDescriptor::getTransportProtocolLabels(void) const
{
	return &transportProtocolLabels;
}
