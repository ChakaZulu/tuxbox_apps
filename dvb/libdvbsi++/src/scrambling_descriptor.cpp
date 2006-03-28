/*
 * $Id: scrambling_descriptor.cpp,v 1.2 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/scrambling_descriptor.h"

ScramblingDescriptor::ScramblingDescriptor(const uint8_t* const buffer):Descriptor(buffer)
{
	ASSERT_MIN_DLEN(1);

	scramblingMode = buffer[2];
}

ScramblingDescriptor::~ScramblingDescriptor()
{
}

uint8_t ScramblingDescriptor::getScramblingMode() const
{
	return scramblingMode;
}
