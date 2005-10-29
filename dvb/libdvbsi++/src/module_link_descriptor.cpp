/*
 * $Id: module_link_descriptor.cpp,v 1.2 2005/10/29 00:10:17 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/module_link_descriptor.h>
#include <dvbsi++/byte_stream.h>

ModuleLinkDescriptor::ModuleLinkDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	position = buffer[2];
	moduleId = r16(&buffer[3]);
}

uint8_t ModuleLinkDescriptor::getPosition(void) const
{
	return position;
}

uint16_t ModuleLinkDescriptor::getModuleId(void) const
{
	return moduleId;
}
