/*
 * $Id: bouquet_association_section.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __bouquet_association_section_h__
#define __bouquet_association_section_h__

#include "descriptor_container.h"
#include "long_crc_section.h"

class BouquetAssociation : public DescriptorContainer
{
	protected:
		unsigned transportStreamId			: 16;
		unsigned originalNetworkId			: 16;
		unsigned transportStreamLoopLength		: 12;

	public:
		BouquetAssociation(const uint8_t * const buffer);
};

typedef std::list<BouquetAssociation *> BouquetAssociationList;
typedef BouquetAssociationList::iterator BouquetAssociationIterator;
typedef BouquetAssociationList::const_iterator BouquetAssociationConstIterator;

class BouquetAssociationSection : public LongCrcSection , public DescriptorContainer
{
	protected:
		unsigned bouquetDescriptorsLength		: 12;
		unsigned transportStreamLoopLength		: 12;
		BouquetAssociationList bouquet;

	public:
		BouquetAssociationSection(const uint8_t * const buffer);
		~BouquetAssociationSection(void);

		static const enum PacketId PID = PID_BAT;
		static const enum TableId TID = TID_BAT;
		static const uint32_t TIMEOUT = 12000;
};

typedef std::list<BouquetAssociationSection *> BouquetAssociationSectionList;
typedef BouquetAssociationSectionList::iterator BouquetAssociationSectionIterator;
typedef BouquetAssociationSectionList::const_iterator BouquetAssociationSectionConstIterator;

#endif /* __bouquet_association_section_h__ */
