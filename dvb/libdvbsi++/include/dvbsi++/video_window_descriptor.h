/*
 * $Id: video_window_descriptor.h,v 1.1 2004/02/13 15:27:38 obi Exp $
 *
 * Copyright (C) 2002-2004 Andreas Oberritter <obi@saftware.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
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
