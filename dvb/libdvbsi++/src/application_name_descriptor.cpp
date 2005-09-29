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

#include <dvbsi++/application_name_descriptor.h>

ApplicationName::ApplicationName(const uint8_t * const buffer)
{
	iso639LanguageCode.assign((char *)&buffer[0], 3);
	applicationNameLength = buffer[3];
	applicationName.assign((char *)&buffer[4], applicationNameLength);
}

const std::string &ApplicationName::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

const std::string &ApplicationName::getApplicationName(void) const
{
	return applicationName;
}

ApplicationNameDescriptor::ApplicationNameDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += buffer[i + 3] + 2)
		applicationNames.push_back(new ApplicationName(&buffer[i + 2]));
}

ApplicationNameDescriptor::~ApplicationNameDescriptor(void)
{
	for (ApplicationNameIterator i = applicationNames.begin(); i != applicationNames.end(); ++i)
		delete *i;
}

const ApplicationNameList *ApplicationNameDescriptor::getApplicationNames(void) const
{
	return &applicationNames;
}
