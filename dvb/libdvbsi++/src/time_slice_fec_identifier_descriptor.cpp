/*
 * $Id: time_slice_fec_identifier_descriptor.cpp,v 1.2 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/time_slice_fec_identifier_descriptor.h"

TimeSliceFecIdentifierDescriptor::TimeSliceFecIdentifierDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(3);

	timeSlicing = (buffer[2] >> 7) & 0x01;
	mpeFec = (buffer[2] >> 5) & 0x03;
	frameSize = buffer[2] & 0x03;
	maxBurstDuration = buffer[3];
	maxAverageRate = (buffer[4] >> 4) & 0x0f;
	timeSliceFecId = buffer[4] & 0x0f;

	idSelectorBytes.resize(descriptorLength - 3);
	memcpy(&idSelectorBytes[0], buffer+5, descriptorLength - 3);
}

TimeSliceFecIdentifierDescriptor::~TimeSliceFecIdentifierDescriptor()
{
}

uint8_t TimeSliceFecIdentifierDescriptor::getTimeSlicing() const
{
	return timeSlicing;
}

uint8_t TimeSliceFecIdentifierDescriptor::getMpeFec() const
{
	return mpeFec;
}

uint8_t TimeSliceFecIdentifierDescriptor::getFrameSize() const
{
	return frameSize;
}

uint8_t TimeSliceFecIdentifierDescriptor::getMaxBurstDuration() const
{
	return maxBurstDuration;
}

uint8_t TimeSliceFecIdentifierDescriptor::getMaxAverageRate() const
{
	return maxAverageRate;
}

uint8_t TimeSliceFecIdentifierDescriptor::getTimeSliceFecId() const
{
	return timeSliceFecId;
}

const TimeSliceFecIdentifierByteVector* TimeSliceFecIdentifierDescriptor::getIdSelectorBytes() const
{
	return &idSelectorBytes;
}
