/*
 * $Id: ac3_descriptor.cpp,v 1.4 2005/12/26 20:48:58 mws Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/ac3_descriptor.h>

Ac3Descriptor::Ac3Descriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	if (descriptorLength != 0) {
		ac3TypeFlag = (buffer[2] >> 7) & 0x01;
		bsidFlag = (buffer[2] >> 6) & 0x01;
		mainidFlag = (buffer[2] >> 5) & 0x01;
		asvcFlag = (buffer[2] >> 4) & 0x01;
	}
	else {
		ac3TypeFlag = 0;
		bsidFlag = 0;
		mainidFlag = 0;
		asvcFlag = 0;
	}

	size_t i = 3;
	if (ac3TypeFlag == 1)
		ac3Type = buffer[i++];

	if (bsidFlag == 1)
		bsid = buffer[i++];

	if (mainidFlag == 1)
		mainid = buffer[i++];

	if (asvcFlag == 1)
		avsc = buffer[i++];

	if ( descriptorLength > 0)
	{
		additionalInfo.resize(descriptorLength - (i - 2));
		memcpy(&additionalInfo[0], buffer + i, descriptorLength - (i - 2));
	}
	// TODO cleanup after verify
//	for (size_t i = 0; i < descriptorLength - ac3TypeFlag - bsidFlag - mainidFlag - asvcFlag - 1; ++i)
//		additionalInfo.push_back(buffer[ac3TypeFlag + bsidFlag + mainidFlag + asvcFlag + i + 3]);
}

uint8_t Ac3Descriptor::getAc3TypeFlag(void) const
{
	return ac3TypeFlag;
}

uint8_t Ac3Descriptor::getBsidFlag(void) const
{
	return bsidFlag;
}

uint8_t Ac3Descriptor::getMainidFlag(void) const
{
	return mainidFlag;
}

uint8_t Ac3Descriptor::getAsvcFlag(void) const
{
	return asvcFlag;
}

uint8_t Ac3Descriptor::getAc3Type(void) const
{
	return ac3Type;
}

uint8_t Ac3Descriptor::getBsid(void) const
{
	return bsid;
}

uint8_t Ac3Descriptor::getMainid(void) const
{
	return mainid;
}

uint8_t Ac3Descriptor::getAvsc(void) const
{
	return avsc;
}

const AdditionalInfoVector *Ac3Descriptor::getAdditionalInfo(void) const
{
	return &additionalInfo;
}

