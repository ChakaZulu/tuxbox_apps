/*
 * $Id: service_move_descriptor.cpp,v 1.2 2004/02/13 17:51:08 obi Exp $
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

