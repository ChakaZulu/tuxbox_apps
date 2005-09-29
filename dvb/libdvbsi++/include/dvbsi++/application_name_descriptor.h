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

#ifndef __application_name_descriptor_h__
#define __application_name_descriptor_h__

#include "descriptor.h"

class ApplicationName
{
	protected:
		std::string iso639LanguageCode;
		unsigned applicationNameLength			: 8;
		std::string applicationName;

	public:
		ApplicationName(const uint8_t * const buffer);

		const std::string &getIso639LanguageCode(void) const;
		const std::string &getApplicationName(void) const;
};

typedef std::list<ApplicationName *> ApplicationNameList;
typedef ApplicationNameList::iterator ApplicationNameIterator;
typedef ApplicationNameList::const_iterator ApplicationNameConstIterator;

class ApplicationNameDescriptor : public Descriptor
{
	protected:
		ApplicationNameList applicationNames;

	public:
		ApplicationNameDescriptor(const uint8_t * const buffer);
		~ApplicationNameDescriptor(void);

		const ApplicationNameList *getApplicationNames(void) const;
};

#endif /* __application_name_descriptor_h__ */
