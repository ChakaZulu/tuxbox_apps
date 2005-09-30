/*
 * $Id: unknown_descriptor.cpp,v 1.2 2005/09/30 18:57:45 ghostrider Exp $
 *
 * Copyright (C) 2005 Andreas Monzner <andreas.monzner@multimedia-labs.de>
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

#include <dvbsi++/unknown_descriptor.h>

UnknownDescriptor::UnknownDescriptor(const uint8_t * const buffer) : Descriptor(buffer), dataBytes(descriptorLength)
{
	memcpy(&dataBytes[0], buffer+2, descriptorLength);
}

size_t UnknownDescriptor::writeToBuffer(uint8_t * const buffer) const
{
	Descriptor::writeToBuffer(buffer);
	memcpy(buffer+2, &dataBytes[0], descriptorLength);
	return 2 + descriptorLength;
}

