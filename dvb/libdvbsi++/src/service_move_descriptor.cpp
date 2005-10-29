/*
 * $Id: service_move_descriptor.cpp,v 1.3 2005/10/29 00:10:17 obi Exp $
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
#include <dvbsi++/service_move_descriptor.h>

ServiceMoveDescriptor::ServiceMoveDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	newOriginalNetworkId = UINT16(&buffer[2]);
	newTransportStreamId = UINT16(&buffer[4]);
	newServiceId = UINT16(&buffer[6]);
}

uint16_t ServiceMoveDescriptor::getNewOriginalNetworkId(void) const
{
	return newOriginalNetworkId;
}

uint16_t ServiceMoveDescriptor::getNewTransportStreamId(void) const
{
	return newTransportStreamId;
}

uint16_t ServiceMoveDescriptor::getNewServiceId(void) const
{
	return newServiceId;
}

