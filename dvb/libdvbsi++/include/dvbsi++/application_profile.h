/*
 * $Id: application_profile.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __application_profile_h__
#define __application_profile_h__

#include "compat.h"

class ApplicationProfile
{
	protected:
		unsigned applicationProfile			: 16;
		unsigned versionMajor				: 8;
		unsigned versionMinor				: 8;
		unsigned versionMicro				: 8;

	public:
		ApplicationProfile(const uint8_t * const buffer);
		
		uint16_t getApplicationProfile(void) const;
		uint8_t getVersionMajor(void) const;
		uint8_t getVersionMinor(void) const;
		uint8_t getVersionMicro(void) const;
};

typedef std::list<ApplicationProfile *> ApplicationProfileList;
typedef ApplicationProfileList::iterator ApplicationProfileIterator;
typedef ApplicationProfileList::const_iterator ApplicationProfileConstIterator;

#endif /* __application_profile_h__ */
