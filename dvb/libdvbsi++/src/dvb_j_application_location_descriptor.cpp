/*
 * $Id: dvb_j_application_location_descriptor.cpp,v 1.2 2005/10/29 00:10:16 obi Exp $
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
	baseDirectoryLength = buffer[2];
	baseDirectory.assign((char *)&buffer[3], baseDirectoryLength);
	classpathExtensionLength = buffer[baseDirectoryLength + 3];
	classpathExtension.assign((char *)&buffer[baseDirectoryLength + 4], classpathExtensionLength);
	initialClass.assign((char *)&buffer[baseDirectoryLength + classpathExtensionLength + 4], descriptorLength - baseDirectoryLength - classpathExtensionLength - 2);
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
