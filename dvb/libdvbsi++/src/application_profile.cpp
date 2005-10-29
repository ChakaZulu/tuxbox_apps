/*
 * $Id: application_profile.cpp,v 1.2 2005/10/29 00:10:16 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/application_profile.h>
#include <dvbsi++/byte_stream.h>
 
ApplicationProfile::ApplicationProfile(const uint8_t * const buffer)
{
	applicationProfile = r16(&buffer[0]);
	versionMajor = buffer[2];
	versionMinor = buffer[3];
	versionMicro = buffer[4];
}

uint16_t ApplicationProfile::getApplicationProfile(void) const
{
	return applicationProfile;
}

uint8_t ApplicationProfile::getVersionMajor(void) const
{
	return versionMajor;
}

uint8_t ApplicationProfile::getVersionMinor(void) const
{
	return versionMinor;
}

uint8_t ApplicationProfile::getVersionMicro(void) const
{
	return versionMicro;
}
