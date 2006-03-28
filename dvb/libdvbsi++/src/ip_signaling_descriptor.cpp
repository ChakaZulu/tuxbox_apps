/*
 * $Id: ip_signaling_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/ip_signaling_descriptor.h>
#include <dvbsi++/byte_stream.h>

IpSignalingDescriptor::IpSignalingDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(3);

	platformId = (buffer[2] << 16) | r16(&buffer[3]);
}

uint32_t IpSignalingDescriptor::getPlatformId(void) const
{
	return platformId;
}
