/*
 * $Id: application_icons_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __application_icons_descriptor_h__
#define __application_icons_descriptor_h__

#include "descriptor.h"

class ApplicationIconsDescriptor : public Descriptor
{
	protected:
		unsigned iconLocatorLength			: 8;
		std::string iconLocator;
		unsigned iconFlags				: 16;

	public:
		ApplicationIconsDescriptor(const uint8_t * const buffer);

		const std::string &getIconLocator(void) const;
		uint16_t getIconFlags(void) const;
};

#endif /* __application_icons_descriptor_h__ */
