/*
 * $Id: descriptor.cpp,v 1.1 2004/02/13 15:27:46 obi Exp $
 *
 * Copyright (C) 2002-2004 Andreas Oberritter <obi@saftware.de>
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

#include <dvbsi++/descriptor.h>

Descriptor::Descriptor(const uint8_t * const buffer)
{
	descriptorTag = buffer[0];
	descriptorLength = buffer[1];
}

uint8_t Descriptor::getTag(void) const
{
	return descriptorTag;
}

uint8_t Descriptor::getLength(void) const
{
	return descriptorLength;
}

size_t Descriptor::writeToBuffer(uint8_t * const buffer) const
{
	size_t total = 0;

	buffer[total++] = descriptorTag;
	buffer[total++] = descriptorLength;

	return total;
}

