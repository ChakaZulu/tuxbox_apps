/*
 * $Id: ip_signaling_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
 
#ifndef __ip_signaling_descriptor_h__
#define __ip_signaling_descriptor_h__

#include "descriptor.h"

class IpSignalingDescriptor : public Descriptor
{
	protected:
		unsigned platformId				: 24;

	public:
		IpSignalingDescriptor(const uint8_t * const buffer);

		uint32_t getPlatformId(void) const;
};

#endif /* __ip_signaling_descriptor_h__ */
