/*
 * $Id: content_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __content_descriptor_h__
#define __content_descriptor_h__

#include "descriptor.h"

class ContentClassification
{
	protected:
		unsigned contentNibbleLevel1			: 4;
		unsigned contentNibbleLevel2			: 4;
		unsigned userNibble1				: 4;
		unsigned userNibble2				: 4;

	public:
		ContentClassification(const uint8_t * const buffer);

		uint8_t getContentNibbleLevel1(void) const;
		uint8_t getContentNibbleLevel2(void) const;
		uint8_t getUserNibble1(void) const;
		uint8_t getUserNibble2(void) const;
};

typedef std::list<ContentClassification *> ContentClassificationList;
typedef ContentClassificationList::iterator ContentClassificationIterator;
typedef ContentClassificationList::const_iterator ContentClassificationConstIterator;

class ContentDescriptor : public Descriptor
{
	protected:
		ContentClassificationList classifications;

	public:
		ContentDescriptor(const uint8_t * const buffer);
		~ContentDescriptor(void);

		const ContentClassificationList *getClassifications(void) const;
};

#endif /* __content_descriptor_h__ */
