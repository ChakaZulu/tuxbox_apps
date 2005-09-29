/*
 * $Id: ca_identifier_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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
