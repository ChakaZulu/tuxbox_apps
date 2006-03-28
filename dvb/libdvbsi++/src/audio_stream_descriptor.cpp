/*
 * $Id: audio_stream_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/audio_stream_descriptor.h>

AudioStreamDescriptor::AudioStreamDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(1);

	freeFormatFlag = (buffer[2] >> 7) & 0x01;
	id = (buffer[2] >> 6) & 0x01;
	layer = (buffer[2] >> 4) & 0x03;
	variableRateAudioIndicator = (buffer[2] >> 3) & 0x01;
}

uint8_t AudioStreamDescriptor::getFreeFormatFlag(void) const
{
	return freeFormatFlag;
}

uint8_t AudioStreamDescriptor::getId(void) const
{
	return id;
}

uint8_t AudioStreamDescriptor::getLayer(void) const
{
	return layer;
}

uint8_t AudioStreamDescriptor::getVariableRateAudioIndicator(void) const
{
	return variableRateAudioIndicator;
}

