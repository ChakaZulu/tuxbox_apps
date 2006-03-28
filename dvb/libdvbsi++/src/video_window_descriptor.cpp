/*
 * $Id: video_window_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/video_window_descriptor.h>

VideoWindowDescriptor::VideoWindowDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(4);

	horizontalOffset = UINT16(&buffer[2]) >> 2;
	verticalOffset = ((buffer[3] & 0x03) << 12) | (UINT16(&buffer[4]) >> 4);
	windowPriority = buffer[5] & 0x0F;
}

uint16_t VideoWindowDescriptor::getHorizontalOffset(void) const
{
	return horizontalOffset;
}

uint16_t VideoWindowDescriptor::getVerticalOffset(void) const
{
	return verticalOffset;
}

uint8_t VideoWindowDescriptor::getWindowPriority(void) const
{
	return windowPriority;
}

