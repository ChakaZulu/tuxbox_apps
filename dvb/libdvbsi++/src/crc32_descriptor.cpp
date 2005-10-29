/*
 * $Id: crc32_descriptor.cpp,v 1.2 2005/10/29 00:10:16 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/crc32_descriptor.h>
#include <dvbsi++/byte_stream.h>

Crc32Descriptor::Crc32Descriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	crc32 = r32(&buffer[2]);
}

uint32_t Crc32Descriptor::getCrc32(void) const
{
	return crc32;
}
