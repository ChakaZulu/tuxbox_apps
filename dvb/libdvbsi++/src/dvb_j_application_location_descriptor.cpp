/*
 * $Id: dvb_j_application_location_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/dvb_j_application_location_descriptor.h>

DvbJApplicationLocationDescriptor::DvbJApplicationLocationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	size_t headerLength = 2;
	ASSERT_MIN_DLEN(headerLength);

	baseDirectoryLength = buffer[2];

	headerLength += baseDirectoryLength;
	ASSERT_MIN_DLEN(headerLength);

	baseDirectory.assign((char *)&buffer[3], baseDirectoryLength);
	classpathExtensionLength = buffer[baseDirectoryLength + 3];

	headerLength += classpathExtensionLength;
	ASSERT_MIN_DLEN(headerLength);

	classpathExtension.assign((char *)&buffer[baseDirectoryLength + 4], classpathExtensionLength);
	initialClass.assign((char *)&buffer[baseDirectoryLength + classpathExtensionLength + 4], descriptorLength - headerLength);
}

const std::string &DvbJApplicationLocationDescriptor::getBaseDirectory(void) const
{
	return baseDirectory;
}

const std::string &DvbJApplicationLocationDescriptor::getClasspathExtension(void) const
{
	return classpathExtension;
}

const std::string &DvbJApplicationLocationDescriptor::getInitialClass(void) const
{
	return initialClass;
}
