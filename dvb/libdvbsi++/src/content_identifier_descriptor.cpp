/*
 * $Id: content_identifier_descriptor.cpp,v 1.2 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/content_identifier_descriptor.h"

#include "dvbsi++/byte_stream.h"

ContentReferenceIdentifier::ContentReferenceIdentifier(const uint8_t * const buffer)
{
	type = (buffer[0] >> 2) & 0x3f;
	location = buffer[0] & 0x03;

	if (location == 0) {
		length = buffer[1];
		cridBytes.reserve(length);
		memcpy(&cridBytes[0], buffer+2, length);
		reference = 0x0000;
	} else if (location == 1) {
		length = 0;
		reference = r16(&buffer[2]);
	}
	// else DVB reserved
}

ContentReferenceIdentifier::~ContentReferenceIdentifier()
{
}

uint8_t ContentReferenceIdentifier::getType() const
{
	return type;
}

uint8_t ContentReferenceIdentifier::getLocation() const
{
	return location;
}

uint8_t ContentReferenceIdentifier::getLength() const
{
	return length;
}

const ContentReferenceIdentifierByteVector *ContentReferenceIdentifier::getBytes() const
{
	return &cridBytes;
}

uint16_t ContentReferenceIdentifier::getReference() const
{
	return reference;
}


ContentIdentifierDescriptor::ContentIdentifierDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; /* inc inside loop */) {
		ContentReferenceIdentifier *crid = new ContentReferenceIdentifier(&buffer[2 + i]);

		if (crid->getLocation() == 0)
			i += crid->getLength() + 2;
		else if (crid->getLocation() == 1)
			i += 3;

		identifier.push_back(crid);
	}
}

ContentIdentifierDescriptor::~ContentIdentifierDescriptor()
{
	for (ContentReferenceIdentifierIterator it = identifier.begin(); it != identifier.end(); ++it)
		delete *it;
}

const ContentReferenceIdentifierList *ContentIdentifierDescriptor::getIdentifier() const
{
	return &identifier;
}
