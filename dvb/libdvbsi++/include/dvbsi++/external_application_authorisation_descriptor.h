/*
 * $Id: external_application_authorisation_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __external_application_authorisation_descriptor_h__
#define __external_application_authorisation_descriptor_h__

#include "application_identifier.h"
#include "descriptor.h"

class ExternalApplicationAuthorisation
{
	protected:
		ApplicationIdentifier *applicationIdentifier;
		unsigned applicationPriority			: 8;

	public:
		ExternalApplicationAuthorisation(const uint8_t * const buffer);
		~ExternalApplicationAuthorisation(void);
		const ApplicationIdentifier *getApplicationIdentifier(void) const;
		uint8_t getApplicationPriority(void) const;
};

typedef std::list<ExternalApplicationAuthorisation *> ExternalApplicationAuthorisationList;
typedef ExternalApplicationAuthorisationList::iterator ExternalApplicationAuthorisationIterator;
typedef ExternalApplicationAuthorisationList::const_iterator ExternalApplicationAuthorisationConstIterator;

class ExternalApplicationAuthorisationDescriptor : public Descriptor
{
	protected:
		ExternalApplicationAuthorisationList externalApplicationAuthorisations;

	public:
		ExternalApplicationAuthorisationDescriptor(const uint8_t * const buffer);
		~ExternalApplicationAuthorisationDescriptor(void);
		
		const ExternalApplicationAuthorisationList *getExternalApplicationAuthorisations(void) const;
};

#endif /* __external_application_authorisation_descriptor_h__ */
