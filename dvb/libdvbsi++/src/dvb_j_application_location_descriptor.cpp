/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
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
