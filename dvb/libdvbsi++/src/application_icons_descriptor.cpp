/*
 * $Id: application_icons_descriptor.cpp,v 1.2 2005/10/29 00:10:16 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/application_icons_descriptor.h>
#include <dvbsi++/byte_stream.h>
 
ApplicationIconsDescriptor::ApplicationIconsDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	iconLocatorLength = buffer[2];
	iconLocator.assign((char *)&buffer[3], iconLocatorLength);
	iconFlags = r16(&buffer[iconLocatorLength + 3]);
}

const std::string &ApplicationIconsDescriptor::getIconLocator(void) const
{
	return iconLocator;
}

uint16_t ApplicationIconsDescriptor::getIconFlags(void) const
{
	return iconFlags;
}
