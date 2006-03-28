/*
 * $Id: carousel_identifier_descriptor.cpp,v 1.6 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/carousel_identifier_descriptor.h>
#include <dvbsi++/byte_stream.h>

CarouselIdentifierDescriptor::CarouselIdentifierDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(5);

	carouselId = r32(&buffer[2]);
	formatId = buffer[6];

	switch (formatId) {
	case 0x00:
		privateDataBytes.reserve(descriptorLength - 5);
		memcpy(&privateDataBytes[0], &buffer[7], descriptorLength - 5);
		break;
	case 0x01:
		ASSERT_MIN_DLEN(21);

		moduleVersion = buffer[7];
		moduleId = r16(&buffer[8]);
		blockSize = r16(&buffer[10]);
		moduleSize = r32(&buffer[12]);
		compressionMethod = buffer[16];
		originalSize = r32(&buffer[17]);
		timeout = buffer[21];
		objectKeyLength = buffer[22];

		ASSERT_MIN_DLEN(objectKeyLength + 21);

		objectKey.assign((char *)&buffer[23], objectKeyLength);
		privateDataBytes.reserve(descriptorLength - objectKeyLength - 21);
		memcpy(&privateDataBytes[0], &buffer[objectKeyLength + 23], descriptorLength - objectKeyLength - 21);
		break;
	}
}

uint32_t CarouselIdentifierDescriptor::getCarouselId(void) const
{
	return carouselId;
}

uint8_t CarouselIdentifierDescriptor::getFormatId(void) const
{
	return formatId;
}

uint8_t CarouselIdentifierDescriptor::getModuleVersion(void) const
{
	return moduleVersion;
}

uint16_t CarouselIdentifierDescriptor::getModuleId(void) const
{
	return moduleId;
}

uint16_t CarouselIdentifierDescriptor::getBlockSize(void) const
{
	return blockSize;
}

uint32_t CarouselIdentifierDescriptor::getModuleSize(void) const
{
	return moduleSize;
}

uint8_t CarouselIdentifierDescriptor::getCompressionMethod(void) const
{
	return compressionMethod;
}

uint32_t CarouselIdentifierDescriptor::getOriginalSize(void) const
{
	return originalSize;
}

uint8_t CarouselIdentifierDescriptor::getTimeout(void) const
{
	return timeout;
}

const std::string &CarouselIdentifierDescriptor::getObjectKey(void) const
{
	return objectKey;
}

const PrivateDataByteVector *CarouselIdentifierDescriptor::getPrivateDataBytes(void) const
{
        return &privateDataBytes;
}
