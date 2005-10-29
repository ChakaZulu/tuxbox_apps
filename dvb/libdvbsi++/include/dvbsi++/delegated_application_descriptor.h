/*
 * $Id: delegated_application_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __delegated_application_descriptor_h__
#define __delegated_application_descriptor_h__

#include "application_identifier.h"
#include "descriptor.h"

class DelegatedApplicationDescriptor : public Descriptor
{
	protected:
		ApplicationIdentifierList applicationIdentifiers;

	public:
		DelegatedApplicationDescriptor(const uint8_t * const buffer);
		~DelegatedApplicationDescriptor(void);

		const ApplicationIdentifierList *getApplicationIdentifiers(void) const;
};

#endif /* __delegated_application_descriptor_h__ */
