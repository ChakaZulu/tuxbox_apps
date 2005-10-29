/*
 * $Id: vbi_data_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
