/*
 * $Id: dvb_html_application_location_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/dvb_html_application_location_descriptor.h>

DvbHtmlApplicationLocationDescriptor::DvbHtmlApplicationLocationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(1);

	physicalRootLength = buffer[2];

	ASSERT_MIN_DLEN(physicalRootLength + 1);

	physicalRoot.assign((char *)&buffer[3], physicalRootLength);
	initialPath.assign((char *)&buffer[physicalRootLength + 3], descriptorLength - physicalRootLength - 1);
}

const std::string &DvbHtmlApplicationLocationDescriptor::getPhysicalRoot(void) const
{
	return physicalRoot;
}

const std::string &DvbHtmlApplicationLocationDescriptor::getInitialPath(void) const
{
	return initialPath;
}
