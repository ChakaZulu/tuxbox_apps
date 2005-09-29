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

#ifndef __dvb_j_application_descriptor_h__
#define __dvb_j_application_descriptor_h__

#include "descriptor.h"

class DvbJApplication
{
	protected:
		unsigned parameterLength			: 8;
		std::string parameter;

	public:
		DvbJApplication(const uint8_t * const buffer);

		uint8_t getParameterLength(void) const;
		const std::string &getParameter(void) const;
};

typedef std::list<DvbJApplication *> DvbJApplicationList;
typedef DvbJApplicationList::iterator DvbJApplicationIterator;
typedef DvbJApplicationList::const_iterator DvbJApplicationConstIterator;

class DvbJApplicationDescriptor : public Descriptor
{
	protected:
		DvbJApplicationList dvbJApplications;

	public:
		DvbJApplicationDescriptor(const uint8_t * const buffer);
		~DvbJApplicationDescriptor(void);

		const DvbJApplicationList *getDvbJApplications(void) const;
};

#endif /* __dvb_j_application_descriptor_h__ */
