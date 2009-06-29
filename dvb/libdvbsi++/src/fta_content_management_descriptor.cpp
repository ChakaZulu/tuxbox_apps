/*
 * $Id: fta_content_management_descriptor.cpp,v 1.1 2009/06/29 16:18:28 mws Exp $
 *
 * Copyright (C) 2009 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/fta_content_management_descriptor.h"

FtaContentManagementDescriptor::FtaContentManagementDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	size_t dataLength = 1;
	ASSERT_MIN_DLEN(dataLength);

	doNotScramble = (buffer[2] >> 3) & 0x01;
	controlRemoteAccessOverInternet = (buffer[2] >> 2) &0x03;
	doNotApplyRevocation = buffer[2] & 0x01;
}

uint8_t FtaContentManagementDescriptor::getDoNotScramble() const
{
	return doNotScramble;
}

uint8_t FtaContentManagementDescriptor::getControlRemoteAccessOverInternet() const
{
	return controlRemoteAccessOverInternet;
}

uint8_t FtaContentManagementDescriptor::getDoNotApplyRevocation() const
{
	return doNotApplyRevocation;
}
