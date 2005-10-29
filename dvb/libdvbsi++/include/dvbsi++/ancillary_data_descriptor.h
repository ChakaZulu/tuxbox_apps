/*
 * $Id: ancillary_data_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __ancillary_data_descriptor_h__
#define __ancillary_data_descriptor_h__

#include "descriptor.h"

class AncillaryDataDescriptor : public Descriptor
{
	protected:
		unsigned ancillaryDataIdentifier		: 8;

	public:
		AncillaryDataDescriptor(const uint8_t * const buffer);

		uint8_t getAncillaryDataIdentifier(void) const;
};

#endif /* __ancillary_data_descriptor_h__ */
