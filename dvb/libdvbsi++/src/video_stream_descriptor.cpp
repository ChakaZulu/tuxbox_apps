/*
 * $Id: video_stream_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/video_stream_descriptor.h>

VideoStreamDescriptor::VideoStreamDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(1);

	multipleFrameRateFlag = (buffer[2] >> 7) & 0x01;
	frameRateCode = (buffer[2] >> 3) & 0x0F;
	mpeg1OnlyFlag = (buffer[2] >> 2) & 0x01;
	constrainedParameterFlag = (buffer[2] >> 1) & 0x01;

	if (!mpeg1OnlyFlag) {
		ASSERT_MIN_DLEN(3);

		profileAndLevelIndication = buffer[3];
		chromaFormat = (buffer[4] >> 6) & 0x03;
		frameRateExtensionFlag = (buffer[4] >> 5) & 0x01;
	}
}

uint8_t VideoStreamDescriptor::getMultipleFrameRateFlag(void) const
{
	return multipleFrameRateFlag;
}

uint8_t VideoStreamDescriptor::getFrameRateCode(void) const
{
	return frameRateCode;
}

uint8_t VideoStreamDescriptor::getMpeg1OnlyFlag(void) const
{
	return mpeg1OnlyFlag;
}

uint8_t VideoStreamDescriptor::getConstrainedParameterFlag(void) const
{
	return constrainedParameterFlag;
}

uint8_t VideoStreamDescriptor::getProfileAndLevelIndication(void) const
{
	return profileAndLevelIndication;
}

uint8_t VideoStreamDescriptor::getChromaFormat(void) const
{
	return chromaFormat;
}

uint8_t VideoStreamDescriptor::getFrameRateExtensionFlag(void) const
{
	return frameRateExtensionFlag;
}

