/*
 * $Id: delegated_application_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/delegated_application_descriptor.h>

DelegatedApplicationDescriptor::DelegatedApplicationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 6) {
		ASSERT_MIN_DLEN(i + 6);
		applicationIdentifiers.push_back(new ApplicationIdentifier(&buffer[i + 2]));
	}
}

DelegatedApplicationDescriptor::~DelegatedApplicationDescriptor(void)
{
	for (ApplicationIdentifierIterator i = applicationIdentifiers.begin(); i != applicationIdentifiers.end(); ++i)
		delete *i;
}

const ApplicationIdentifierList *DelegatedApplicationDescriptor::getApplicationIdentifiers(void) const
{
	return &applicationIdentifiers;
}
