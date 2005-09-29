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

#include <dvbsi++/application_descriptor.h>
#include <dvbsi++/byte_stream.h>

ApplicationDescriptor::ApplicationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	applicationProfilesLength = buffer[2];
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
