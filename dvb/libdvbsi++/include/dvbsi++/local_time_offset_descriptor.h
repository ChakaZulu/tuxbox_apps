/*
 * $Id: local_time_offset_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __local_time_offset_descriptor_h__
#define __local_time_offset_descriptor_h__

#include "descriptor.h"

class LocalTimeOffset
{
	protected:
		std::string countryCode;
		unsigned countryRegionId			: 6;
		unsigned localTimeOffsetPolarity		: 1;
		unsigned localTimeOffset			: 16;
		unsigned timeOfChangeMjd			: 16;
		unsigned timeOfChangeBcd			: 24;
		unsigned nextTimeOffset				: 16;

	public:
		LocalTimeOffset(const uint8_t * const buffer);

		const std::string &getCountryCode(void) const;
		uint8_t getCountryRegionId(void) const;
		uint8_t getLocalTimeOffsetPolarity(void) const;
		uint16_t getLocalTimeOffset(void) const;
		uint16_t getTimeOfChangeMjd(void) const;
		uint32_t getTimeOfChangeBcd(void) const;
		uint16_t getNextTimeOffset(void) const;
};

typedef std::list<LocalTimeOffset *> LocalTimeOffsetList;
typedef LocalTimeOffsetList::iterator LocalTimeOffsetIterator;
typedef LocalTimeOffsetList::const_iterator LocalTimeOffsetConstIterator;

class LocalTimeOffsetDescriptor : public Descriptor
{
	protected:
		LocalTimeOffsetList localTimeOffsets;

	public:
		LocalTimeOffsetDescriptor(const uint8_t * const buffer);
		~LocalTimeOffsetDescriptor(void);

		const LocalTimeOffsetList *getLocalTimeOffsets(void) const;
};

#endif /* __local_time_offset_descriptor_h__ */
