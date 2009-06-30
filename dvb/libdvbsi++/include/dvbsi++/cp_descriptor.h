/*
 * $Id: cp_descriptor.h,v 1.1 2009/06/30 12:03:02 mws Exp $
 *
 * Copyright (C) 2009 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __cp_descriptor_h__
#define __cp_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> CpDataByteVector;
typedef CpDataByteVector::iterator CpDataByteIterator;
typedef CpDataByteVector::const_iterator CpDataByteConstIterator;

class CpDescriptor : public Descriptor
{
	protected:
		unsigned cpSystemId				: 16;
		unsigned cpPid					: 13;
		CpDataByteVector cpDataBytes;

	public:
		CpDescriptor(const uint8_t * const buffer);

		uint16_t getCpSystemId(void) const;
		uint16_t getCpPid(void) const;
		const CpDataByteVector *getCpDataBytes(void) const;
};

#endif /* __cp_descriptor_h__ */
