/*
 * $Id: cp_identifier_descriptor.h,v 1.1 2009/06/30 12:03:02 mws Exp $
 *
 * Copyright (C) 2009 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __cp_identifier_descriptor_h__
#define __cp_identifier_descriptor_h__

#include "descriptor.h"

typedef std::list<uint16_t> CpSystemIdList;
typedef CpSystemIdList::iterator CpSystemIdIterator;
typedef CpSystemIdList::const_iterator CpSystemIdConstIterator;

class CpIdentifierDescriptor : public Descriptor
{
	protected:
		CpSystemIdList cpSystemIds;

	public:
		CpIdentifierDescriptor(const uint8_t * const buffer);

		const CpSystemIdList *getCpSystemIds(void) const;
};

#endif /* __cp_identifier_descriptor_h__ */
