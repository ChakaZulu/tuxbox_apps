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
 
#include <dvbsi++/external_application_authorisation_descriptor.h>

ExternalApplicationAuthorisation::ExternalApplicationAuthorisation(const uint8_t
* const buffer)
{
	applicationIdentifier = new ApplicationIdentifier(&buffer[0]);
	applicationPriority = buffer[6];
}

ExternalApplicationAuthorisation::~ExternalApplicationAuthorisation(void)
{
	delete applicationIdentifier;
}

const ApplicationIdentifier
*ExternalApplicationAuthorisation::getApplicationIdentifier(void) const
{
	return applicationIdentifier;
}

uint8_t ExternalApplicationAuthorisation::getApplicationPriority(void) const
{
	return applicationPriority;
}

ExternalApplicationAuthorisationDescriptor::ExternalApplicationAuthorisationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 7)
		externalApplicationAuthorisations.push_back(new ExternalApplicationAuthorisation(&buffer[i + 2]));
}

ExternalApplicationAuthorisationDescriptor::~ExternalApplicationAuthorisationDescriptor(void)
{
	for (ExternalApplicationAuthorisationIterator i = externalApplicationAuthorisations.begin(); i != externalApplicationAuthorisations.end(); ++i)
		delete *i;
}

const ExternalApplicationAuthorisationVector *ExternalApplicationAuthorisationDescriptor::getExternalApplicationAuthorisations(void) const
{
	return &externalApplicationAuthorisations;
}
