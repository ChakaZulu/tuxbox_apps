/*
 * $Id: application_storage_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/application_storage_descriptor.h>
#include <dvbsi++/byte_stream.h>

ApplicationStorageDescriptor::ApplicationStorageDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(7);

	storageProperty = buffer[2];
	notLaunchableFromBroadcast = (buffer[3] >> 7) & 0x01;
	version = r32(&buffer[4]);
	priority = buffer[8];
}

uint8_t ApplicationStorageDescriptor::getStorageProperty(void) const
{
	return storageProperty;
}

uint8_t ApplicationStorageDescriptor::getNotLaunchableFromBroadcast(void) const
{
	return notLaunchableFromBroadcast;
}

uint32_t ApplicationStorageDescriptor::getVersion(void) const
{
	return version;
}

uint8_t ApplicationStorageDescriptor::getPriority(void) const
{
	return priority;
}
