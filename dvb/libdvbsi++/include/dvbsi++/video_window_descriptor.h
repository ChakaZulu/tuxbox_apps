/*
 * $Id: video_window_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __video_window_descriptor_h__
#define __video_window_descriptor_h__

#include "descriptor.h"

class VideoWindowDescriptor : public Descriptor
{
	protected:
		unsigned horizontalOffset			: 14;
		unsigned verticalOffset				: 14;
		unsigned windowPriority				: 4;

	public:
		VideoWindowDescriptor(const uint8_t * const buffer);

		uint16_t getHorizontalOffset(void) const;
		uint16_t getVerticalOffset(void) const;
		uint8_t getWindowPriority(void) const;
};

#endif /* __video_window_descriptor_h__ */
