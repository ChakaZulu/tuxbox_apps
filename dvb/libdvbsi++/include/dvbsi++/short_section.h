/*
 * $Id: short_section.h,v 1.5 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __short_section_h__
#define __short_section_h__

#include "compat.h"
#include "packet_id.h"
#include "table_id.h"

class ShortSection
{
	protected:
		unsigned tableId				: 8;
		unsigned sectionSyntaxIndicator			: 1;
		unsigned sectionLength				: 12;

	public:
		ShortSection(const uint8_t * const buffer);
		virtual ~ShortSection() { };

		static const uint8_t CRC32 = 0;
		static const uint16_t LENGTH = 1024;
		static const enum PacketId PID = PID_RESERVED;
		static const uint8_t SYNTAX = 0;
		static const enum TableId TID = TID_RESERVED;
		static const uint32_t TIMEOUT = 0;

		uint8_t getTableId(void) const;
		uint8_t getSectionSyntaxIndicator(void) const;
		uint16_t getSectionLength(void) const;
};

typedef std::list<ShortSection *> ShortSectionList;
typedef ShortSectionList::iterator ShortSectionIterator;
typedef ShortSectionList::const_iterator ShortSectionConstIterator;

#endif /* __short_section_h__ */
