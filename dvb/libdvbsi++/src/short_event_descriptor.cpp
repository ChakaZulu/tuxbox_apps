/*
 * $Id: short_event_descriptor.cpp,v 1.2 2005/10/29 00:10:17 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/short_event_descriptor.h>

ShortEventDescriptor::ShortEventDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	iso639LanguageCode.assign((char *)&buffer[2], 3);
	eventNameLength = buffer[5];
	eventName.assign((char *)&buffer[6], eventNameLength);
	textLength = buffer[6 + eventNameLength];
	text.assign((char *)&buffer[7 + eventNameLength], textLength);
}

const std::string &ShortEventDescriptor::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

const std::string &ShortEventDescriptor::getEventName(void) const
{
	return eventName;
}

const std::string &ShortEventDescriptor::getText(void) const
{
	return text;
}

