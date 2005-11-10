/*
 *  $Id: time_slice_fec_identifier_descriptor.h,v 1.1 2005/11/10 23:55:32 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __time_slice_fec_identifier_descriptor_h__
#define __time_slice_fec_identifier_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> TimeSliceFecIdentifierByteVector;
typedef TimeSliceFecIdentifierByteVector::iterator TimeSliceFecIdentifierByteIterator;
typedef TimeSliceFecIdentifierByteVector::const_iterator TimeSliceFecIdentifierByteConstIterator;

class TimeSliceFecIdentifierDescriptor : public Descriptor
{
	protected:
		unsigned timeSlicing			: 1;
		unsigned mpeFec				: 2;
		// 2 bits reserved
		unsigned frameSize			: 3;
		unsigned maxBurstDuration		: 8;
		unsigned maxAverageRate			: 4;
		unsigned timeSliceFecId			: 4;

		TimeSliceFecIdentifierByteVector idSelectorBytes;

	public:
		TimeSliceFecIdentifierDescriptor(const uint8_t* const buffer);
		virtual ~TimeSliceFecIdentifierDescriptor();

		uint8_t getTimeSlicing() const;
		uint8_t getMpeFec() const;
		uint8_t getFrameSize() const;
		uint8_t getMaxBurstDuration() const;
		uint8_t getMaxAverageRate() const;
		uint8_t getTimeSliceFecId() const;

		const TimeSliceFecIdentifierByteVector* getIdSelectorBytes() const;
};

#endif /* __time_slice_fec_identifier_descriptor_h__*/
