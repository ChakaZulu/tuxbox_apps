/*
 * $Id: stream_identifier_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/stream_identifier_descriptor.h>

StreamIdentifierDescriptor::StreamIdentifierDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(1);

	componentTag = buffer[2];
}

uint8_t StreamIdentifierDescriptor::getComponentTag(void) const
{
	return componentTag;
}

