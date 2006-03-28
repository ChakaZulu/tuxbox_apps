/*
 * $Id: extended_event_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/extended_event_descriptor.h>

ExtendedEvent::ExtendedEvent(const uint8_t * const buffer)
{
	itemDescriptionLength = buffer[0];
	itemDescription.assign((char *)&buffer[1], itemDescriptionLength);
	itemLength = buffer[itemDescriptionLength + 1];
	item.assign((char *)&buffer[itemDescriptionLength + 2], itemLength);
}

const std::string &ExtendedEvent::getItemDescription(void) const
{
	return itemDescription;
}

const std::string &ExtendedEvent::getItem(void) const
{
	return item;
}

ExtendedEventDescriptor::ExtendedEventDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	size_t headerLength = 6;
	ASSERT_MIN_DLEN(headerLength);

	descriptorNumber = (buffer[2] >> 4) & 0x0f;
	lastDescriptorNumber = buffer[2] & 0x0f;
	iso639LanguageCode.assign((char *)&buffer[3], 3);
	lengthOfItems = buffer[6];

	headerLength += lengthOfItems;
	ASSERT_MIN_DLEN(headerLength);

	ExtendedEvent *e;
	for (size_t i = 0; i < lengthOfItems; i += e->itemDescriptionLength + e->itemLength + 2) {
		e = new ExtendedEvent(&buffer[i + 7]);
		items.push_back(e);
	}

	textLength = buffer[lengthOfItems + 7];

	headerLength += textLength;
	ASSERT_MIN_DLEN(headerLength);

	text.assign((char *)&buffer[lengthOfItems + 8], textLength);
}

ExtendedEventDescriptor::~ExtendedEventDescriptor(void)
{
	for (ExtendedEventIterator i = items.begin(); i != items.end(); ++i)
		delete *i;
}

uint8_t ExtendedEventDescriptor::getDescriptorNumber(void) const
{
	return descriptorNumber;
}

uint8_t ExtendedEventDescriptor::getLastDescriptorNumber(void) const
{
	return lastDescriptorNumber;
}

const std::string &ExtendedEventDescriptor::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

const ExtendedEventList *ExtendedEventDescriptor::getItems(void) const
{
	return &items;
}

const std::string &ExtendedEventDescriptor::getText(void) const
{
	return text;
}

