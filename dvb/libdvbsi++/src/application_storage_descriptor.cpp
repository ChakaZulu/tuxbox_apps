/*
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
 
#include <dvbsi++/application_storage_descriptor.h>
#include <dvbsi++/byte_stream.h>

ApplicationStorageDescriptor::ApplicationStorageDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
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
