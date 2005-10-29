/*
 * $Id: data_broadcast_id_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __data_broadcast_id_descriptor_h__
#define __data_broadcast_id_descriptor_h__

#include "descriptor.h"

typedef std::list<uint8_t> IdSelectorByteList;
typedef IdSelectorByteList::iterator IdSelectorByteIterator;
typedef IdSelectorByteList::const_iterator IdSelectorByteConstIterator;

class DataBroadcastIdDescriptor : public Descriptor
{
	protected:
		unsigned dataBroadcastId			: 16;
		IdSelectorByteList idSelectorBytes;

	public:
		DataBroadcastIdDescriptor(const uint8_t * const buffer);

		uint16_t getDataBroadcastId(void) const;
		const IdSelectorByteList *getIdSelectorBytes(void) const;
};

#endif /* __data_broadcast_id_descriptor_h__ */
