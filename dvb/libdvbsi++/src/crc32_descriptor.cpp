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
