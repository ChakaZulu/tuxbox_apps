/*
 * $Id: bouquet_association_section.h,v 1.5 2006/09/26 20:34:32 mws Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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

		uint16_t getTransportStreamId(void) const;
		uint16_t getOriginalNetworkId(void) const;
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

		const BouquetAssociationList *getBouquets(void) const;
};

typedef std::list<BouquetAssociationSection *> BouquetAssociationSectionList;
typedef BouquetAssociationSectionList::iterator BouquetAssociationSectionIterator;
typedef BouquetAssociationSectionList::const_iterator BouquetAssociationSectionConstIterator;

#endif /* __bouquet_association_section_h__ */
