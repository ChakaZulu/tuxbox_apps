/*
 * $Id: short_event_descriptor.cpp,v 1.1 2004/02/13 15:27:47 obi Exp $
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

