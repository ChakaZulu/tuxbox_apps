/*
 * $Id: content_descriptor.cpp,v 1.2 2005/09/29 23:49:44 ghostrider Exp $
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

#include <dvbsi++/content_descriptor.h>

ContentClassification::ContentClassification(const uint8_t * const buffer)
{
	contentNibbleLevel1 = (buffer[0] >> 4) & 0x0f;
	contentNibbleLevel2 = buffer[0] & 0x0f;
	userNibble1 = (buffer[1] >> 4) & 0x0f;
	userNibble2 = buffer[1] & 0x0f;
}

uint8_t ContentClassification::getContentNibbleLevel1(void) const
{
	return contentNibbleLevel1;
}

uint8_t ContentClassification::getContentNibbleLevel2(void) const
{
	return contentNibbleLevel2;
}

uint8_t ContentClassification::getUserNibble1(void) const
{
	return userNibble1;
}

uint8_t ContentClassification::getUserNibble2(void) const
{
	return userNibble2;
}

ContentDescriptor::ContentDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 2)
		classifications.push_back(new ContentClassification(&buffer[i + 2]));
}

ContentDescriptor::~ContentDescriptor(void)
{
	for (ContentClassificationList::iterator i = classifications.begin(); i != classifications.end(); ++i)
		delete *i;
}

const ContentClassificationList *ContentDescriptor::getClassifications(void) const
{
	return &classifications;
}

