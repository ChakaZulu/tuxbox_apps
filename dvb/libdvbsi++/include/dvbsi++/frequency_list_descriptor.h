/*
 * $Id: frequency_list_descriptor.h,v 1.1 2004/02/13 15:27:37 obi Exp $
 *
 * Copyright (C) 2002-2004 Andreas Oberritter <obi@saftware.de>
 *
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

#ifndef __frequency_list_descriptor_h__
#define __frequency_list_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint32_t> CentreFrequencyVector;
typedef CentreFrequencyVector::iterator CentreFrequencyIterator;
typedef CentreFrequencyVector::const_iterator CentreFrequencyConstIterator;

class FrequencyListDescriptor : public Descriptor
{
	protected:
		unsigned codingType				: 2;
		CentreFrequencyVector centreFrequencies;

	public:
		FrequencyListDescriptor(const uint8_t * const buffer);

		uint8_t getCodingType(void) const;
		const CentreFrequencyVector *getCentreFrequencies(void) const;
};

#endif /* __frequency_list_descriptor_h__ */
