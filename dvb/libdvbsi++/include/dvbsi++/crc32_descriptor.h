/*
 * $Id: crc32_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __crc32_descriptor_h__
#define __crc32_descriptor_h__

#include "descriptor.h"

class Crc32Descriptor : public Descriptor
{
	protected:
		unsigned crc32				: 32;

	public:
		Crc32Descriptor(const uint8_t * const buffer);

		uint32_t getCrc32(void) const;
};

#endif /* __crc32_descriptor_h__ */
