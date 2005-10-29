/*
 * $Id: target_background_grid_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __target_background_grid_descriptor_h__
#define __target_background_grid_descriptor_h__

#include "descriptor.h"

class TargetBackgroundGridDescriptor : public Descriptor
{
	protected:
		unsigned horizontalSize				: 14;
		unsigned verticalSize				: 14;
		unsigned aspectRatioInformation			: 4;

	public:
		TargetBackgroundGridDescriptor(const uint8_t * const buffer);

		uint16_t getHorizontalSize(void) const;
		uint16_t getVerticalSize(void) const;
		uint8_t getAspectRatioInformation(void) const;
};

#endif /* __target_background_grid_descriptor_h__ */
