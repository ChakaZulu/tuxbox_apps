/*
 * $Id: application_signalling_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __application_signalling_descriptor_h__
#define __application_signalling_descriptor_h__

#include "descriptor.h"

class ApplicationSignalling
{
	protected:
		unsigned applicationType			: 16;
		unsigned aitVersionNumber			: 5;

	public:
		ApplicationSignalling(const uint8_t * const buffer);

		uint16_t getApplicationType(void) const;
		uint8_t getAitVersionNumber(void) const;
};

typedef std::list<ApplicationSignalling *> ApplicationSignallingList;
typedef ApplicationSignallingList::iterator ApplicationSignallingIterator;
typedef ApplicationSignallingList::const_iterator ApplicationSignallingConstIterator;

class ApplicationSignallingDescriptor : public Descriptor
{
	protected:
		ApplicationSignallingList applicationSignallings;

	public:
		ApplicationSignallingDescriptor(const uint8_t * const buffer);
		~ApplicationSignallingDescriptor(void);

		const ApplicationSignallingList *getApplicationSignallings(void) const;
};

#endif /* __application_signalling_descriptor_h__ */
