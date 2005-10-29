/*
 * $Id: ca_identifier_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __ca_identifier_descriptor_h__
#define __ca_identifier_descriptor_h__

#include "descriptor.h"

typedef std::list<uint16_t> CaSystemIdList;
typedef CaSystemIdList::iterator CaSystemIdIterator;
typedef CaSystemIdList::const_iterator CaSystemIdConstIterator;

class CaIdentifierDescriptor : public Descriptor
{
	protected:
		CaSystemIdList caSystemIds;

	public:
		CaIdentifierDescriptor(const uint8_t * const buffer);

		const CaSystemIdList *getCaSystemIds(void) const;
};

#endif /* __ca_identifier_descriptor_h__ */
