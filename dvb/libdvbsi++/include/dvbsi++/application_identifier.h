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

#ifndef __application_identifier_h__
#define __application_identifier_h__

#include "compat.h"

class ApplicationIdentifier
{
	protected:
		unsigned organisationId				: 32;
		unsigned applicationId				: 16;

	public:
		ApplicationIdentifier(const uint8_t * const buffer);

		uint32_t getOrganisationId(void) const;
		uint16_t getApplicationId(void) const;
};

typedef std::list<ApplicationIdentifier *> ApplicationIdentifierList;
typedef ApplicationIdentifierList::iterator ApplicationIdentifierIterator;
typedef ApplicationIdentifierList::const_iterator ApplicationIdentifierConstIterator;

#endif /* __application_identifier_h__ */
