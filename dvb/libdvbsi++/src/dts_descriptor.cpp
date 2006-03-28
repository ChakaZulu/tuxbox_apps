/*
 * $Id: dts_descriptor.cpp,v 1.2 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/dts_descriptor.h"

DTSDescriptor::DTSDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(5);

	sampleRate = (buffer[2] >> 4) & 0x0f;
	bitRate = ((buffer[2] & 0x0f) << 2) | (buffer[3] >> 6) & 0x02;
	numberOfBlocks = ((buffer[3] & 0x3f) << 2) | (buffer[4] >> 7) & 0x01;
	frameSize = ((buffer[4] & 0x7f) << 7) | (buffer[5] >> 1);
	surroundMode = ((buffer[5] & 0x01) << 6) | (buffer[6] >> 3) & 0x1f;
	lfeFlag = (buffer[6] >> 2) & 0x01;
	extendedSurroundFlag = buffer[6] & 0x03;

	additionalInfoBytes.resize(descriptorLength - 5);
	memcpy(&additionalInfoBytes[0], &buffer[7], descriptorLength - 5);
}

DTSDescriptor::~DTSDescriptor()
{
}

uint8_t DTSDescriptor::getSampleRate() const
{
	return sampleRate;
}

uint8_t DTSDescriptor::getBitRate() const
{
	return bitRate;
}

uint8_t DTSDescriptor::getNumberOfBlocks() const
{
	return numberOfBlocks;
}

uint16_t DTSDescriptor::getFrameSize() const
{
	return frameSize;
}

uint8_t DTSDescriptor::getLfeFlag() const
{
	return lfeFlag;
}

uint8_t DTSDescriptor::getExtendedSurroundFlag() const
{
	return extendedSurroundFlag;
}

const AdditionalInfoByteVector *DTSDescriptor::getAdditionalInfoBytes() const
{
	return &additionalInfoBytes;
}
