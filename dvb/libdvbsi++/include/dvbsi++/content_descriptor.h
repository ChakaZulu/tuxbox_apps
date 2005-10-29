/*
 * $Id: content_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
