/*
 * $Id: component_descriptor.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/component_descriptor.h>

ComponentDescriptor::ComponentDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(6);

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

