/*
 * $Id: time_offset_section.h,v 1.1 2004/02/13 15:27:38 obi Exp $
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

#ifndef __time_offset_section_h__
#define __time_offset_section_h__

#include "descriptor_container.h"
#include "short_crc_section.h"

class TimeOffsetSection : public ShortCrcSection, public DescriptorContainer
{
	protected:
		unsigned utcTimeMjd				: 16;
		unsigned utcTimeBcd				: 24;
		unsigned descriptorsLoopLength			: 12;

	public:
		TimeOffsetSection(const uint8_t * const buffer);

		static const enum PacketId PID = PID_TOT;
		static const enum TableId TID = TID_TOT;
		static const uint32_t TIMEOUT = 36000;

		uint16_t getUtcTimeMjd(void) const;
		uint32_t getUtcTimeBcd(void) const;
};

typedef std::vector<TimeOffsetSection *> TimeOffsetSectionVector;
typedef TimeOffsetSectionVector::iterator TimeOffsetSectionIterator;
typedef TimeOffsetSectionVector::const_iterator TimeOffsetSectionConstIterator;

#endif /* __time_offset_section_h__ */
