/*
 * $Id: section_handler.h,v 1.1 2003/07/17 01:07:35 obi Exp $
 *
 * Copyright (C) 2002, 2003 Andreas Oberritter <obi@saftware.de>
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

#ifndef __dvb_pool_section_handler_h__
#define __dvb_pool_section_handler_h__

#include <inttypes.h>

/**
 * An abstract SectionHandler class.
 * @see SectionPool
 */

class SectionHandler
{
public:
	/**
	 * Release is called by the SectionPool after the last section has
	 * been delivered.
	 */
	void release(void) {};

	/**
	 * A pure virtual callback for a single section.
	 * @param data pointer to the beginning of a section.
	 * @param size number of data bytes, typically section_length + 3.
	 * @return false if more sections belong to the current table,
	 *         true when all sections have been processed.
	 */
	virtual bool section(const uint8_t * const data, const uint16_t size) = 0;
};

#endif /* __dvb_pool_section_handler_h__ */
