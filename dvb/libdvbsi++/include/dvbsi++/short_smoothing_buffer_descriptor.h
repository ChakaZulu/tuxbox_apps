/*
 *  $Id: short_smoothing_buffer_descriptor.h,v 1.1 2008/03/31 08:16:23 mws Exp $
 *
 *  Copyright (C) 2008 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __short_smoothing_buffer_descriptor_h__
#define __short_smoothing_buffer_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> PrivateDataByteVector;
typedef PrivateDataByteVector::iterator PrivateDataByteIterator;
typedef PrivateDataByteVector::const_iterator PrivateDataByteConstIterator;

class ShortSmoothingBufferDescriptor : public Descriptor
{
	protected:
		unsigned sbSize			: 2;
		unsigned sbLeakRate		: 6;
		PrivateDataByteVector privateDataBytes;

	public:
		ShortSmoothingBufferDescriptor(const uint8_t* const buffer);
		virtual ~ShortSmoothingBufferDescriptor();

		uint8_t getSbSize(void) const;
		uint8_t getSbLeakRate(void) const;
		const PrivateDataByteVector *getPrivateDataBytes(void) const;
};

#endif /* __short_smoothing_buffer_descriptor_h__*/
