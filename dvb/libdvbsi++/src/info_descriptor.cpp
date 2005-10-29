/*
 * $Id: info_descriptor.cpp,v 1.2 2005/10/29 00:10:17 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/info_descriptor.h>

InfoDescriptor::InfoDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	iso639LanguageCode.assign((char *)&buffer[2], 3);
	info.assign((char *)&buffer[5], descriptorLength - 3);
}

const std::string &InfoDescriptor::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

const std::string &InfoDescriptor::getInfo(void) const
{
	return info;
}
