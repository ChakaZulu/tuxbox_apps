/*
 * $Id: image_icon_descriptor.cpp,v 1.1 2009/06/30 12:03:03 mws Exp $
 *
 * Copyright (C) 2009 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#include "dvbsi++/image_icon_descriptor.h"
#include "dvbsi++/byte_stream.h"

ImageIconDescriptor::ImageIconDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(1);

	size_t	offset = 0;

	extensionTag = buffer[2];
	descriptorNumber = (buffer[3] >> 4) & 0x0f;
	lastDescriptorNumber = buffer[3] & 0x0f;
	iconId = buffer[4] & 0x03;

	if (descriptorNumber == 0x00)
	{
		iconTransportMode = (buffer[5] >> 6) & 0x03;
		positionFlag = (buffer[5] >> 5) & 0x01;

		if (positionFlag == 0x01)
		{
			coordinateSystem = (buffer[5] >> 2) & 0x03;

			iconHorizontalOrigin = (r16(&buffer[6])) >> 4;
			iconVerticalOrigin = (r16(&buffer[7])) & 0x0fff;
			offset = 4;
		}

		iconTypeLength = buffer[5+offset];
		iconTypeChars.resize(iconTypeLength);
		memcpy(&iconTypeChars[0], &buffer[6+offset], iconTypeLength);

		offset += iconTypeLength;

		if (iconTransportMode == 0x00 )
		{
			iconDataLength = buffer[6+offset];
			iconDataBytes.resize(iconDataLength);
			memcpy(&iconDataBytes[0], &buffer[7+offset], iconDataLength);

			offset += iconTypeLength;
		}
		else if (iconTransportMode == 0x01 )
		{
			/* copy url content */
			iconDataLength = buffer[6+offset];
			iconDataBytes.resize(iconDataLength);
			memcpy(&iconDataBytes[0], &buffer[7+offset], iconDataLength);
			offset += iconTypeLength;
		}
	}
	else
	{
		iconDataLength = buffer[5];
		iconDataBytes.resize(iconDataLength);
		memcpy(&iconDataBytes[0], &buffer[6], iconDataLength);
	}
}

ImageIconDescriptor::~ImageIconDescriptor()
{
}

uint8_t ImageIconDescriptor::getExtensionTag() const
{
	return extensionTag;
}

uint8_t ImageIconDescriptor::getDescriptorNumber() const
{
	return descriptorNumber;
}

uint8_t ImageIconDescriptor::getLastDescriptorNumber() const
{
	return lastDescriptorNumber;
}

uint8_t ImageIconDescriptor::getIconId() const
{
	return iconId;
}

uint8_t ImageIconDescriptor::getIconTransportMode() const
{
	return iconTransportMode;
}

uint8_t ImageIconDescriptor::getPositionFlag() const
{
	return positionFlag;
}

uint8_t ImageIconDescriptor::getCoordinateSystem() const
{
	return coordinateSystem;
}

uint16_t ImageIconDescriptor::getIconHorizontalOrigin() const
{
	return iconHorizontalOrigin;
}

uint16_t ImageIconDescriptor::getIconVerticalOrigin() const
{
	return iconVerticalOrigin;
}

const SelectorByteVector* ImageIconDescriptor::getIconTypeChars() const
{
	return &iconTypeChars;
}

const SelectorByteVector* ImageIconDescriptor::getIconDataBytes() const
{
	return &iconDataBytes;
}

const SelectorByteVector* ImageIconDescriptor::getUrlsChars() const
{
	return getIconDataBytes();
}
