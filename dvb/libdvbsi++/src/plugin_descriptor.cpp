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

#include <dvbsi++/plugin_descriptor.h>
#include <dvbsi++/byte_stream.h>

PluginDescriptor::PluginDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	applicationType = r16(&buffer[2]);
	for (size_t i = 0; i < descriptorLength - 2; i += 5)
		applicationProfiles.push_back(new ApplicationProfile(&buffer[i + 4]));
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

const ApplicationProfileVector *PluginDescriptor::getApplicationProfiles(void) const
{
	return &applicationProfiles;
}
