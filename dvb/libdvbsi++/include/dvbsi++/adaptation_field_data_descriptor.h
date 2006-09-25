/*
 *  $Id: adaptation_field_data_descriptor.h,v 1.1 2006/09/25 18:58:48 mws Exp $
 *
 *  Copyright (C) 2006 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __adaptation_field_data_descriptor_h__
#define __adaptation_field_data_descriptor_h__

#include "descriptor.h"

class AdaptationFieldDataDescriptor : public Descriptor
{
	protected:
		unsigned adaptationFieldDataIdentifier		: 8;

	public:
		AdaptationFieldDataDescriptor(const uint8_t* const buffer);
		virtual ~AdaptationFieldDataDescriptor();

		uint8_t getAdaptationFieldDataIdentifier(void) const;
};

#endif /* __adaptation_field_data_descriptor_h__ */
