/*
 * $Id: time_date_section.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __time_date_section_h__
#define __time_date_section_h__

#include "short_section.h"

class TimeAndDateSection : public ShortSection
{
	protected:
		unsigned utcTimeMjd				: 16;
		unsigned utcTimeBcd				: 24;

	public:
		TimeAndDateSection(const uint8_t * const buffer);

		static const enum PacketId PID = PID_TDT;
		static const enum TableId TID = TID_TDT;
		static const uint32_t TIMEOUT = 36000;

		uint16_t getUtcTimeMjd(void) const;
		uint32_t getUtcTimeBcd(void) const;
};

typedef std::list<TimeAndDateSection *> TimeAndDateSectionList;
typedef TimeAndDateSectionList::iterator TimeAndDateSectionIterator;
typedef TimeAndDateSectionList::const_iterator TimeAndDateSectionConstIterator;

#endif /* __time_date_section_h__ */
