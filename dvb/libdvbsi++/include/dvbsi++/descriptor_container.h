/*
 * $Id: descriptor_container.h,v 1.6 2006/03/28 17:21:59 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
	private:
		Descriptor *descriptorSi(const uint8_t * const buffer, bool back = true);
		Descriptor *descriptorCarousel(const uint8_t * const buffer, bool back = true);
		Descriptor *descriptorMhp(const uint8_t * const buffer, bool back = true);

	protected:
		void descriptor(const uint8_t * const buffer, const enum DescriptorScope scope, bool back = true);
		DescriptorList descriptorList;

	public:
		~DescriptorContainer(void);

		const DescriptorList *getDescriptors(void) const;
};

#endif /* __descriptor_container_h__ */
