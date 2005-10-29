/*
 * $Id: bouquet_name_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __bouquet_name_descriptor_h__
#define __bouquet_name_descriptor_h__

#include "descriptor.h"

class BouquetNameDescriptor : public Descriptor
{
	protected:
		std::string bouquetName;

	public:
		BouquetNameDescriptor(const uint8_t * const buffer);
		~BouquetNameDescriptor(void);

		const std::string &getBouquetName(void) const;
};

#endif /* __bouquet_name_descriptor_h__ */
