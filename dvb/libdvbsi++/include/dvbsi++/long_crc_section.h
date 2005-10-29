/*
 * $Id: long_crc_section.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __long_crc_section_h__
#define __long_crc_section_h__

#include "long_section.h"

class LongCrcSection : public LongSection
{
	protected:
		unsigned crc32					: 32;

	public:
		LongCrcSection(const uint8_t * const buffer);

		static const uint8_t CRC32 = 1;

		uint32_t getCrc32(void) const;
};

typedef std::list<LongCrcSection *> LongCrcSectionList;
typedef LongCrcSectionList::iterator LongCrcSectionIterator;
typedef LongCrcSectionList::const_iterator LongCrcSectionConstIterator;

#endif /* __long_crc_section_h__ */
