/*
 * $Id: linkage_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __linkage_descriptor_h__
#define __linkage_descriptor_h__

#include "descriptor.h"

typedef std::list<uint8_t> PrivateDataByteList;
typedef PrivateDataByteList::iterator PrivateDataByteIterator;
typedef PrivateDataByteList::const_iterator PrivateDataByteConstIterator;

class LinkageDescriptor : public Descriptor
{
	protected:
		unsigned transportStreamId			: 16;
		unsigned originalNetworkId			: 16;
		unsigned serviceId				: 16;
		unsigned linkageType				: 8;
		PrivateDataByteList privateDataBytes;
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
		const PrivateDataByteList *getPrivateDataBytes(void) const;
		uint8_t getHandOverType(void) const;
		uint8_t getOriginType(void) const;
		uint16_t getNetworkId(void) const;
		uint16_t getInitialServiceId(void) const;
};

#endif /* __linkage_descriptor_h__ */
