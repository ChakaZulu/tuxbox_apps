/*
 *  $Id: related_content_descriptor.h,v 1.1 2005/11/10 23:55:32 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __related_content_descriptor_h__
#define __related_content_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> RelatedContentByteVector;
typedef RelatedContentByteVector::iterator RelatedContentByteIterator;
typedef RelatedContentByteVector::const_iterator RelatedContentByteConstIterator;

class RelatedContentDescriptor : public Descriptor
{
	protected:
		RelatedContentByteVector bytes;

	public:
		RelatedContentDescriptor(const uint8_t* const buffer);
		virtual ~RelatedContentDescriptor();

		const RelatedContentByteVector* getRelatedContentBytes(void) const;
};

#endif /* __related_content_descriptor_h__*/
