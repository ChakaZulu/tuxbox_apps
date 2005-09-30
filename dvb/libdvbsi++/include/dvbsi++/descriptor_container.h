/*
 * $Id: descriptor_container.h,v 1.4 2005/09/30 16:13:49 ghostrider Exp $
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

#ifndef __descriptor_container_h__
#define __descriptor_container_h__

#include "descriptor.h"

enum DescriptorScope {
	SCOPE_SI,
	SCOPE_CAROUSEL,
	SCOPE_MHP
};

class DescriptorContainer
{
	protected:
		void descriptor(const uint8_t * const buffer, const enum DescriptorScope scope, bool back=true);
		void descriptorSi(const uint8_t * const buffer, bool back=true);
		void descriptorCarousel(const uint8_t * const buffer, bool back=true);
		void descriptorMhp(const uint8_t * const buffer, bool back=true);
		DescriptorList descriptorList;

	public:
		~DescriptorContainer(void);

		const DescriptorList *getDescriptors(void) const;
};

#endif /* __descriptor_container_h__ */
