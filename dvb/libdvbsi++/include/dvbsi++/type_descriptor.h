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

#ifndef __type_descriptor_h__
#define __type_descriptor_h__

#include "descriptor.h"

class TypeDescriptor : public Descriptor
{
	protected:
		std::string type;

	public:
		TypeDescriptor(const uint8_t * const buffer);

		const std::string &getType(void) const;
};

#endif /* __type_descriptor_h__ */
