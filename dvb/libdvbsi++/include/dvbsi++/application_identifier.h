/*
 * $Id: application_identifier.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __application_identifier_h__
#define __application_identifier_h__

#include "compat.h"

class ApplicationIdentifier
{
	protected:
		unsigned organisationId				: 32;
		unsigned applicationId				: 16;

	public:
		ApplicationIdentifier(const uint8_t * const buffer);

		uint32_t getOrganisationId(void) const;
		uint16_t getApplicationId(void) const;
};

typedef std::list<ApplicationIdentifier *> ApplicationIdentifierList;
typedef ApplicationIdentifierList::iterator ApplicationIdentifierIterator;
typedef ApplicationIdentifierList::const_iterator ApplicationIdentifierConstIterator;

#endif /* __application_identifier_h__ */
