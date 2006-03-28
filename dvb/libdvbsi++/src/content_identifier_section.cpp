/*
 * $Id: content_identifier_section.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/byte_stream.h"
#include "dvbsi++/content_identifier_section.h"

CridLabel::CridLabel(const uint8_t* const buffer)
{
	cridRef = r16(&buffer[0]);

	prependStringIndex = buffer[2];
	uniqueStringLength = buffer[3];

	for (size_t i = 0; i < uniqueStringLength; i++)
		uniqueStringBytes.push_back(buffer[i+4]);
}

CridLabel::~CridLabel()
{
}

uint16_t CridLabel::getCridRef(void) const
{
	return cridRef;
}

uint8_t CridLabel::getPrependStringIndex() const
{
	return prependStringIndex;
}

uint8_t CridLabel::getUniqueStringLength() const
{
	return uniqueStringLength;
}

const ContentIdentifierByteVector* CridLabel::getUniqueStringBytes() const
{
	return &uniqueStringBytes;
}

ContentIdentifierSection::ContentIdentifierSection(const uint8_t* const buffer) : LongCrcSection(buffer)
{
	transportStreamId = sectionLength > 10 ? r16(&buffer[8]) : 0;
	originalNetworkId = sectionLength > 12 ? r16(&buffer[10]) : 0;
	prependStringLength = sectionLength > 13 ? buffer[12] : 0;

	for (size_t i = 0; i < prependStringLength; i++)
		prependStringsBytes.push_back(buffer[i+13]);

	uint16_t pos = 13 + prependStringLength;
	uint16_t bytesLeft = sectionLength > 14+prependStringLength ? sectionLength-14-prependStringLength : 0;
	uint16_t loopLength = 0;

	while (bytesLeft > 3 && bytesLeft >= (loopLength = 4 + buffer[pos+3])) {
		cridLabels.push_back(new CridLabel(&buffer[pos]));
		bytesLeft -= loopLength;
		pos += loopLength;
	}
}

ContentIdentifierSection::~ContentIdentifierSection(void)
{
	for ( CridLabelIterator it(cridLabels.begin()); it != cridLabels.end(); ++it)
		delete *it;
}

uint16_t ContentIdentifierSection::getTransportStreamId(void) const
{
	return transportStreamId;
}

uint16_t ContentIdentifierSection::getOriginalNetworkId(void) const
{
	return originalNetworkId;
}

uint8_t ContentIdentifierSection::getPrependStringLength(void) const
{
	return prependStringLength;
}

const ContentIdentifierByteVector* ContentIdentifierSection::getPrependStringBytes() const
{
	return &prependStringsBytes;
}

const CridLabelList* ContentIdentifierSection::getCridLabels() const
{
	return &cridLabels;
}
