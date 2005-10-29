/*
 * $Id: target_background_grid_descriptor.cpp,v 1.3 2005/10/29 00:10:17 obi Exp $
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
#include <dvbsi++/target_background_grid_descriptor.h>

TargetBackgroundGridDescriptor::TargetBackgroundGridDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	horizontalSize = UINT16(&buffer[2]) >> 2;
	verticalSize = ((buffer[3] & 0x03) << 12) | (UINT16(&buffer[4]) >> 4);
	aspectRatioInformation = buffer[5] & 0x0F;
}

uint16_t TargetBackgroundGridDescriptor::getHorizontalSize(void) const
{
	return horizontalSize;
}

uint16_t TargetBackgroundGridDescriptor::getVerticalSize(void) const
{
	return verticalSize;
}

uint8_t TargetBackgroundGridDescriptor::getAspectRatioInformation(void) const
{
	return aspectRatioInformation;
}

