/*
 * $Id: dvb_j_application_location_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __dvb_j_application_location_descriptor_h__
#define __dvb_j_application_location_descriptor_h__

#include "descriptor.h"

class DvbJApplicationLocationDescriptor : public Descriptor
{
	protected:
		uint8_t baseDirectoryLength			: 8;
		std::string baseDirectory;
		uint8_t classpathExtensionLength		: 8;
		std::string classpathExtension;
		std::string initialClass;

	public:
		DvbJApplicationLocationDescriptor(const uint8_t * const buffer);

		const std::string &getBaseDirectory(void) const;
		const std::string &getClasspathExtension(void) const;
		const std::string &getInitialClass(void) const;
};

#endif /* __dvb_j_application_location_descriptor_h__ */

