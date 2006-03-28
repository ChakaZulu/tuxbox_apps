/*
 * $Id: default_authority_descriptor.cpp,v 1.2 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/default_authority_descriptor.h"

DefaultAuthorityDescriptor::DefaultAuthorityDescriptor(const uint8_t * const buffer) : Descriptor(buffer),
	bytes(descriptorLength)
{

	memcpy(&bytes[0], &buffer[2], descriptorLength);
}

DefaultAuthorityDescriptor::~DefaultAuthorityDescriptor()
{
}

const DefaultAuthorityByteVector *DefaultAuthorityDescriptor::getAuthorityBytes(void) const
{
	return &bytes;
}
