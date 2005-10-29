/*
 * $Id: ca_message_section.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __ca_message_section_h__
#define __ca_message_section_h__

#include "short_section.h"

class ConditionalAccessMessageSection : public ShortSection
{
	protected:
		std::list<uint8_t> caDataByte;

	public:
		ConditionalAccessMessageSection(const uint8_t * const buffer);

		static const uint16_t LENGTH = 256;
		static const enum TableId TID = TID_CAMT_ECM_0;
};

typedef std::list<ConditionalAccessMessageSection *> ConditionalAccessMessageSectionList;
typedef ConditionalAccessMessageSectionList::iterator ConditionalAccessMessageSectionIterator;
typedef ConditionalAccessMessageSectionList::const_iterator ConditionalAccessMessageSectionConstIterator;

#endif /* __ca_message_section_h__ */
