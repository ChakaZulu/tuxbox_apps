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

#include <dvbsi++/dvb_html_application_location_descriptor.h>

DvbHtmlApplicationLocationDescriptor::DvbHtmlApplicationLocationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	physicalRootLength = buffer[2];
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
