/*
 * $Id: compressed_module_descriptor.cpp,v 1.2 2005/10/29 00:10:16 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/compressed_module_descriptor.h>
#include <dvbsi++/byte_stream.h>

CompressedModuleDescriptor::CompressedModuleDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	compressionMethod = buffer[2];
	originalSize = r32(&buffer[3]);
}

uint8_t CompressedModuleDescriptor::getCompressionMethod(void) const
{
	return compressionMethod;
}

uint32_t CompressedModuleDescriptor::getOriginalSize(void) const
{
	return originalSize;
}
