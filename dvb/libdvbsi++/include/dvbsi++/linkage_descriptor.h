/*
 * $Id: linkage_descriptor.h,v 1.1 2004/02/13 15:27:37 obi Exp $
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

#ifndef __linkage_descriptor_h__
#define __linkage_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> PrivateDataByteVector;
typedef PrivateDataByteVector::iterator PrivateDataByteIterator;
typedef PrivateDataByteVector::const_iterator PrivateDataByteConstIterator;

class LinkageDescriptor : public Descriptor
{
	protected:
		unsigned transportStreamId			: 16;
		unsigned originalNetworkId			: 16;
		unsigned serviceId				: 16;
		unsigned linkageType				: 8;
		PrivateDataByteVector privateDataBytes;
		unsigned handOverType				: 4;
		unsigned originType				: 1;
		unsigned networkId				: 16;
		unsigned initialServiceId			: 16;

	public:
		LinkageDescriptor(const uint8_t * const buffer);

		uint16_t getTransportStreamId(void) const;
		uint16_t getOriginalNetworkId(void) const;
		uint16_t getServiceId(void) const;
		uint8_t getLinkageType(void) const;
		const PrivateDataByteVector *getPrivateDataBytes(void) const;
		uint8_t getHandOverType(void) const;
		uint8_t getOriginType(void) const;
		uint16_t getNetworkId(void) const;
		uint16_t getInitialServiceId(void) const;
};

#endif /* __linkage_descriptor_h__ */
