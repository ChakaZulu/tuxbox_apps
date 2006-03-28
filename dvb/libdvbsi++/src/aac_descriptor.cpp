/*
 * $Id: aac_descriptor.cpp,v 1.2 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/aac_descriptor.h"

AACDescriptor::AACDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	size_t headerLength = 2;
	ASSERT_MIN_DLEN(headerLength);

	profileLevel = buffer[2];
	aacTypeFlag = (buffer[3] >> 7) & 0x01;

	size_t i = 4;
	if (aacTypeFlag == 0x01) {
		headerLength++;
		ASSERT_MIN_DLEN(headerLength);

		aacType = buffer[i++];
	}

	additionalInfoBytes.resize(descriptorLength - headerLength);
	memcpy(&additionalInfoBytes[0], &buffer[i], descriptorLength - headerLength);
}

AACDescriptor::~AACDescriptor()
{
}

uint8_t AACDescriptor::getProfileLevel() const
{
	return profileLevel;
}

uint8_t AACDescriptor::getAACTypeFlag() const
{
	return aacTypeFlag;
}

uint8_t AACDescriptor::getAACType() const
{
	return aacType;
}

const AdditionalInfoByteVector *AACDescriptor::getAdditionalInfoBytes() const
{
	return &additionalInfoBytes;
}
