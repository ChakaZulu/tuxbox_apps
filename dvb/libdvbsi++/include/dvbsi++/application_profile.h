/*
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

#ifndef __application_profile_h__
#define __application_profile_h__

#include "compat.h"

class ApplicationProfile
{
	protected:
		unsigned applicationProfile			: 16;
		unsigned versionMajor				: 8;
		unsigned versionMinor				: 8;
		unsigned versionMicro				: 8;

	public:
		ApplicationProfile(const uint8_t * const buffer);
		
		uint16_t getApplicationProfile(void) const;
		uint8_t getVersionMajor(void) const;
		uint8_t getVersionMinor(void) const;
		uint8_t getVersionMicro(void) const;
};

typedef std::list<ApplicationProfile *> ApplicationProfileList;
typedef ApplicationProfileList::iterator ApplicationProfileIterator;
typedef ApplicationProfileList::const_iterator ApplicationProfileConstIterator;

#endif /* __application_profile_h__ */
