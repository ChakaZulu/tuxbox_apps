/*
 * $Id: extension_descriptor.cpp,v 1.2 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/extension_descriptor.h"

ExtensionDescriptor::ExtensionDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(1);

	extensionTag = buffer[2];
	selectorBytes.resize(descriptorLength - 1);
	memcpy(&selectorBytes[0], &buffer[3], descriptorLength - 1);
}

ExtensionDescriptor::~ExtensionDescriptor()
{
}

uint8_t ExtensionDescriptor::getExtensionTag() const
{
	return extensionTag;
}

const SelectorByteVector *ExtensionDescriptor::getSelectorBytes() const
{
	return &selectorBytes;
}
