/*
 * $Id: component_descriptor.cpp,v 1.1 2004/02/13 15:27:46 obi Exp $
 *
 * Copyright (C) 2002-2004 Andreas Oberritter <obi@saftware.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <dvbsi++/component_descriptor.h>

ComponentDescriptor::ComponentDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	streamContent = buffer[2] & 0x0f;
	componentType = buffer[3];
	componentTag = buffer[4];
	iso639LanguageCode.assign((char *) &buffer[5], 3);
	text.assign((char *) &buffer[8], descriptorLength - 6);
}

uint8_t ComponentDescriptor::getStreamContent(void) const
{
	return streamContent;
}

uint8_t ComponentDescriptor::getComponentType(void) const
{
	return componentType;
}

uint8_t ComponentDescriptor::getComponentTag(void) const
{
	return componentTag;
}

const std::string &ComponentDescriptor::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

const std::string &ComponentDescriptor::getText(void) const
{
	return text;
}

