/*
 * MPE_FEC Section ETSI EN 301 192 V1.4.1
 *
 * $Id: mpe_fec_section.cpp,v 1.3 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/mpe_fec_section.h"

RealTimeParameters::RealTimeParameters(const uint8_t* const buffer)
{
	delta_t = (buffer[0] << 4) | (buffer[1]>>4);
	tableBoundary = (buffer[1]>>3)& 0x01;
	frameBoundary = (buffer[1]>>2) & 0x01;
	address = ((buffer[2] & 0x03) << 8) | buffer[3];
}

RealTimeParameters::~RealTimeParameters()
{
}

uint16_t RealTimeParameters::getDeltaT() const
{
	return delta_t;
}

uint8_t RealTimeParameters::getTableBoundary() const
{
	return tableBoundary;
}

uint8_t RealTimeParameters::getFrameBoundary() const
{
	return frameBoundary;
}

uint32_t RealTimeParameters::getAddress() const
{
	return address;
}

MpeFecSection::MpeFecSection(const uint8_t* const buffer):LongCrcSection(buffer), rtParam(buffer+8), rsDataBytes(sectionLength > 13 ? sectionLength - 13 : 0)
{
	memcpy(&rsDataBytes[0], buffer+11, rsDataBytes.size());
}

MpeFecSection::~MpeFecSection()
{
}

const RealTimeParameters* MpeFecSection::getRealTimeParameters() const
{
	return &rtParam;
}

const RSDataByteVector* MpeFecSection::getRSDataBytes() const
{
	return &rsDataBytes;
}
