/*
 * $Id: cpcm_delivery_signalling_descriptor.cpp,v 1.1 2009/06/30 12:03:03 mws Exp $
 *
 * Copyright (C) 2009 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/cpcm_delivery_signalling_descriptor.h"

CpcmDeliverySignallingDescriptor::CpcmDeliverySignallingDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(2);

	extensionTag = buffer[2];
	cpcmVersion = buffer[3];
	selectorBytes.resize(descriptorLength - 2);
	memcpy(&selectorBytes[0], &buffer[4], descriptorLength - 2);
}

CpcmDeliverySignallingDescriptor::~CpcmDeliverySignallingDescriptor()
{
}

uint8_t CpcmDeliverySignallingDescriptor::getExtensionTag() const
{
	return extensionTag;
}

uint8_t CpcmDeliverySignallingDescriptor::getCpcmVersion() const
{
	return cpcmVersion;
}

const SelectorByteVector *CpcmDeliverySignallingDescriptor::getSelectorBytes() const
{
	return &selectorBytes;
}
