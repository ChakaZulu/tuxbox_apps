/*
 * $Id: tva_id_descriptor.cpp,v 1.2 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/tva_id_descriptor.h"

#include "dvbsi++/byte_stream.h"

TVAIdentifier::TVAIdentifier(const uint8_t * const buffer)
{
	id = r16(&buffer[0]);
	runningStatus = buffer[2] & 0x03;
}

TVAIdentifier::~TVAIdentifier()
{
}

uint16_t TVAIdentifier::getId() const
{
	return id;
}

uint8_t TVAIdentifier::getRunningStatus() const
{
	return runningStatus;
}

TVAIdDescriptor::TVAIdDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 3) {
		ASSERT_MIN_DLEN(i + 3);
		identifier.push_back(new TVAIdentifier(&buffer[i + 2]));
	}
}

TVAIdDescriptor::~TVAIdDescriptor()
{
	for (TVAIdentifierIterator it = identifier.begin(); it != identifier.end(); ++it)
		delete *it;
}

const TVAIdentifierList *TVAIdDescriptor::getIdentifier() const
{
	return &identifier;
}
