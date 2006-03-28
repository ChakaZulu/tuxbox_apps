/*
 * $Id: content_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
	for (size_t i = 0; i < descriptorLength; i += 2) {
		ASSERT_MIN_DLEN(i + 2);
		classifications.push_back(new ContentClassification(&buffer[i + 2]));
	}
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

