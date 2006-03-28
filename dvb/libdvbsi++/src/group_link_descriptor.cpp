/*
 * $Id: group_link_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/group_link_descriptor.h>
#include <dvbsi++/byte_stream.h>

GroupLinkDescriptor::GroupLinkDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(5);

	position = buffer[2];
	groupId = r32(&buffer[3]);
}

uint8_t GroupLinkDescriptor::getPosition(void) const
{
	return position;
}

uint32_t GroupLinkDescriptor::getGroupId(void) const
{
	return groupId;
}
