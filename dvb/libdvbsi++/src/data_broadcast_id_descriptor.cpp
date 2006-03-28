/*
 * $Id: data_broadcast_id_descriptor.cpp,v 1.5 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/data_broadcast_id_descriptor.h>

DataBroadcastIdDescriptor::DataBroadcastIdDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(2);

	dataBroadcastId = UINT16(&buffer[2]);

	for (size_t i = 0; i < descriptorLength - 2; ++i)
		idSelectorBytes.push_back(buffer[i + 4]);
}

uint16_t DataBroadcastIdDescriptor::getDataBroadcastId(void) const
{
	return dataBroadcastId;
}

const IdSelectorByteList *DataBroadcastIdDescriptor::getIdSelectorBytes(void) const
{
	return &idSelectorBytes;
}

