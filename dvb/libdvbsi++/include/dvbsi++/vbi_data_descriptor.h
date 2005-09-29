/*
 * $Id: vbi_data_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __vbi_data_descriptor_h__
#define __vbi_data_descriptor_h__

#include "descriptor.h"

class VbiDataLine
{
	protected:
		unsigned fieldParity				: 1;
		unsigned lineOffset				: 5;

	public:
		VbiDataLine(const uint8_t * const buffer);

		uint8_t getFieldParity(void) const;
		uint8_t getLineOffset(void) const;
};

typedef std::list<VbiDataLine *> VbiDataLineList;
typedef VbiDataLineList::iterator VbiDataLineIterator;
typedef VbiDataLineList::const_iterator VbiDataLineConstIterator;

class VbiDataService
{
	protected:
		unsigned dataServiceId				: 8;
		unsigned dataServiceDescriptorLength		: 8;
		VbiDataLineList vbiDataLines;

	public:
		VbiDataService(const uint8_t * const buffer);
		~VbiDataService(void);

		uint8_t getDataServiceId(void) const;
		const VbiDataLineList *getVbiDataLines(void) const;
};

typedef std::list<VbiDataService *> VbiDataServiceList;
typedef VbiDataServiceList::iterator VbiDataServiceIterator;
typedef VbiDataServiceList::const_iterator VbiDataServiceConstIterator;

class VbiDataDescriptor : public Descriptor
{
	protected:
		VbiDataServiceList vbiDataServices;

	public:
		VbiDataDescriptor(const uint8_t * const buffer);
		~VbiDataDescriptor(void);

		const VbiDataServiceList *getVbiDataServices(void) const;
};

#endif /* __vbi_data_descriptor_h__ */
