/*
 *  $Id: default_authority_descriptor.h,v 1.1 2005/11/10 23:55:32 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __default_authority_descriptor_h__
#define __default_authority_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> DefaultAuthorityByteVector;
typedef DefaultAuthorityByteVector::iterator DefaultAuthorityByteIterator;
typedef DefaultAuthorityByteVector::const_iterator DefaultAuthorityByteConstIterator;

class DefaultAuthorityDescriptor : public Descriptor
{
	protected:
		DefaultAuthorityByteVector bytes;

	public:
		DefaultAuthorityDescriptor(const uint8_t* const buffer);
		virtual ~DefaultAuthorityDescriptor();

		const DefaultAuthorityByteVector* getAuthorityBytes(void) const;
};

#endif /* __default_authority_descriptor_h__*/
