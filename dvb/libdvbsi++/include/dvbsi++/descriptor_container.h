/*
 * $Id: descriptor_container.h,v 1.5 2005/10/29 00:10:08 obi Exp $
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
