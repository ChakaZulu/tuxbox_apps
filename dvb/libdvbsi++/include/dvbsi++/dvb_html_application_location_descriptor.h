/*
 * $Id: dvb_html_application_location_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __dvb_html_application_location_descriptor_h__
#define __dvb_html_application_location_descriptor_h__

#include "descriptor.h"

class DvbHtmlApplicationLocationDescriptor : public Descriptor
{
	protected:
		uint8_t physicalRootLength			: 8;
		std::string physicalRoot;
		std::string initialPath;

	public:
		DvbHtmlApplicationLocationDescriptor(const uint8_t * const buffer);

		const std::string &getPhysicalRoot(void) const;
		const std::string &getInitialPath(void) const;
};

#endif /* __dvb_html_application_location_descriptor_h__ */
