/*
 * $Id: teletext_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __teletext_descriptor_h__
#define __teletext_descriptor_h__

#include "vbi_teletext_descriptor.h"

class TeletextDescriptor : public VbiTeletextDescriptor
{
	public:
		TeletextDescriptor(const uint8_t * const buffer);
};

#endif /* __teletext_descriptor_h__ */
