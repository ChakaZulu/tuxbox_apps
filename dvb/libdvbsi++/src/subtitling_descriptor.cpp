/*
 * $Id: subtitling_descriptor.cpp,v 1.5 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/subtitling_descriptor.h>

Subtitling::Subtitling(const uint8_t * const buffer)
{
	iso639LanguageCode.assign((char *)&buffer[0], 3);
	subtitlingType = buffer[3];
	compositionPageId = UINT16(&buffer[4]);
	ancillaryPageId = UINT16(&buffer[6]);
}

const std::string &Subtitling::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

uint8_t Subtitling::getSubtitlingType(void) const
{
	return subtitlingType;
}

uint16_t Subtitling::getCompositionPageId(void) const
{
	return compositionPageId;
}

uint16_t Subtitling::getAncillaryPageId(void) const
{
	return ancillaryPageId;
}

SubtitlingDescriptor::SubtitlingDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 8) {
		ASSERT_MIN_DLEN(i + 8);
		subtitlings.push_back(new Subtitling(&buffer[i + 2]));
	}
}

SubtitlingDescriptor::~SubtitlingDescriptor(void)
{
	for (SubtitlingIterator i = subtitlings.begin(); i != subtitlings.end(); ++i)
		delete *i;
}

const SubtitlingList *SubtitlingDescriptor::getSubtitlings(void) const
{
	return &subtitlings;
}

