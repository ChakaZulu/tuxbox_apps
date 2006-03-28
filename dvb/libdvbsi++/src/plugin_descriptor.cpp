/*
 * $Id: plugin_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/plugin_descriptor.h>
#include <dvbsi++/byte_stream.h>

PluginDescriptor::PluginDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(2);

	applicationType = r16(&buffer[2]);

	for (size_t i = 0; i < descriptorLength - 2; i += 5) {
		ASSERT_MIN_DLEN(i + 7);
		applicationProfiles.push_back(new ApplicationProfile(&buffer[i + 4]));
	}
}

PluginDescriptor::~PluginDescriptor(void)
{
	for (ApplicationProfileIterator i = applicationProfiles.begin(); i != applicationProfiles.end(); ++i)
		delete *i;
}

uint16_t PluginDescriptor::getApplicationType(void) const
{
	return applicationType;
}

const ApplicationProfileList *PluginDescriptor::getApplicationProfiles(void) const
{
	return &applicationProfiles;
}
