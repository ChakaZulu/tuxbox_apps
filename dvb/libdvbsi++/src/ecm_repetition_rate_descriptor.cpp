/*
 * $Id: ecm_repetition_rate_descriptor.cpp,v 1.1 2005/11/10 23:55:33 mws Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/ecm_repetition_rate_descriptor.h"

#include "dvbsi++/byte_stream.h"

ECMRepetitionRateDecriptor::ECMRepetitionRateDecriptor(const uint8_t* const buffer) : Descriptor(buffer), privateDataBytes(descriptorLength-4)
{
	caSystemId = r16(&buffer[2]);
	repetitionRate = r16(&buffer[4]);
	memcpy(&privateDataBytes[0], buffer+6, descriptorLength-4);
}

ECMRepetitionRateDecriptor::~ECMRepetitionRateDecriptor()
{
}

uint16_t ECMRepetitionRateDecriptor::getCaSystemId(void) const
{
	return caSystemId;
}

uint16_t ECMRepetitionRateDecriptor::getRepetitionRate(void) const
{
	return repetitionRate;
}

const ECMRepetitionPrivateByteVector* ECMRepetitionRateDecriptor::getPrivateDataBytes() const
{
	return &privateDataBytes;
}
