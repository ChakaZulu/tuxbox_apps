/*
 * $Id: fta_content_management_descriptor.h,v 1.1 2009/06/29 16:18:27 mws Exp $
 *
 * Copyright (C) 2009 mws@twisted-brains.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __fta_content_management_descriptor_h__
#define __fta_content_management_descriptor_h__

#include "descriptor.h"

class FtaContentManagementDescriptor : public Descriptor
{
	protected:
		unsigned doNotScramble				: 1;
		unsigned controlRemoteAccessOverInternet	: 2;
		unsigned doNotApplyRevocation			: 1;

	public:
		FtaContentManagementDescriptor(const uint8_t * const buffer);
		virtual ~FtaContentManagementDescriptor() {};

		uint8_t getDoNotScramble(void) const;
		uint8_t getControlRemoteAccessOverInternet(void) const;
		uint8_t getDoNotApplyRevocation(void) const;
};

#endif /* __fta_content_management_descriptor_h__ */
