/*
 * $Id: plugin_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
 
#ifndef __plugin_descriptor_h__
#define __plugin_descriptor_h__

#include "application_profile.h"
#include "descriptor.h"

class PluginDescriptor : public Descriptor
{
	protected:
		unsigned applicationType			: 16;
		ApplicationProfileList applicationProfiles;

	public:
		PluginDescriptor(const uint8_t * const buffer);
		~PluginDescriptor(void);

		uint16_t getApplicationType(void) const;
		const ApplicationProfileList *getApplicationProfiles(void) const;
};

#endif /* __plugin_descriptor_h__ */
