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
