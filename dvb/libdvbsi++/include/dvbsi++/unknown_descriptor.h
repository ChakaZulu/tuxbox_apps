/*
 * $Id: unknown_descriptor.h,v 1.1 2005/09/30 16:14:42 ghostrider Exp $
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

#ifndef __unknown_descriptor_h__
#define __unknown_descriptor_h__

#include "descriptor.h"

class UnknownDescriptor : public Descriptor
{
	protected:
		std::vector<uint8_t> dataBytes;

	public:
		UnknownDescriptor(const uint8_t * const buffer);

		virtual size_t writeToBuffer(uint8_t * const buffer) const;
};

#endif /* __unknown_descriptor_h__ */
