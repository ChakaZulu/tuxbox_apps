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
 
#include <dvbsi++/delegated_application_descriptor.h>

DelegatedApplicationDescriptor::DelegatedApplicationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 6)
		applicationIdentifiers.push_back(new ApplicationIdentifier(&buffer[i + 2]));
}

DelegatedApplicationDescriptor::~DelegatedApplicationDescriptor(void)
{
	for (ApplicationIdentifierIterator i = applicationIdentifiers.begin(); i != applicationIdentifiers.end(); ++i)
		delete *i;
}

const ApplicationIdentifierVector *DelegatedApplicationDescriptor::getApplicationIdentifiers(void) const
{
	return &applicationIdentifiers;
}
