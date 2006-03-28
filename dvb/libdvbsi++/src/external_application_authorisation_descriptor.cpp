/*
 * $Id: external_application_authorisation_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
 
#include <dvbsi++/external_application_authorisation_descriptor.h>

ExternalApplicationAuthorisation::ExternalApplicationAuthorisation(const uint8_t
* const buffer)
{
	applicationIdentifier = new ApplicationIdentifier(&buffer[0]);
	applicationPriority = buffer[6];
}

ExternalApplicationAuthorisation::~ExternalApplicationAuthorisation(void)
{
	delete applicationIdentifier;
}

const ApplicationIdentifier
*ExternalApplicationAuthorisation::getApplicationIdentifier(void) const
{
	return applicationIdentifier;
}

uint8_t ExternalApplicationAuthorisation::getApplicationPriority(void) const
{
	return applicationPriority;
}

ExternalApplicationAuthorisationDescriptor::ExternalApplicationAuthorisationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 7) {
		ASSERT_MIN_DLEN(i + 7);
		externalApplicationAuthorisations.push_back(new ExternalApplicationAuthorisation(&buffer[i + 2]));
	}
}

ExternalApplicationAuthorisationDescriptor::~ExternalApplicationAuthorisationDescriptor(void)
{
	for (ExternalApplicationAuthorisationIterator i = externalApplicationAuthorisations.begin(); i != externalApplicationAuthorisations.end(); ++i)
		delete *i;
}

const ExternalApplicationAuthorisationList *ExternalApplicationAuthorisationDescriptor::getExternalApplicationAuthorisations(void) const
{
	return &externalApplicationAuthorisations;
}
