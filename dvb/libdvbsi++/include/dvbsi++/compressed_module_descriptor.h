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

#ifndef __compressed_module_descriptor_h__
#define __compressed_module_descriptor_h__

#include "descriptor.h"

class CompressedModuleDescriptor : public Descriptor
{
	protected:
		unsigned compressionMethod			: 8;
		unsigned originalSize				: 32;

	public:
		CompressedModuleDescriptor(const uint8_t * const buffer);

		uint8_t getCompressionMethod(void) const;
		uint32_t getOriginalSize(void) const;
};

#endif /* __compressed_module_descriptor_h__ */
