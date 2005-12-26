/*
 * $Id: aac_descriptor.cpp,v 1.1 2005/12/26 20:48:58 mws Exp $
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

AACDescriptor::AACDescriptor(const uint8_t* const buffer):Descriptor(buffer)
{
	profileLevel = buffer[2];
	aacTypeFlag = (buffer[3] >> 7) & 0x01;
	size_t cnt = 0x04;
	if ( aacTypeFlag == 0x01 )
	{
		aacType = buffer[cnt++];
	}
	additionalInfoBytes.resize(descriptorLength - (cnt - 2));
	memcpy(&additionalInfoBytes[0], buffer+cnt, descriptorLength - (cnt - 2));
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

const AdditionalInfoByteVector* AACDescriptor::getAdditionalInfoBytes() const
{
	return &additionalInfoBytes;
}
