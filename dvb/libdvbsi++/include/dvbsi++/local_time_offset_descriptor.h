/*
 * $Id: local_time_offset_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
