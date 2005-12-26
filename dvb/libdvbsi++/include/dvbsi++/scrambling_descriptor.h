/*
 *  $Id: scrambling_descriptor.h,v 1.1 2005/12/26 20:48:57 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __scrambling_descriptor_h__
#define __scrambling_descriptor_h__

#include "descriptor.h"

class ScramblingDescriptor : public Descriptor
{
	protected:
		unsigned scramblingMode		: 8;

	public:
		ScramblingDescriptor(const uint8_t* const buffer);
		virtual ~ScramblingDescriptor();

		uint8_t getScramblingMode() const;
};

#endif /* __scrambling_descriptor_h__*/
