/*
 * $Id: ac3_descriptor.h,v 1.3 2005/10/29 00:10:07 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __ac3_descriptor_h__
#define __ac3_descriptor_h__

#include "descriptor.h"

typedef std::list<uint8_t> AdditionalInfoList;
typedef AdditionalInfoList::iterator AdditionalInfoIterator;
typedef AdditionalInfoList::const_iterator AdditionalInfoConstIterator;

class Ac3Descriptor : public Descriptor
{
	protected:
		unsigned ac3TypeFlag				: 1;
		unsigned bsidFlag				: 1;
		unsigned mainidFlag				: 1;
		unsigned asvcFlag				: 1;
		unsigned ac3Type				: 8;
		unsigned bsid					: 8;
		unsigned mainid					: 8;
		unsigned avsc					: 8;
		AdditionalInfoList additionalInfo;

	public:
		Ac3Descriptor(const uint8_t * const buffer);

		uint8_t getAc3TypeFlag(void) const;
		uint8_t getBsidFlag(void) const;
		uint8_t getMainidFlag(void) const;
		uint8_t getAsvcFlag(void) const;
		uint8_t getAc3Type(void) const;
		uint8_t getBsid(void) const;
		uint8_t getMainid(void) const;
		uint8_t getAvsc(void) const;
		const AdditionalInfoList *getAdditionalInfo(void) const;
};

#endif /* __ac3_descriptor_h__ */
