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

#ifndef __external_application_authorisation_descriptor_h__
#define __external_application_authorisation_descriptor_h__

#include "application_identifier.h"
#include "descriptor.h"

class ExternalApplicationAuthorisation
{
	protected:
		ApplicationIdentifier *applicationIdentifier;
		unsigned applicationPriority			: 8;

	public:
		ExternalApplicationAuthorisation(const uint8_t * const buffer);
		~ExternalApplicationAuthorisation(void);
		const ApplicationIdentifier *getApplicationIdentifier(void) const;
		uint8_t getApplicationPriority(void) const;
};

typedef std::vector<ExternalApplicationAuthorisation *> ExternalApplicationAuthorisationVector;
typedef ExternalApplicationAuthorisationVector::iterator ExternalApplicationAuthorisationIterator;
typedef ExternalApplicationAuthorisationVector::const_iterator ExternalApplicationAuthorisationConstIterator;

class ExternalApplicationAuthorisationDescriptor : public Descriptor
{
	protected:
		ExternalApplicationAuthorisationVector externalApplicationAuthorisations;

	public:
		ExternalApplicationAuthorisationDescriptor(const uint8_t * const buffer);
		~ExternalApplicationAuthorisationDescriptor(void);
		
		const ExternalApplicationAuthorisationVector *getExternalApplicationAuthorisations(void) const;
};

#endif /* __external_application_authorisation_descriptor_h__ */
