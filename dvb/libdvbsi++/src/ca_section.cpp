/*
 * $Id: ca_section.cpp,v 1.2 2004/05/31 21:21:23 obi Exp $
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

#include <dvbsi++/ca_section.h>

ConditionalAccessSection::ConditionalAccessSection(const uint8_t * const buffer) : LongCrcSection(buffer)
{
	for (size_t i = 8; i < sectionLength - 1; i += buffer[i + 1] + 2)
		descriptor(&buffer[i], SCOPE_SI);
}

