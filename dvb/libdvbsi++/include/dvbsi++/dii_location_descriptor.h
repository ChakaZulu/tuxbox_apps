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
 
#ifndef __dii_location_descriptor_h__
#define __dii_location_descriptor_h__

#include "descriptor.h"

class DiiLocation
{
	protected:
		unsigned diiIdentification			: 15;
		unsigned associationTag				: 16;

	public:
		DiiLocation(const uint8_t * const buffer);

		uint16_t getDiiIdentification(void) const;
		uint16_t getAssociationTag(void) const;
};

typedef std::vector<DiiLocation *> DiiLocationVector;
typedef DiiLocationVector::iterator DiiLocationIterator;
typedef DiiLocationVector::const_iterator DiiLocationConstIterator;

class DiiLocationDescriptor : public Descriptor
{
	protected:
		unsigned transportProtocolLabel			: 8;
		DiiLocationVector diiLocations;

	public:
		DiiLocationDescriptor(const uint8_t * const buffer);
		~DiiLocationDescriptor(void);
		
		uint8_t getTransportProtocolLabel(void) const;
		const DiiLocationVector *getDiiLocations(void) const;
};

#endif /* __dii_location_descriptor_h__ */
