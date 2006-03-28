/*
 * $Id: data_broadcast_descriptor.cpp,v 1.5 2006/03/28 17:22:00 ghostrider Exp $
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
#include <dvbsi++/data_broadcast_descriptor.h>

DataBroadcastDescriptor::DataBroadcastDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(8);

	dataBroadcastId = UINT16(&buffer[2]);
	componentTag = buffer[4];
	selectorLength = buffer[5];

	ASSERT_MIN_DLEN(selectorLength + 8);

	for (size_t i = 0; i < selectorLength; ++i)
		selectorBytes.push_back(buffer[i + 6]);

	iso639LanguageCode.assign((char *)&buffer[selectorLength + 6], 3);
	textLength = buffer[selectorLength + 9];

	ASSERT_MIN_DLEN(textLength + selectorLength + 8);

	text.assign((char *)&buffer[selectorLength + 10], textLength);
}

uint16_t DataBroadcastDescriptor::getDataBroadcastId(void) const
{
	return dataBroadcastId;
}

uint8_t DataBroadcastDescriptor::getComponentTag(void) const
{
	return componentTag;
}

const selectorByteList *DataBroadcastDescriptor::getSelectorBytes(void) const
{
	return &selectorBytes;
}

const std::string &DataBroadcastDescriptor::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

const std::string &DataBroadcastDescriptor::getText(void) const
{
	return text;
}

